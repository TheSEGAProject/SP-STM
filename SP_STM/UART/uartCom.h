#if 0 // This file is not needed
///////////////////////////////////////////////////////////////////////////////
//! \file uartCom.h
//! \brief Facilitates the UART communication between the computer and MSP430
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




#ifndef UARTCOM_H_
#define UARTCOM_H_

//! @name UART Buffer Size
//!
//! @{
//!\def UART_BUFFERSIZE
//! \brief The size of the UART Buffer. Must include the return char.

#define UART_BUFFERSIZE 8
//! @}

//! @name UART Functions
//! These functions are used to control the Computer to SP UART.
//!
//! @{
void vUARTCOM_Initialize(void);
void vUARTCOM_TXString( char*, int);
unsigned char ucUARTCOM_getBufferFill(void);
unsigned char ucUARTCOM_LastIsReturn(void);
unsigned char cpUARTCOM_readBuffer(char *);
void vUART_DisplayVoltage(void);
void vUARTCOM_HandleUART(void);
//! @}

//! @}

#endif /* UARTCOM_H_ */
#endif
