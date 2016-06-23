///////////////////////////////////////////////////////////////////////////////
//! \file comm.c
//! \brief This modules implements a software I2C on any two Digital I/O pins
//!
//! This module uses Port 2 to implement a software I2C on any two defined
//! digital I/O pin. Note that the serial clock (SCL) pin must be interrupt capable.
//!
//! @addtogroup core
//! @{
//!
//! @addtogroup comm Software I2C
//! The software I2C module allows the user to define a I2C interface on
//! any two GPIO pins, provided that the SCL pin is interrupt capable.
//! @{
///////////////////////////////////////////////////////////////////////////////
//*****************************************************************************
// By: Kenji Yamamoto
//     Wireless Networks Research Lab
//     Dept. of Electrical Engineering, CEFNS
//     Northern Arizona University
//
// Edited By: Christopher Porter
//*****************************************************************************

#include <msp430x23x.h>
#include "core.h"
#include "comm.h"
#include "crc.h"

//******************  Control and Indication Variables  *********************//
//! @name Control and Indication Variables
//! These variables are used to indicate to the system the current status
//! of the \ref comm Module and to store the baud rate timer information.
//! @{
//! \var volatile uint8 g_ucCOMM_Flags
//! \brief This 8-bit field indicates the status of the COMM module.
volatile uint8 g_ucCOMM_Flags;

//! \var uint16 g_unCOMM_BaudRateControl
//! \brief This is the value used to control the baud rate.
//!
//! This value is the number of timer ticks corresponding to one bit period
//! for the baud rate. It should be set from the \ref comm_baud
//! "Baud Rate Defines".
uint16 g_unCOMM_BaudRateControl;

//! \var uint16 g_unCOMM_BaudRateDelayControl
//! \brief This is the value used to delay from the start bit
//!
//! This value is the number of timer ticks to wait from the beginning of the
//! start bit to the middle of the first data bit. It should be set from the
//! \ref comm_baud_delay "Baud Rate Start Delays".
uint16 g_unCOMM_BaudRateDelayControl;
//! @}

//******************  RX Variables  *****************************************//
//! @name Receive Variables
//! These variables are used in the receiving of data on the \ref comm Module.
//! @{
//! \var volatile uint8 g_ucaRXBuffer[MAXMSGLEN]
//! \brief The software UART RX Buffer
volatile uint8 g_ucaRXBuffer[MAXMSGLEN];

//! \var volatile uint8 g_ucRXBufferIndex
//! \brief This index into g_ucaRXBuffer showing the current write position.
volatile uint8 g_ucRXBufferIndex;

//! \var uint8 g_ucRXBitsLeft
//! \brief The number of bits left to be received for the current byte.
uint8 g_ucRXBitsLeft;

//! \var uint8 g_ucRXParityBit
//! \brief Even Parity for bit banging uart
uint8 ucRXParityBit;
//! @}


//******************  Functions  ********************************************//
///////////////////////////////////////////////////////////////////////////////
//! \brief This sets up the hardware resources for doing software UART
//!
//! Since we are doing UART without the USCI, we use TimerA and it's interrupt
//! to control the baud rate. The SDA and SCL pins are completely controllable
//! at compile time. The software UART expects 1 start bit, 8 data bits and
//! 1 stop bit.
//!
//! To ensure correct operation of the software UART, the \ref comm_pins
//! "Comm Pin Defines" must be set correctly.
//!   \param ucBaud The baud rate define to use
//!   \return None
//!   \sa vCOMM_SendByte(), TIMERA0_ISR()
///////////////////////////////////////////////////////////////////////////////
void vCOMM_Init()
{
	// We set the directionality of the SDA and SCL pins based on the defines
	P_SDA_DIR &= ~SDA_PIN;
	P_SCL_DIR &= ~SCL_PIN;

	// Clear the RX buffer and reset index
	g_ucRXBufferIndex = MAXMSGLEN;
	while (g_ucRXBufferIndex) {
		g_ucRXBufferIndex--;
		g_ucaRXBuffer[g_ucRXBufferIndex] = 0xFF;
	}
	g_ucRXBufferIndex = 0x00;

	// Set up for falling edge interrupts on SCL
	P_SCL_IES |= SCL_PIN;
	P_SCL_IFG &= ~SCL_PIN;

	// Set up falling edge interrupt on the SDA line (to handle start condition)
	P_SDA_IES |= SDA_PIN;
	P_SDA_IFG &= ~SDA_PIN;

	// Enable interrupts on the dedicated interrupt line
	P_INT_IES &= ~INT_PIN;
	P_INT_IFG &= ~INT_PIN;
	P_INT_IE |= INT_PIN;

	g_ucCOMM_Flags = COMM_RUNNING;
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Waits for the start signal from the CP board
//!
//! Officially the start condition does not include pulling the clock line low,
//! but the clock needs to be low before entering the send and receive bytes
//! functions.
//!
//!          __________________________
//! SCL _____|                         |____
//!           _________________
//! SDA _____|                 |_____________
//!
//!   \param None
//!   \return 1 if start condition received else 0
///////////////////////////////////////////////////////////////////////////////
uint8 ucCOMM_WaitForStartCondition(void)
{
	// Clear the flag
	g_ucCOMM_Flags &= ~COMM_START_CONDITION;

	// Enable interrupts on the SDA line
	P_SDA_IFG &= ~SDA_PIN;
	P_SDA_IE |= SDA_PIN;

	// Wait in deep sleep
	LPM3;

	// Disable interrupts on the SDA line
	P_SDA_IE &= ~SDA_PIN;

	// Prepare for communication if the start condition flag is set
	if (g_ucCOMM_Flags & COMM_START_CONDITION) {

		// Clear the flag
		g_ucCOMM_Flags &= ~COMM_START_CONDITION;

		// Enable interrupts on the falling edge of the clock line
		P_SCL_IFG &= ~SCL_PIN;
		P_SCL_IES |= SCL_PIN;

		// Wait for the clock to go low then clear the flag
		while (!(P_SCL_IFG & SCL_PIN));
		P_SCL_IFG &= ~SCL_PIN;

		return 1;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Sends a byte via the software I2C
//!
//! This function has been optimized for speed and therefore appears cumbersome
//! by most coding standards.  Switch case statements, while more elegant, generate
//! jump tables in assembly which take several cycles before executing a particular case.
//! They have been replace with if statements which only require 2 or 3 instructions.
//!
//!   \param ucTXChar The 8-bit value to send
//!   \return None
///////////////////////////////////////////////////////////////////////////////
uint8 ucCOMM_SendByte(uint8 ucTXChar)
{
	uint8 ucParityBit;
	uint8 ucBitIdx;
	uint8 ucTXBitsLeft;
	uint8 ucAck;
	uint16 uiTXChar;
	uint8 ucSDABit;
	uint8 ucSCLBit;

	// If we are already busy, return
	if (g_ucCOMM_Flags & COMM_TX_BUSY)
		return COMM_ERROR;

	// Indicate in the status register that we are now busy
	g_ucCOMM_Flags |= COMM_TX_BUSY;

	// Local declarations of this bits place the values in registers - accessing them is faster.
	ucSDABit = (uint8)SDA_PIN;
	ucSCLBit = (uint8)SCL_PIN;

	// Calculate the parity bit prior to transmission
	ucParityBit = 0;
	for (ucBitIdx = 0; ucBitIdx < 8; ucBitIdx++) {
		ucParityBit ^= ((ucTXChar >> ucBitIdx) & 0x01);
	}

	// Combine the data and parity into one variable (more time saving)
	uiTXChar = (uint16)(ucTXChar | (ucParityBit << 8));

	// Reset the bit count
	ucTXBitsLeft = 9;

	// Enable interrupts on the falling edge of the clock line
	P_SCL_IFG &= ~ucSCLBit;
	P_SCL_IES |= ucSCLBit;

	// Set the direction of the data line to output
	P_SDA_DIR |= ucSDABit;

	// Transmission loop which controls the UART timing for byte transmission
	do{

		// For data bits, mask to get correct value and the shift for next time
		if (uiTXChar & 0x01)
			P_SDA_OUT |= ucSDABit;
		else
			P_SDA_OUT &= ~ucSDABit;

		// Decrement the total bit count and shift byte
		ucTXBitsLeft--;
		uiTXChar >>= 1;

		// Wait for the next falling clock
		while (!(P_SCL_IFG & ucSCLBit));
		P_SCL_IFG &= ~ucSCLBit;

	}while (ucTXBitsLeft != 0);

	// Next bit is ack so switch the direction of the SDA pin
	P_SDA_DIR &= ~ucSDABit;

	// Switch clock edge interrupt
	P_SCL_IES &= ~ucSCLBit;

	// Wait for the next rising clock
	while (!(P_SCL_IFG & ucSCLBit));
	P_SCL_IFG &= ~ucSCLBit;

	// Last bit is ack bit, return to idle state
	ucAck = (P_SDA_IN & ucSDABit);

	// Switch clock edge interrupt
	P_SCL_IES |= ucSCLBit;

	// Wait for the next clock
	while (!(P_SCL_IFG & ucSCLBit));
	P_SCL_IFG &= ~ucSCLBit;

	g_ucCOMM_Flags &= ~COMM_TX_BUSY;

	if (ucAck == 1)
		return COMM_ACK_ERR;
	else
		return COMM_OK;
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Receives a byte via the software I2C
//!
//! This function has been optimized for speed and therefore appears cumbersome
//! by most coding standards.  Switch case statements, while more elegant, generate
//! jump tables in assembly which take several cycles before executing a particular case.
//! They have been replace with if statements which only require 2 or 3 instructions.
//!
//!   \param none
//!   \return error code
//!   \sa vCOMM_Init()
///////////////////////////////////////////////////////////////////////////////
uint8 ucCOMM_ReceiveByte(void)
{
	uint8 ucRXBitsLeft;
	uint8 ucParityBit; // The calculated parity bit
	uint8 ucRxParityBit; // The received parity bit
	uint8 ucRXByte;

	// If we are already busy, return
	if (g_ucCOMM_Flags & COMM_RX_BUSY)
		return COMM_ERROR;

	// Indicate in the status register that we are now busy
	g_ucCOMM_Flags |= COMM_RX_BUSY;

	// Init the parity bit and the bit count
	ucParityBit = 0;
	ucRXBitsLeft = 8;

	// Enable interrupts on rising edges of the SCL line
	P_SCL_IFG &= ~SCL_PIN;
	P_SCL_IES &= ~SCL_PIN;

	// Set the direction of the data line to input
	P_SDA_DIR &= ~SDA_PIN;

	do {
		// Wait for the next clock
		while (!(P_SCL_IFG & SCL_PIN));
		P_SCL_IFG &= ~SCL_PIN;

		// Shift over for the next bit
		ucRXByte >>= 1;

		// Sample the data line
		if (P_SDA_IN & SDA_PIN) {
			ucRXByte |= 0x80;
			ucParityBit++;
		}
		else {
			ucRXByte &= ~0x80;
		}

	}while (--ucRXBitsLeft != 0);

	// Wait for the next rising clock
	while (!(P_SCL_IFG & SCL_PIN));
	P_SCL_IFG &= ~SCL_PIN;

	// Sample the parity bit
	if (P_SDA_IN & SDA_PIN)
		ucRxParityBit = 1;
	else
		ucRxParityBit = 0;

	// Set the interrupt edge select for falling
	P_SCL_IES |= SCL_PIN;

	// Wait for the next falling clock
	while (!(P_SCL_IFG & SCL_PIN));
	P_SCL_IFG &= ~SCL_PIN;

	ucParityBit = ucParityBit % 2;
	// If the calculated and received parity bits match then send ack, else nack
	if (ucParityBit == ucRxParityBit) {
		P_SDA_OUT &= ~SDA_PIN;
	}
	else {
		P_SDA_OUT |= SDA_PIN;
	}

	// Next bit is ack so switch the direction of the SDA pin
	P_SDA_DIR |= SDA_PIN;

	// Wait for the next falling clock clock
	while (!(P_SCL_IFG & SCL_PIN));
	P_SCL_IFG &= ~SCL_PIN;

	// Switch direction back to input
	P_SDA_DIR &= ~SDA_PIN;

	g_ucCOMM_Flags &= ~COMM_RX_BUSY;

	// Set the parity error flag
	if (ucParityBit != ucRxParityBit)
		g_ucCOMM_Flags |= COMM_PARITY_ERR;

	g_ucaRXBuffer[g_ucRXBufferIndex] = ucRXByte;
	g_ucRXBufferIndex++; // Increment index for next byte

	return COMM_OK;

}

///////////////////////////////////////////////////////////////////////////////
//! \brief Shuts off the software modules
//!
//! This shuts down TimerA and disables all of the interrupts used
//!   \param None
//!   \return None
//!   \sa vCOMM_Init()
///////////////////////////////////////////////////////////////////////////////
void vCOMM_Shutdown(void)
{

	// Disable RX interrupt
	P_SCL_IE &= ~SCL_PIN;
	g_ucCOMM_Flags &= ~COMM_RUNNING;

	//Let SDA drop
	P_SDA_OUT &= ~SDA_PIN;
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Waits for a message on the serial line
//!
//!
//! \param none
//! \return none
///////////////////////////////////////////////////////////////////////////////
uint8 ucCOMM_WaitForMessage(void)
{

uint8 ucRXMessageSize;

 // Set the size of the received message to the minimum
	ucRXMessageSize = SP_HEADERSIZE;

	// Wait to receive the message
	do {

		if (ucCOMM_ReceiveByte()) {
			return COMM_ERROR;
		}

		// If we have received the header of the message, update the RX message to
		// the size of the message received
		if (g_ucRXBufferIndex == SP_HEADERSIZE) {
			ucRXMessageSize = g_ucaRXBuffer[MSG_LEN_IDX] + CRC_SZ;

			// Range check the g_ucRXMessageSize variable
			if (ucRXMessageSize > MAXMSGLEN || ucRXMessageSize < SP_HEADERSIZE)
				return COMM_ERROR;
		}

	}
	while (g_ucRXBufferIndex != ucRXMessageSize);

	// No message received
	if (g_ucRXBufferIndex == 0) {
		return COMM_ERROR;
	}

	//success
	return 0;

}

///////////////////////////////////////////////////////////////////////////////
//! \brief Sends a data message on the serial port
//!
//! This function sends the data message pointed to by \e p_DataMessage on the
//! software UART line
//!   \param p_DataMessage Pointer to the message to send
//!   \return None
///////////////////////////////////////////////////////////////////////////////.
void vCOMM_SendMessage(volatile uint8 * pBuff, uint8 ucLength)
{
	uint8 ucLoopCount;
	uint8 ucErrorCount;

	// Clear error count
	ucErrorCount = 0;

	// add the CRC bytes to the length
	ucLength += CRC_SZ;

	// Compute the CRC of the message
	ucCRC16_compute_msg_CRC(CRC_FOR_MSG_TO_SEND, pBuff, ucLength);

	for (ucLoopCount = 0x00; ucLoopCount < ucLength; ucLoopCount++) {

		// Attempt to send a byte
		if (ucCOMM_SendByte(*pBuff++) != COMM_OK) {

			// If there is an error then increment the error count
			ucErrorCount++;

			// Decrement the loop count to attempt to resend the byte
			ucLoopCount--;

			// If the error count reaches 5 then consider this a failure
			if (ucErrorCount == 5)
				break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Grabs the raw chars from buffer and formats into a data message
//!
//! This function takes the characters from \e g_ucaRXBuffer and
//! stores them in the data message pointed to by \e message.
//!   \param message Pointer to the message
//!   \return The error code indicating the status after call
//!   \sa comm.h msg.h
///////////////////////////////////////////////////////////////////////////////
uint8 ucCOMM_GrabMessageFromBuffer(volatile uint8 * pucBuff)
{
	uint8 ucLoopCount;
	uint8 ucLength;

	if (g_ucRXBufferIndex < SP_HEADERSIZE)
		return COMM_BUFFER_UNDERFLOW;

	// Read the length of the message
	ucLength = g_ucaRXBuffer[MSG_LEN_IDX];

	// If the message is to long return error
	if (ucLength > MAXMSGLEN)
		return COMM_BUFFER_UNDERFLOW;

	// Check the CRC of the message
	if (!ucCRC16_compute_msg_CRC(CRC_FOR_MSG_TO_REC, g_ucaRXBuffer, g_ucaRXBuffer[MSG_LEN_IDX] + CRC_SZ))
		return COMM_ERROR;

	for (ucLoopCount = 0x00; ucLoopCount < ucLength; ucLoopCount++)
		*pucBuff++ = g_ucaRXBuffer[ucLoopCount];

	// Reset the RX index
	g_ucRXBufferIndex = 0x00;

	return COMM_OK;
}

//! @}
//! @}

