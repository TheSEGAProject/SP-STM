#if 0 //This file is not needed
///////////////////////////////////////////////////////////////////////////////
//! \file uartCom.c
//! \brief This is the primary file for the UART interface.
//!
//!
//!
//! @addtogroup uartCom Computer to SP UART
//! The Computer to SP Board UART interface is used mainly to observe the SP
//! Software during debugging. It can usually be removed in the final release.
//! @{
//!
//! Written by Samuel Boegli 2012
///////////////////////////////////////////////////////////////////////////////

#include <msp430x23x.h>
//#include "uartCom.h" //Now included in changeable_core_header.h
#include "../core/core.h"


//******************  UART Buffer Variables  ***************************//
//! @name UART Buffer Variables
//! These are the buffer and related index variables
//! @{
//! \var char cRXBuffer[UART_BUFFERSIZE]
//! \brief The char buffer for the UART
char cRXBuffer[UART_BUFFERSIZE];

//! \var unsigned int cBufferInCounter
//! \brief The pointer at the next place to enter data from UART
unsigned int cBufferInCounter;

//! \var unsigned int cBufferOutCounter
//! \brief The pointer at the next place to read data from UART
unsigned int cBufferOutCounter;

//! \var char string1[UART_BUFFERSIZE]
//! \brief The buffer is turned into a string and stored in this variable
char string1[UART_BUFFERSIZE];

//! \var char length
//! \brief The length of the data in the buffer
char length;

//! @}

//******************  UART Handler Variables  ***************************//
//! @name UART Handler Variables
//! These variables are used globally for the Handler if data was entered
//! in the UART Buffer.
//! @{
//! \var char valveQ
//! \brief If valve was demanded by UART (Handle_UART function)
char valveQ = 0;

//! \var unsigned int time
//! \brief This is the variable into which the time that valve should run is entered
//! (Handle_UART function)
unsigned int time = 0;

//! \var char SoilQ
//! \brief If STM Measurement was demanded by UART (Handle_UART function)
char SoilQ = 0;

//! @}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Init the UART Module (baud rate, clock, ports, interrupts) and
//!      UART buffer
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
void vUARTCOM_Initialize(void)
{
	UCA0CTL1 |= UCSWRST; //Reset. Set this when wanting to change registers
	UCA0CTL0 &= ~(UCMODE0 + UCMODE1);//0x06; //UART Mode (00)
	UCA0CTL1 = UCSSEL_3; //SMCLK
	UCA0BR0 = 160; //416 for baud rate 9600
	UCA0BR1 = 1;
	P3SEL |= (BIT4 + BIT5);//0x30; //Make 3.4/3.5 UART
	cBufferInCounter = 0; //Init the buffer pointers
	cBufferOutCounter = 0;
	UCA0CTL1 &= ~UCSWRST; //End UART Reset
	IE2 |= UCA0RXIE; //Enable RX interrupts
}



///////////////////////////////////////////////////////////////////////////////
//!   \brief Write a string of data into UART TX port
//!
//!   Does not function in a parallel fashion.
//!   (wait till each byte is done -> slow), improve with interrupt?
//!
//!   \param string: pointer at first char to be written.
//!          len: the length of the string
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
void vUARTCOM_TXString( char* string, int len )
{
  int pointer;
  for( pointer = 0; pointer < len; pointer++)
  {
    volatile int i;
    UCA0TXBUF = string[pointer];
    while (!(IFG2&UCA0TXIFG));              // USCI_A0 TX buffer ready for next byte?
  }
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Find out how many bytes of data are in the buffer and haven't
//!      been read.
//!
//!   \param none
//!
//!   \return length of the unread buffer
///////////////////////////////////////////////////////////////////////////////
unsigned char ucUARTCOM_getBufferFill(void)
{
	if(cBufferInCounter < cBufferOutCounter){//If buffer pointer has wrapped around
		return (cBufferInCounter + (UART_BUFFERSIZE-cBufferOutCounter));
	}
	return cBufferInCounter - cBufferOutCounter;
}


///////////////////////////////////////////////////////////////////////////////
//!   \brief Find out if last symbol in buffer is a return -> complete command.
//!
//!   \param none
//!
//!   \return 1: last entry is return, 0: not a return
///////////////////////////////////////////////////////////////////////////////
unsigned char ucUARTCOM_LastIsReturn()
{
	char place = cBufferInCounter-1;//The last place was one before the pointer
	if(!cBufferInCounter)//If it is =0 and therefore the return would be in the last place
		place = UART_BUFFERSIZE-1;
	if(cRXBuffer[place] == 0x0A){//if return
		return 1;
	}
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
//!   \brief Write Buffer to String
//!
//!   Empty the whole buffer into a given string. Make sure the string is long
//!   enough (UART_BUFFERSIZE if possible).
//!
//!   This will include the possible return at the end of the string.
//!   The read pointer is moved, so if that is not requested, move it back by
//!   the length returned.
//!
//!   \param stringp: pointer to an empty string to be filled by buffer
//!
//!   \return length of string
///////////////////////////////////////////////////////////////////////////////
unsigned char cpUARTCOM_readBuffer(char * stringp)
{
	length = ucUARTCOM_getBufferFill();
	while(cBufferOutCounter != cBufferInCounter){
		*stringp = cRXBuffer[cBufferOutCounter];
		stringp++;
		if(cBufferOutCounter < UART_BUFFERSIZE-1)//Advance the read pointer
			cBufferOutCounter++;
		else
			cBufferOutCounter = 0;
	}

	return length;//Return the length of the string
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Display the input voltage on UART.
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
void vUART_DisplayVoltage()
{
  unsigned int voltage = unCORE_GetVoltage();
  char temp = (voltage/100)+48;
  vUARTCOM_TXString( "\r\nInput Voltage: ", 17);
  vUARTCOM_TXString( &temp, 1);
  temp = ((voltage % 100)/10)+48;
  vUARTCOM_TXString( ".", 1);
  vUARTCOM_TXString( &temp, 1);
  temp = (voltage%10)+48;
  vUARTCOM_TXString( &temp, 1);
  vUARTCOM_TXString( "\r\n", 2);
}


///////////////////////////////////////////////////////////////////////////////
//!   \brief If a UART message was received, handle the message.
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
void vUARTCOM_HandleUART(void){
	while(cBufferOutCounter != cBufferInCounter){
		switch(cRXBuffer[cBufferOutCounter]){
		// 's' is the command that a STM Soil Sensor must be activated. '1' or '2' follow
		case 's':
			SoilQ = 1;
			break;
		case 'v':
			break;
		case '1':

			if(SoilQ)
			{
				vUARTCOM_TXString("Activating STM 1\r\n",18);
				vSTM_Initialize();
				cSTM_Measure1();
				vSTM_Display(1);
				vUARTCOM_TXString("Finished STM 1\r\n",16);
			}
			break;
		case '2':

			if(SoilQ)
			{
				vUARTCOM_TXString("Activating STM 2\r\n",18);
				vSTM_Initialize();
				cSTM_Measure2();
				vSTM_Display(2);
				vUARTCOM_TXString("Finished STM 2\r\n",16);
			}
			break;
		case '3':

			if(SoilQ)
			{
				vUARTCOM_TXString("Activating STM 3\r\n",18);
				vSTM_Initialize();
				cSTM_Measure3();
				vSTM_Display(3);
				vUARTCOM_TXString("Finished STM 3\r\n",16);
			}
			break;
		case '4':

			if(SoilQ)
			{
				vUARTCOM_TXString("Activating STM 4\r\n",18);
				vSTM_Initialize();
				cSTM_Measure4();
				vSTM_Display(4);
				vUARTCOM_TXString("Finished STM 4\r\n",16);
			}
			break;

		//Just a test case that outputs "Hello World"
		case 'h':
			vUARTCOM_TXString("Hello World!\r\n",14);
			break;
		}
		//Move to the next char
		if(cBufferOutCounter < UART_BUFFERSIZE-1)
			cBufferOutCounter++;
		else
			cBufferOutCounter = 0;
	}
	valveQ = 0;
	SoilQ = 0;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Interrupt Handler writes UCA0BUF into the UART Buffer and ends LPM4
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
	//We do not worry about whether it's USCI Module A or B that reacted since B is not used.
	cRXBuffer[cBufferInCounter] = UCA0RXBUF;
	if(cBufferInCounter < UART_BUFFERSIZE-1)
		cBufferInCounter++;
	else
		cBufferInCounter = 0;
	__bic_SR_register_on_exit(LPM4_bits); //All Clocks and CPU etc awake.
}

//! @}
#endif
