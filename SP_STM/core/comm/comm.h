///////////////////////////////////////////////////////////////////////////////
//! \file comm.h
//! \brief Header file for the software UART module
//!
//! This file provides all of the defines and function prototypes for the
//! \ref comm Module.
//!
//! @addtogroup core
//! @{
//!
//! @addtogroup comm Software UART
//! The software UART module allows the user to define a UART interface on
//! any two GPIO pins, provided that the RX pin is interrupt capable. The
//! module requires the use of TimerA.
//! @{
///////////////////////////////////////////////////////////////////////////////
//*****************************************************************************
// By: Kenji Yamamoto
//     Wireless Networks Research Lab
//     Dept. of Electrical Engineering, CEFNS
//     Northern Arizona University
//
//*****************************************************************************

#ifndef COMM_H_
#define COMM_H_

//******************  Pin Configurations  ***********************************//
// Defines for the SDA pin (currently 1.1)
//! @name Pin Defines
//! These defines are used so that the software has the right hardware
//! registers for the serial data and serial clock pins.
//! @{
//! \def SDA_PIN
//! \brief The pin number of the serial data line (SDA) pin (BIT0 to BIT7)
#define SDA_PIN          BIT1
//! \def P_SDA_OUT
//! \brief The PxOUT register that the serial data line pin is on
#define P_SDA_OUT        P1OUT
//! \def P_SDA_DIR
//! \brief The PxDIR register that the serial data line pin is on
#define P_SDA_DIR        P1DIR
//! \def SDA_PIN
//! \brief The pin number of the serial data line (SDA) pin (BIT0 to BIT7)
#define SDA_PIN          BIT1
//! \def P_SDA_OUT
//! \brief The PxOUT register that the serial data line pin is on
#define P_SDA_OUT        P1OUT
//! \def P_SDA_IN
//! \brief The PxIN register that the serial data line pin is on
#define P_SDA_IN			P1IN
//! \def P_SDA_DIR
//! \brief The PxDIR register that the serial data line pin is on
#define P_SDA_DIR    	P1DIR
//! \def P_SDA_IES
//! \brief The PxIES register of the SDA pin
#define P_SDA_IES       P1IES
//! \def P_SDA_IFG
//! \brief The PxIFG register of the SDA pin
#define P_SDA_IFG       P1IFG
//! \def P_SDA_IE
//! \brief The PxIE register of the SDA pin
#define P_SDA_IE        P1IE

// Defines for the SCL pin (currently 2.2)
//! \def SCL_PIN
//! \brief The pin number of the serial clock line (SCL) pin (BIT0 to BIT7)
#define SCL_PIN          BIT2
//! \def P_SCL_DIR
//! \brief The PxDIR register of the SCL pin
#define P_SCL_DIR        P2DIR
//! \def P_SCL_IN
//! \brief The PxIN register of the SCL pin
#define P_SCL_IN         P2IN
//! \def P_SCL_IES
//! \brief The PxIES register of the SCL pin
#define P_SCL_IES        P2IES
//! \def P_SCL_IFG
//! \brief The PxIFG register of the SCL pin
#define P_SCL_IFG        P2IFG
//! \def P_SCL_IE
//! \brief The PxIE register of the SCL pin
#define P_SCL_IE         P2IE

// Define for the Interrupt pin (currently 2.0)
//! \def INT_PIN
//! \brief The pin number of the INT pin (BIT0 to BIT7)
#define INT_PIN          BIT0
//! \def P_INT_OUT
//! \brief The PxOUT register of the interrupt pin
#define P_INT_OUT        P2OUT
//! \def P_INT_DIR
//! \brief The PxDIR register of the interrupt pin
#define P_INT_DIR        P2DIR
//! \def P_INT_IN
//! \brief The PxIN register of the interrupt pin
#define P_INT_IN         P2IN
//! \def P_INT_IES
//! \brief The PxIES register of the interrupt pin
#define P_INT_IES        P2IES
//! \def P_INT_IFG
//! \brief The PxIFG register of the interrupt pin
#define P_INT_IFG        P2IFG
//! \def P_INT_IE
//! \brief The PxIE register of the interrupt pin
#define P_INT_IE         P2IE
//! @}

//! \name Status Flags
//! These are bit defines that are used to set and clear the
//! g_ucCOMM_Flags register.
//! @{
//! \def COMM_RUNNING
//! \brief Bit define - Indicates UART is running
#define COMM_RUNNING 0x01
//! \def COMM_TX_BUSY
//! \brief Bit define - Indicates TX in progress
#define COMM_TX_BUSY 0x02
//! \def COMM_RX_BUSY
//! \brief Bit define - Indicates RX in progress
#define COMM_RX_BUSY 0x04
//! \def COMM_PARITY_ERR
//! \brief Bit define - Indicates a parity bit failure
#define COMM_PARITY_ERR 0x08
//! \def COMM_START_CONDITION
//! \brief Bit define - Indicates a start bit has been received
#define COMM_START_CONDITION 0x10
//! @}

//! \name Communication Flags
//! These are flags are used to pass information between CP and SP in the flags byte
//! @{
#define SHUTDOWN_BIT		0x01
//! @}

//! \def INT_PIN
//! \brief The pin number of the INT pin (BIT0 to BIT7)
#define INT_PIN          BIT0
//! \def P_INT_DIR
//! \brief The PxDIR register of the interrupt pin
#define P_INT_DIR        P2DIR
//! \def P_INT_OUT
//! \brief The PxOUT register of the interrupt pin
#define P_INT_OUT        P2OUT
//! \def P_INT_IN
//! \brief The PxIN register of the interrupt pin
#define P_INT_IN         P2IN
//! \def P_INT_IES
//! \brief The PxIES register of the interrupt pin
#define P_INT_IES        P2IES
//! \def P_INT_IFG
//! \brief The PxIFG register of the interrupt pin
#define P_INT_IFG        P2IFG
//! \def P_INT_IE
//! \brief The PxIE register of the interrupt pin
#define P_INT_IE         P2IE

// Baud rate defines
//! @name Baud Rate Defines
//! These values are computed for a 4MHz SMCLK to produce the corresponding
//! baud rates
//! @{
//! \def BAUD_460800
//! \brief Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_460800 0x0008
//! \def BAUD_345600
//! \brief Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_345600 0x000B
//! \def BAUD_230400
//! \brief Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_230400 0x0011
//! \def BAUD_115200
//! \brief Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_115200 0x0023
//! \def BAUD_57600
//! Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_57600  0x0045
//! \def BAUD_19200
//! Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_19200  0x00D0
//! \def BAUD_9600
//! Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_9600   0x01A0
//! \def BAUD_1200
//! Timer count for specific UART data rate, computed for 4Mhz SMCLK
#define BAUD_1200   0x0D05 //3333
//! @}

//******************  Baud Rate Delays  *************************************//
//! @name Baud Rate Start Delays
//! These values are used to delay from the start bit to the middle of the
//! first data bit. Computed for a 4MHz SMCLK.  While the baud rates for
//! 230400 values have been included in the definitions it has been shown that
//! the MCU at 16 MHz cannot support these speeds.
//! @{
//! \def BAUD_460800_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_460800_DELAY  0x0004
//! \def BAUD_345600_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_345600_DELAY  0x0006
//! \def BAUD_230400_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_230400_DELAY  0x0007
//! \def BAUD_115200_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_115200_DELAY  BAUD_115200 + BAUD_115200/2 - 37
//! \def BAUD_57600_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_57600_DELAY   BAUD_57600 + BAUD_57600/2 - 36
//! \def BAUD_19200_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_19200_DELAY   BAUD_19200 + BAUD_19200/2 - 36
//! \def BAUD_9600_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_9600_DELAY    BAUD_9600 + BAUD_9600/2 - 36
//! \def BAUD_1200_DELAY
//! \brief Timer count for specific data rate delay, computed for 4Mhz SMCLK.
#define BAUD_1200_DELAY    0x0682 //1666
//! @}

//! \name Return Codes
//! Possible return codes from the \ref comm functions
//! @{
//! \def COMM_OK
//! \brief Function return code
#define COMM_OK               0x00
//! \def COMM_BUFFER_UNDERFLOW
//! \brief user has tried to pull more data that is available from g_ucaRXBuffer
#define COMM_BUFFER_UNDERFLOW 0x01
//! \def COMM_BUFFER_OVERFLOW
//! \brief The RX buffer contains too many bytes
#define COMM_BUFFER_OVERFLOW 	0x02
//! \def COMM_ERROR
//! \brief Indicates a general communication error
#define COMM_ERROR						0x04
//! \def COMM_ACK_ERR
//! \brief Indicates that the ack bit was not received
#define COMM_ACK_ERR					0x10
//! @}

// Comm.c function prototypes
//! @name Control Functions
//! These functions handle controlling the \ref comm Module.
//! @{
void vCOMM_Init(void);
void vCOMM_Shutdown(void);
uint8 ucCOMM_WaitForMessage(void);
//! @}

//! @name Transmit Functions
//! These functions transmit information on the \ref comm Module.
//! @{
uint8 ucCOMM_SendByte(uint8 ucChar);
void vCOMM_SendMessage(volatile uint8 * pBuff, uint8 ucLength);
//! @}

//! @name Receive Functions
//! These functions take information from the \ref comm Module and format
//! it appropriately.
//! @{
uint8 ucCOMM_WaitForStartCondition(void);
uint8 ucCOMM_ReceiveByte(void);
uint8 ucCOMM_GrabMessageFromBuffer(volatile uint8 * pucBuff);
//! @}

//! @name Interrupt Handlers
//! These are the interrupt handlers used by the \ref comm Module.
//! @{
__interrupt void PORT2_ISR(void);
__interrupt void TIMERA0_ISR(void);
//! @}

#endif /*COMM_H_*/
//! @}
//! @}
