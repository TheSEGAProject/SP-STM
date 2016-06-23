///////////////////////////////////////////////////////////////////////////////
//! \file FiveTM.h
//! \brief This is the h file for controlling the STM interface on an SP-STM
//! board
//!
//!
//!
//! @addtogroup
//! @{
//!
//! Written by Samuel Boegli 2012
///////////////////////////////////////////////////////////////////////////////


#ifndef FiveTM_H_
#define FiveTM_H_

#define NUM_1_STM_ON	1
#define NUM_2_STM_ON	1
#define NUM_3_STM_ON	1
#define NUM_4_STM_ON	1

//******************  STM Com Variables  *****************************************//
//! @name STM Com Variables
//! There variables are used in the receiving of data from the STM
//! @{

//! \def cSTM_1_RX_PIN
//! \brief The RX Pin for STM 1
#define cSTM_1_RX_PIN       BIT3

//! \def cSTM_2_RX_PIN
//! \brief The RX Pin for STM 2
#define cSTM_2_RX_PIN       BIT4

//! \def cSTM_3_RX_PIN
//! \brief The RX Pin for STM 3
#define cSTM_3_RX_PIN       BIT5

//! \def cSTM_4_RX_PIN
//! \brief The RX Pin for STM 4
#define cSTM_4_RX_PIN       BIT6

//! \def P_STM_RX_DIR
//! \brief The PxDIR register of the RX pin
#define P_STM_RX_DIR       P1DIR
//! \def P_STM_RX_IN
//! \brief The PxIN register of the RX pin
#define P_STM_RX_IN        P1IN
//! \def P_RX_IES
//! \brief The PxIES register of the RX pin
#define P_STM_RX_IES       P1IES
//! \def P_RX_IFG
//! \brief The PxIFG register of the RX pin
#define P_STM_RX_IFG       P1IFG
//! \def P_RX_IE
//! \brief The PxIE register of the RX pin
#define P_STM_RX_IE        P1IE
//! @}

//******************  STM Power Variables  *****************************************//
//! @name STM Power Variables
//! There variables are used in the powering of the STM
//! @{

//! \def cSTM_1_PWR_PIN
//! \brief The Pin for powering the STM1
#define cSTM_1_PWR_PIN       BIT0

//! \def cSTM_2_PWR_PIN
//! \brief The Pin for powering the STM2
#define cSTM_2_PWR_PIN       BIT1

//! \def cSTM_3_PWR_PIN
//! \brief The Pin for powering the STM3
#define cSTM_3_PWR_PIN       BIT2

//! \def cSTM_4_PWR_PIN
//! \brief The Pin for powering the STM4
#define cSTM_4_PWR_PIN       BIT3

//! \def P_STM_PWR_DIR
//! \brief The PxDIR register of the STM Power
#define P_STM_PWR_DIR       P3DIR
//! \def P_STM_PWR_OUT
//! \brief The PxOUT register of the STMK Power
#define P_STM_PWR_OUT        P3OUT
//! @}

#define RX_BUFFER_SIZE_STM 	   20

//! \def STM_ERROR_CODE_1
//! \brief The Checksum didn't work out...
#define STM_ERROR_CODE_1		0x01

//! \def STM_ERROR_CODE_2
//! \brief Timed out without a response
#define STM_ERROR_CODE_2		0x02


void vSTM_Initialize(void);
char cSTM_Measure(uint8 ucChannel);

void vSTM_Display(char);
char cSTM_Test_Checksum(void);
void cSTM_ReadValue(void);

uint8 cSTM_RequestSensorType(uint8 ucChannel);
uint8 cSTM_ReturnSensorType(uint8 ucChannel);

signed long lSTM_GetSoil(void);
signed int iSTM_GetTemp(void);

//!Interrupt Handler
__interrupt void TIMERB1_ISR(void);
__interrupt void PORT1_ISR(void);
#endif /* FiveTM_H_ */
