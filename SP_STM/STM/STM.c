///////////////////////////////////////////////////////////////////////////////
//! \file STM.h
//! \brief This is the c file for controlling the STM interface on an SP-CM-STM
//! board
//!
//!
//!
//! @addtogroup
//! @{
//!
//! Written by Samuel Boegli 2012
//! Edited by CP
///////////////////////////////////////////////////////////////////////////////

#include <msp430x23x.h>
//#include STM.h //Now included in core.h
#include "../core/core.h"

//! \def MPS6
//! \brief Decagon's code indicating sensor is MPS6
#define MPS6	0x6C
//! \def FIVETM
//! \brief Decagon's code indicating sensor is  5TM
#define FIVETM	0x78
//! \def FIVETE
//! \brief Decagon's code indicating sensor is  5TE
#define FIVETE	0x7A

//******************  RX Variables  *****************************************//
//! @name Receive Variables
//! These variables are used in the receiving of data on the \ref comm Module.
//! @{
//! \var char g_ucaSTM_RXBuffer[RX_BUFFER_SIZE_STM]
//! \brief The software UART RX Buffer
char g_ucaSTM_RXBuffer[RX_BUFFER_SIZE_STM];

//! \var volatile char g_ucSTM_RXBufferIndex
//! \brief This index into g_ucaRXBuffer showing the current write position.
volatile char g_ucSTM_RXBufferIndex;

//! \var g_ucSTM_RXBitsLeft
//! \brief Bit counter
char g_ucSTM_RXBitsLeft;

//! \var cSTM_RX_Pin
//! \brief The RX pin of the active soil moisture sensor.
char cSTM_RX_Pin;

//! \var g_ucSTM_RXBusy
//! \brief Indicates that we are receiving a byte
uint8 g_ucSTM_RXBusy;
//! @}

//******************  Values  *****************************************//
//! @name The Temperature and Soil Moisture Values
//! Save the calculated Values here
//! @{
//! \var int16 nSTM_Temperature
//! \brief The temperature Value of STM. The correct value *10 to handle floating point
int16 nSTM_Temperature;

//! \var int32 ulSTM_Soil
//! \brief The Soil Moisture Value of STM4.
int32 lSTM_Soil;
//! @}

//! \var uiSTM1_type
//! \brief The sensor type for STM1.
uint8 uiSTM1_type = 0x52;

//! \var uiSTM2_type
//! \brief The sensor type for STM2.
uint8 uiSTM2_type = 0x52;

//! \var uiSTM3_type
//! \brief The sensor type for STM3.
uint8 uiSTM3_type = 0x52;

//! \var uiSTM4_type
//! \brief The sensor type for STM4.
uint8 uiSTM4_type = 0x52;

//! \var timeoutcounter
//! \brief Timeout used to exit the measurement process when response takes too long
char timeoutcounter = 0;

//! \var ISR
//! \var ADCISRindicator
//! \brief Variables used in sensor debugging
char ISR = 0;
char ADCISRindicator = 0;

///////////////////////////////////////////////////////////////////////////////
//!   \brief Initializes the STM program
//!
//!		IO Ports, variables, clock, and turns on 5V power.
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
void vSTM_Initialize(void)
{
	// Set direction and voltage on STM power pins
	P_STM_PWR_OUT &= ~(cSTM_1_PWR_PIN | cSTM_2_PWR_PIN | cSTM_3_PWR_PIN | cSTM_4_PWR_PIN);
	P_STM_PWR_DIR |= (cSTM_1_PWR_PIN | cSTM_2_PWR_PIN | cSTM_3_PWR_PIN | cSTM_4_PWR_PIN);

	// Set the RX flag to 0 indicating not busy
	g_ucSTM_RXBusy = 0;

	// Clear the RX buffer and the index
	for (g_ucSTM_RXBufferIndex = 0x00; g_ucSTM_RXBufferIndex < RX_BUFFER_SIZE_STM; g_ucSTM_RXBufferIndex++)
		g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] = 0xFF;

	g_ucSTM_RXBufferIndex = 0x00;
}

//////////////////////////////////////////////////////////////////////////
//!
//! \brief Reads from the desired soil moisture sensor
//!
//!
//! \param ucSensor
//!
/////////////////////////////////////////////////////////////////////////
char cSTM_Measure(uint8 ucChannel)
{
	uint8 ucChannelIdx;
	uint8 ucaSTMExciteBits[4] = { cSTM_1_PWR_PIN, cSTM_2_PWR_PIN, cSTM_3_PWR_PIN, cSTM_4_PWR_PIN };
	uint8 ucaSTMRXBits[4] = { cSTM_1_RX_PIN, cSTM_2_RX_PIN, cSTM_3_RX_PIN, cSTM_4_RX_PIN };

	// Handle indexing starting at zero
	ucChannelIdx = ucChannel - 1;
	cSTM_RX_Pin = ucaSTMRXBits[ucChannelIdx];
	// Clear the RX buffer and reset index WAS here, but I don't think it's necessary. Just a reminder it's an option...

	TBCTL = (TBSSEL_2 | TBCLR); //select SMCLK clear TBR
	P_STM_PWR_OUT |= ucaSTMExciteBits[ucChannelIdx]; //START exciting the STM

	// ******************Delay for Level Shifter Bug*******************************************************
	// Enable timer interrupt, configure for delay
	TBCCR2 = 50000; //Equal to CPU delay 200000 (12.5 ms)
	TBCCTL1 &= ~CCIE;
	TBCCTL0 &= ~CCIE;
	TBCCTL2 = CCIE; // Enable interrupts
	TBCTL |= MC1; //Continuous Mode

	__bis_SR_register(LPM0_bits); //CPU asleep.

	TBCTL &= ~TBIFG;
	TBCCTL1 &= ~TBIFG; // Clear IFGs
	// **********************************************************************************

	//Enable the falling edge interrupt
	P_STM_RX_IES |= ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IFG &= ~ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IE |= ucaSTMRXBits[ucChannelIdx];

	g_ucSTM_RXBufferIndex = 0;

	//In case there's no STM1 attached, time out after 3 timer B roll overs. Timer was already started and ready for IFG back in delay
	TBCTL |= TBCLR;
	timeoutcounter = 0;

	//Sleep until measurement is done
	while (timeoutcounter < 10)
	{
		__bis_SR_register(LPM0_bits); //CPU asleep.
		//CPU asleep.
		timeoutcounter++;
		// Break out if we have received the packet from the sensor
		if ((g_ucSTM_RXBufferIndex > 0) && (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex - 1] == 0x0A))
			break;
	}

	//Disable Interrupt
	P_STM_RX_IE &= ~ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IFG &= ~ucaSTMRXBits[ucChannelIdx];

	//Turn off STM
	P_STM_PWR_OUT &= ~ucaSTMExciteBits[ucChannelIdx]; //END exciting the STM

	TBCTL = TBCLR; //Clear Timer

	if (timeoutcounter >= 10)
		return 2;

	// verify message integrity
	if (cSTM_Test_Checksum())
		return 1;

	// Parse the message
	cSTM_ReadValue();
	return 0;


}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Tests whether the returned Checksum is correct -> Transmission
//!		was free of errors, or the error compensates itself(possible but unlikely.)
//!
//!
//!   \param char arg: 	1: Evaluate Sensor 1
//!						2: Evaluate Sensor 2
//!						3: Evaluate both Sensor 1 and 2
//!
//!   \return 1: Checksum ok
//!			  0: Checksum fail or parameter fail
///////////////////////////////////////////////////////////////////////////////
char cSTM_Test_Checksum(void)
{
	int add = 0;

	g_ucSTM_RXBufferIndex = 0;

	do
	{
		add += g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex];
		g_ucSTM_RXBufferIndex++;
	}
	while (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] != 0xD);
	add += g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex]; //Add the 0xD
	add += g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex + 1]; //Add one more (x or z)
	add = (add % 64) + 32;
	if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex + 2] == add)
		return 0;

	return 1; //It failed
}

/////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Parses the message from the soil moisture sensor, extracting the sensor
//!  type rather than returning a specific data sample. This method automatically determines
//!  the type of sensor (5TM, 5TE, MPS6, flow-meter) based on the sensor's message. The byte
//!  following the carriage return is the sensor type.
//!
//!  z = 5TE
//!  x = 5TM
//!  l = MPS6
//!  ? = FlowMeter
//!
/////////////////////////////////////////////////////////////////////////////////////////////
uint8 cSTM_RequestSensorType(uint8 ucChannel)
{
	P3OUT |= BIT4;

	uint8 ucSensorType;
	uint8 ucChannelIdx;
	uint8 ucaSTMExciteBits[4] = { cSTM_1_PWR_PIN, cSTM_2_PWR_PIN, cSTM_3_PWR_PIN, cSTM_4_PWR_PIN };
	uint8 ucaSTMRXBits[4] = { cSTM_1_RX_PIN, cSTM_2_RX_PIN, cSTM_3_RX_PIN, cSTM_4_RX_PIN };

	// Handle indexing starting at zero
	ucChannelIdx = ucChannel - 1;
	cSTM_RX_Pin = ucaSTMRXBits[ucChannelIdx];
	// Clear the RX buffer and reset index WAS here, but I don't think it's necessary. Just a reminder it's an option...

	TBCTL = (TBSSEL_2 | TBCLR); //select SMCLK clear TBR
	P_STM_PWR_OUT |= ucaSTMExciteBits[ucChannelIdx]; //START exciting the STM

	// ******************Delay for Level Shifter Bug*******************************************************
	// Enable timer interrupt, configure for delay
	TBCCR2 = 50000; //Equal to CPU delay 200000 (12.5 ms)
	TBCCTL1 &= ~CCIE;
	TBCCTL0 &= ~CCIE;
	TBCCTL2 = CCIE; // Enable interrupts
	TBCTL |= MC1; //Continuous Mode

	__bis_SR_register(LPM0_bits); //CPU asleep.

	TBCTL &= ~TBIFG;
	TBCCTL1 &= ~TBIFG; // Clear IFGs
	// **********************************************************************************

	//Enable the falling edge interrupt
	P_STM_RX_IES |= ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IFG &= ~ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IE |= ucaSTMRXBits[ucChannelIdx];

	g_ucSTM_RXBufferIndex = 0;

	//In case there's no STM1 attached, time out after 3 timer B roll overs. Timer was already started and ready for IFG back in delay
	TBCTL |= TBCLR;
	timeoutcounter = 0;

	//Sleep until measurement is done
	while (timeoutcounter < 3)
	{
		__bis_SR_register(LPM0_bits); //CPU asleep.
		//CPU asleep.
		timeoutcounter++;
		// Break out if we have received the packet from the sensor
		if ((g_ucSTM_RXBufferIndex > 0) && (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex - 1] == 0x0A))
			break;
	}

	//Disable Interrupt
	P_STM_RX_IE &= ~ucaSTMRXBits[ucChannelIdx];
	P_STM_RX_IFG &= ~ucaSTMRXBits[ucChannelIdx];

	//Turn off STM
	P_STM_PWR_OUT &= ~ucaSTMExciteBits[ucChannelIdx]; //END exciting the STM

	TBCTL = TBCLR; //Clear Timer

	if (timeoutcounter >= 3)
		return 2;

	// verify message integrity
	if (cSTM_Test_Checksum())
		return 1;

	// Parse the message
	//cSTM_ReadValue();
	//return 0;

	// Search the RX buffer to determine the sensor type
	for (g_ucSTM_RXBufferIndex = 0; g_ucSTM_RXBufferIndex < RX_BUFFER_SIZE_STM; g_ucSTM_RXBufferIndex++)
	{
		// If we found the carriage return the next byte is the sensor type
		if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] == 0x0D)
		{
			ucSensorType = g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex + 1];
			break;
		}
	}

	// assign type value to appropriate global var
	switch(ucChannel){
		case 1:
			uiSTM1_type = ucSensorType;
			break;

		case 2:
			uiSTM2_type = ucSensorType;
			break;

		case 3:
			uiSTM3_type = ucSensorType;
			break;

		case 4:
			uiSTM4_type = ucSensorType;
			break;

		default:
			break;
	}

	// indicates success
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the sensor type currently stored to a particular channel's global var
//!
///////////////////////////////////////////////////////////////////////////////////////////////
uint8 cSTM_ReturnSensorType(uint8 ucChannel)
{
	switch(ucChannel)
	{
		case 1:
			return uiSTM1_type;

		case 2:
			return uiSTM2_type;

		case 3:
			return uiSTM3_type;

		case 4:
			return uiSTM4_type;

		default:
			return 0x52;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Parses the message from the soil moisture sensor.
//!  This is designed to automatically determine the type of sensor (5TM, 5TE, MPS6)
//!  based on the sensors message.  The byte following the carriage return is the sensor type.
//!
//! z = 5TE
//! x = 5TM
//! l = MPS6
//!
//!
///////////////////////////////////////////////////////////////////////////////////////////////
void cSTM_ReadValue(void)
{
	uint8 ucSensorType;
	uint32 ulMult = 1;

	// Search the RX buffer to determine the sensor type
	for (g_ucSTM_RXBufferIndex = 0; g_ucSTM_RXBufferIndex < RX_BUFFER_SIZE_STM; g_ucSTM_RXBufferIndex++)
	{
		// If we found the carriage return the next byte is the sensor type
		if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] == 0x0D)
		{
			ucSensorType = g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex + 1];
			break;
		}
	}

	// At this point the RX buffer Index is pointing at the carriage return (0x0D)
	// From here we can go backwards through the buffer converting the ascii values
	// to decimal
	switch (ucSensorType)
	{
		case FIVETM:
			// Clear the temperature and soil moisture variable
			nSTM_Temperature = 0;
			lSTM_Soil = 0;

			// Initialize the multiplier
			ulMult = 1;

			// Convert the soil temperature from ascii to decimal
			while (g_ucaSTM_RXBuffer[--g_ucSTM_RXBufferIndex] != 0x20 && g_ucSTM_RXBufferIndex != 0) //0x20 is a " "
			{
				nSTM_Temperature += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;
			}

			// Decrement the index to point to the soil moisture
			g_ucSTM_RXBufferIndex -= 2;

			// Initialize the multiplier
			ulMult = 1;

			// Convert the Soil moisture from ascii to decimal
			while (g_ucSTM_RXBufferIndex-- != 0)
			{
				lSTM_Soil += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;
			}

		break;

		case FIVETE:
			// Clear the temperature and soil moisture variable
			nSTM_Temperature = 0;
			lSTM_Soil = 0;

			// Initialize the multiplier
			ulMult = 1;

			// Loop backwards until we have reached the end of the temperature reading
			while (g_ucaSTM_RXBuffer[--g_ucSTM_RXBufferIndex] != 0x20 && g_ucSTM_RXBufferIndex != 0) //0x20 is a " "
			{
				nSTM_Temperature += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;
			}

			// Loop backwards until we have reached the soil moisture reading
			while (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex--] != 0x20 && g_ucSTM_RXBufferIndex != 0)
				; //0x20 is a " "

			// Initialize the multiplier
			ulMult = 1;

			while (g_ucSTM_RXBufferIndex-- != 0)
			{
				lSTM_Soil += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;
			}

		break;

		case MPS6:
			// Initialize the soil moisture, soil temperature variables
			lSTM_Soil = 0;
			nSTM_Temperature = 0;

			// Start the multiplier at 1
			ulMult = 1;

			// Loop backwards until we have reached the end of the temperature reading
			while (g_ucaSTM_RXBuffer[--g_ucSTM_RXBufferIndex] != 0x20 && g_ucSTM_RXBufferIndex != 0)
			{
				// Ignore the decimal point
				if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] == 0x2E)
					continue;

				nSTM_Temperature += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;

				// If the temperature is negative then multiply by -1
				if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] == 0x2D)
					nSTM_Temperature = -1 * nSTM_Temperature;
			}

			g_ucSTM_RXBufferIndex--; //Skip the ' '

			// Reset the multiplier
			ulMult = 1;

			// Loop back and convert until we reach the negative sign (0x2D)
			while (g_ucaSTM_RXBuffer[--g_ucSTM_RXBufferIndex] != 0x2D)
			{
				// Ignore the decimal point
				if (g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] == 0x2E)
					continue;

				lSTM_Soil += ((g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] - 48) * ulMult);
				ulMult *= 10;
			}
			// For the MPS6 the soil moisture values are always negative
			lSTM_Soil *= -1;

		break;

		default:
		break;

	} //End: switch()
} //End:cSTM_ReadValue

///////////////////////////////////////////////////////////////////////////////
//!   \brief Returns Soil Moisture value
//!
//!   \return lSTM_Soil
///////////////////////////////////////////////////////////////////////////////
int32 lSTM_GetSoil(void)
{
	return lSTM_Soil;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Returns Soil Temperature value
//!
//!
//!   \return Temperature
///////////////////////////////////////////////////////////////////////////////
int iSTM_GetTemp(void)
{
	return nSTM_Temperature;
}

/////////////////////////////////////////////////////////////////////////////////
////!   \brief Second Interrupt Handler for TimerB (TimerB0 is the other)
////!
////!	  Timer is used to read the UART data from the sensor. Count down the bits
////!   until there are none left(Start, 8 bits, plus stop bit to make a byte,
////!   then finish. If more bytes are to be sent, a new IO interrupt will be
////!   called when the start bit comes. The byte index is incremented.
////!
////!   \param none
////!
////!   \return none
/////////////////////////////////////////////////////////////////////////////////
//#pragma vector=TIMERB1_VECTOR
//__interrupt void TIMERB1_ISR(void)
//{
//
//	switch (__even_in_range(TBIV, 14))
//	{
//		case TBIV_NONE: /* No Interrupt pending */
//			__no_operation();
//		break;
//
//		case TBIV_TBCCR1: /* TBCCR1_CCIFG */
//			if (g_ucSTM_RXBusy)
//			{
//				TBCTL |= TBCLR; //Clear
//				TBCTL |= MC1; //Continuous Mode
//
//				switch (g_ucSTM_RXBitsLeft)
//				{
//					case 0x00:
//						// There are no bits left, so lets reset all the values and stop timer
//						P_STM_RX_IE |= cSTM_RX_Pin;
//						P_STM_RX_IFG &= ~cSTM_RX_Pin;
//						g_ucSTM_RXBufferIndex++;
//						g_ucSTM_RXBusy = 0;
//					break;
//
//					case 0x01:
//						if (P_STM_RX_IN & cSTM_RX_Pin)
//							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] |= 0x80;
//						else
//							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] &= ~0x80;
//
//					break;
//
//					default:
//						if (P_STM_RX_IN & cSTM_RX_Pin)
//							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] |= 0x80;
//						else
//							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] &= ~0x80;
//
//						g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] >>= 1;
//					break;
//				}
//			}
//
//			g_ucSTM_RXBitsLeft--;
//		break;
//
//		case TBIV_TBCCR2: /* TBCCR2_CCIFG */
//			__no_operation();
//			__bic_SR_register_on_exit(LPM4_bits);
//		break;
//
//		case TBIV_3: /* Reserved */
//		case TBIV_4: /* Reserved */
//		case TBIV_5: /* Reserved */
//		case TBIV_6: /* Reserved */
//		break;
//
//		case TBIV_TBIFG:
//		break;
//
//	}
//}

/////////////////////////////////////////////////////////////////////////////////
////!   \brief Port 1 interrupt service routine
////!
////! Used for both transducer debugging and bit-banging
////!
////!   \param none
////!   \return none
/////////////////////////////////////////////////////////////////////////////////
//#pragma vector=PORT1_VECTOR
//__interrupt void PORT1_ISR(void)
//{
//	if (P1IFG & SDA_PIN) {
//		P_SDA_IFG &= ~SDA_PIN;
//		__bic_SR_register_on_exit(LPM4_bits);
//	}
//
//	if (P_STM_RX_IFG & cSTM_RX_Pin)
//	{
//		timeoutcounter = 0;
//		// Delay for half bit, this ensures we start sampling at the middle of
//		// each bit
//		TBCTL &= ~(MC0 | MC1 | TBIE | TBIFG); //Halt timer, Disable interrupts
//		TBCTL |= TBCLR; //Clear Timer
//		TBCCTL1 &= ~CCIFG;
//		TBCCTL1 &= ~CCIE; //Disable Compare interrupt
//		TBCCTL0 &= ~CCIE; //Disable Compare interrupt
//		TBCCR0 = BAUD_1200_DELAY; //Input delay time in compare register
//		TBCTL |= MC_1; //Start timer in UP Mode (Counts to TBCCR0 then starts at 0 again)
//		while (!(TBCTL & TBIFG)); //Wait until Compare register is met
//		TBCTL &= ~TBIFG;
//		// Enable timer interrupt, configure for baud rate
//		TBCTL &= ~(MC0 | MC1); //Halt timer
//		TBCTL |= TBCLR; //Clear
//		TBCCTL1 |= CCIE;
//		TBCCR0 = 0;
//		TBCCR1 = BAUD_1200;
//		__bic_SR_register_on_exit(LPM4_bits);
//		//All Clocks and CPU etc awake now that we received a start bit.
//		// Disable interrupt on RX
//		P_STM_RX_IE &= ~cSTM_RX_Pin;
//		P_STM_RX_IFG &= ~cSTM_RX_Pin;
//		//*****************
//		g_ucSTM_RXBitsLeft = 0x08;
//		g_ucSTM_RXBusy = 1;
//		//*****************
//		TBCTL |= MC1; //Continuous Mode
//		//Clear Interrupt Flag
//		TBCCTL1 &= ~TBIFG; //IFG
//
//	} //END if(P_STM_RX_IFG & cSTM_1_RX_PIN)//P2IFG & BIT4
//}
//! @}
