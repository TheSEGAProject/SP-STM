///////////////////////////////////////////////////////////////////////////////
//! \file irupt.c
//! \brief This file holds the interrupt service routines
//!
//!
//!
//!
///////////////////////////////////////////////////////////////////////////////

#include "msp430.h"
#include "core.h"
#include "STM.h"
#include "comm.h"

/********************** externs ************************/
extern uint8 g_ucSTM_RXBusy;
extern char g_ucSTM_RXBitsLeft;
extern char cSTM_RX_Pin;
extern char g_ucaSTM_RXBuffer[RX_BUFFER_SIZE_STM];
extern volatile char g_ucSTM_RXBufferIndex;
extern char ISR;
extern char ADCISRindicator;
extern char timeoutcounter;
extern volatile uint8 g_ucCOMM_Flags;


///////////////////////////////////////////////////////////////////////////////
//!   \brief Second Interrupt Handler for TimerB (TimerB0 is the other)
//!
//!	  Timer is used to read the UART data from the sensor. Count down the bits
//!   until there are none left(Start, 8 bits, plus stop bit to make a byte,
//!   then finish. If more bytes are to be sent, a new IO interrupt will be
//!   called when the start bit comes. The byte index is incremented.
//!
//!   \param none
//!
//!   \return none
///////////////////////////////////////////////////////////////////////////////
#pragma vector=TIMERB1_VECTOR
__interrupt void TIMERB1_ISR(void)
{

	switch (__even_in_range(TBIV, 14))
	{
		case TBIV_NONE: /* No Interrupt pending */
			__no_operation();
		break;

		case TBIV_TBCCR1: /* TBCCR1_CCIFG */
			if (g_ucSTM_RXBusy)
			{
				TBCTL |= TBCLR; //Clear
				TBCTL |= MC1; //Continuous Mode

				switch (g_ucSTM_RXBitsLeft)
				{
					case 0x00:
						// There are no bits left, so lets reset all the values and stop timer
						P_STM_RX_IE |= cSTM_RX_Pin;
						P_STM_RX_IFG &= ~cSTM_RX_Pin;
						g_ucSTM_RXBufferIndex++;
						g_ucSTM_RXBusy = 0;
					break;

					case 0x01:
						if (P_STM_RX_IN & cSTM_RX_Pin)
							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] |= 0x80;
						else
							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] &= ~0x80;

					break;

					default:
						if (P_STM_RX_IN & cSTM_RX_Pin)
							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] |= 0x80;
						else
							g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] &= ~0x80;

						g_ucaSTM_RXBuffer[g_ucSTM_RXBufferIndex] >>= 1;
					break;
				}
			}

			g_ucSTM_RXBitsLeft--;
		break;

		case TBIV_TBCCR2: /* TBCCR2_CCIFG */
			__no_operation();
			__bic_SR_register_on_exit(LPM4_bits);
		break;

		case TBIV_3: /* Reserved */
		case TBIV_4: /* Reserved */
		case TBIV_5: /* Reserved */
		case TBIV_6: /* Reserved */
		break;

		case TBIV_TBIFG:
		break;

	}
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Port 1 interrupt service routine
//!
//! Used for both transducer debugging and bit-banging
//!
//!   \param none
//!   \return none
///////////////////////////////////////////////////////////////////////////////
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
	if (P1IFG & SDA_PIN) {
		P_SDA_IFG &= ~SDA_PIN;
		__bic_SR_register_on_exit(LPM4_bits);
	}

	if (P_STM_RX_IFG & cSTM_RX_Pin)
	{
		timeoutcounter = 0;
		// Delay for half bit, this ensures we start sampling at the middle of
		// each bit
		TBCTL &= ~(MC0 | MC1 | TBIE | TBIFG); //Halt timer, Disable interrupts
		TBCTL |= TBCLR; //Clear Timer
		TBCCTL1 &= ~CCIFG;
		TBCCTL1 &= ~CCIE; //Disable Compare interrupt
		TBCCTL0 &= ~CCIE; //Disable Compare interrupt
		TBCCR0 = BAUD_1200_DELAY; //Input delay time in compare register
		TBCTL |= MC_1; //Start timer in UP Mode (Counts to TBCCR0 then starts at 0 again)
		while (!(TBCTL & TBIFG)); //Wait until Compare register is met
		TBCTL &= ~TBIFG;
		// Enable timer interrupt, configure for baud rate
		TBCTL &= ~(MC0 | MC1); //Halt timer
		TBCTL |= TBCLR; //Clear
		TBCCTL1 |= CCIE;
		TBCCR0 = 0;
		TBCCR1 = BAUD_1200;
		__bic_SR_register_on_exit(LPM4_bits);
		//All Clocks and CPU etc awake now that we received a start bit.
		// Disable interrupt on RX
		P_STM_RX_IE &= ~cSTM_RX_Pin;
		P_STM_RX_IFG &= ~cSTM_RX_Pin;
		//*****************
		g_ucSTM_RXBitsLeft = 0x08;
		g_ucSTM_RXBusy = 1;
		//*****************
		TBCTL |= MC1; //Continuous Mode
		//Clear Interrupt Flag
		TBCCTL1 &= ~TBIFG; //IFG

	} //END if(P_STM_RX_IFG & cSTM_1_RX_PIN)//P2IFG & BIT4
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Port2 ISR, handles the start of the UART RX
//!
//! The idle state for UART is line high, thus when we get the falling edge
//! indicating the start bit, we can start the timer to handle the UART
//! sampling.
//!   \param None
//!   \return None
//!   \sa vCOMM_Init(), TIMERA0_ISR
///////////////////////////////////////////////////////////////////////////////
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
	// If the source of the interrupt was the RX pin
	if (P2IFG & SCL_PIN) {
		// If comm with the CP is running
		if (g_ucCOMM_Flags & COMM_RUNNING) {
			P_SCL_IFG &= ~SCL_PIN;
			__bic_SR_register_on_exit(LPM0_bits);
		}
	}

	// Wake up from LPM3/4 triggered by the CP to
	// allow the SP time to wake up before receiving a command
	if (P2IFG & INT_PIN) {
		P_INT_IFG &= ~INT_PIN;
		__bic_SR_register_on_exit(LPM4_bits);
	}

} //END __interrupt void PORT2_ISR(void)

#pragma vector=COMPARATORA_VECTOR
__interrupt void COMPARATORA_ISR(void)
{}

#pragma vector=NMI_VECTOR
__interrupt void NMI_ISR(void)
{}

#pragma vector=TIMERA1_VECTOR
__interrupt void TIMERA1_ISR(void)
{}
