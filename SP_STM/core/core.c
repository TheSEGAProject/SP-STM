///////////////////////////////////////////////////////////////////////////////
//! \file core.c
//! \brief This is the primary file for the SP Board core
//!
//! This file contains the heart of the SP Board core. The function pointer
//! and label tables are kept here and maintained by the core.
//!
//! @addtogroup core Core
//! The Core Module handles all of the communication to the CP board as well
//! as acts as the supervisor to all activities on the SP Board. The user
//! built wrapper should interface with the Core Module as documented.
//! @{
///////////////////////////////////////////////////////////////////////////////
//*****************************************************************************
// By: Kenji Yamamoto
//     Wireless Networks Research Lab
//     Dept of Electrical Engineering, CEFNS
//     Northern Arizona University
//
//	Modified by: Chris Porter
//
//*****************************************************************************

#include <msp430x23x.h>
#include "core.h"

//******************  Software version variables  ***************************//
//! @name Software Version Variables
//! These variables contain ID and version information.
//! @{
//! \var static const uint8 g_ucaCoreVersion[VERSION_LABEL_LEN]
//! \brief The name and version of the core
#define VERSION_LABEL "SP-Core v2.10   "
//! @}

//! \var uiHID
//! \brief Variable holds the unique SP ID as a byte array
uint16 uiHID[4];

//******************  Functions  ********************************************//
///////////////////////////////////////////////////////////////////////////////
//! \brief This function starts up the Core and configures hardware & RAM
//!
//! All of the clock setup and initial port configuration is handled in this
//! function. At the end of the function, before the return, any additional
//! core initilization functions are called.
//!   \param None.
//!   \return None.
///////////////////////////////////////////////////////////////////////////////
void vCORE_Initilize(void)
{
	// First, stop the watchdog
	WDTCTL = WDTPW + WDTHOLD;

	// Configure DCO for 16 MHz
	DCOCTL = CALDCO_16MHZ;
	BCSCTL1 = CALBC1_16MHZ;

	// Configure VLO
	BCSCTL3 = 0x00;

	// MCLK = DCO/1    SMCLK = DCO / 4
	BCSCTL2 = SELM_0 | DIVM_0 | DIVS_2;

	// ACLK = VLO / 4 = ~3 kHz
	BCSCTL3 |= LFXT1S_2;
	BCSCTL1 &= ~(XT2OFF + XTS); // = 0xC0

	// ACLK = VLO/4
	BCSCTL1 |= DIVA_2;

	// Configure the pins
	P1OUT = CoreP1OUT;
	P1DIR = CoreP1DIR;
	P1REN = CoreP1REN;
	P1SEL = CoreP1SEL;

	P2OUT = CoreP2OUT;
	P2DIR = CoreP2DIR;
	P2REN = CoreP2REN;
	P2SEL = CoreP2SEL;

	P3OUT = CoreP3OUT;
	P3DIR = CoreP3DIR;
	P3REN = CoreP3REN;
	P3SEL = CoreP3SEL;

	P4OUT = CoreP4OUT;
	P4DIR = CoreP4DIR;
	P4REN = CoreP4REN;
	P4SEL = CoreP4SEL;

	P5OUT = CoreP5OUT;
	P5DIR = CoreP5DIR;
	P5REN = CoreP5REN;
	P5SEL = CoreP5SEL;

	P6OUT = CoreP6OUT;
	P6DIR = CoreP6DIR;
	P6REN = CoreP6REN;
	P6SEL = CoreP6SEL;

	// All core modules get initialized now
	vCOMM_Init();

	// Get the SPs serial number from flash
	vFlash_GetHID(uiHID);

	// Enable interrupts
	__bis_SR_register(GIE);

}

///////////////////////////////////////////////////////////////////////////////
//! \brief Measure the MSP430 supply voltage
//!
//! Uses the ADC12 to measure the input voltage. Uses the MEM15 register.
//!
//!   \param none
//!
//!   \return unsigned int Input voltage * 100
///////////////////////////////////////////////////////////////////////////////

unsigned int unCORE_GetVoltage(void)
{
	int rt_volts;

	ADC12CTL0 &= ~(SHT10 + SHT12 + SHT13 + MSC + ADC12OVIE + ADC12TOVIE + ENC + ADC12SC); //ADC12CTL0 &= ~0xD08F = ~1101 0000 1000 1111 //Have to turn ENC Off first
	ADC12CTL0 |= (SHT11 + REF2_5V + REFON + ADC12ON); //ADC12CTL0 |= 0x2070 = 0010 xxxx 0111 00(11)* - 16-Cycle Hold time + Single Conversion + 2.5V Ref + RefON + ADC ON + Interrupts off + (Enable + Start)
	ADC12CTL1 &= ~(SHS1 + SHS0 + ISSH + ADC12DIV2 + ADC12DIV1 + ADC12DIV0 + ADC12SSEL1 + ADC12SSEL0 + CONSEQ1 + CONSEQ0); //ADC12CTL1 &= ~0x0FDE = ~0000 1101 1111 1110
	ADC12MEM15 = 0;
	ADC12MCTL15 |= (SREF0 + INCH3 + INCH1 + INCH0); //ADC12MCTL15 |= 0x1B = x001 1011 - Reference Select + Input Select
	ADC12MCTL15 &= ~(SREF2 + SREF1 + INCH2); // ADC12MCTL15 &= ~0x64 = 0110 0100
	ADC12IE &= ~0x8000; //Turn off IE and clear IFG
	ADC12IFG &= ~0x8000;

	__delay_cycles(1000);

	ADC12CTL1 |= (CSTARTADD3 + CSTARTADD2 + CSTARTADD1 + CSTARTADD0 + SHP); //ADC12CTL1 |= 0xF200 = 1111 0010 0000 000x - MEM15 + Internal OSC CLK + Single-Channel, Single-conversion
	ADC12CTL0 |= ENC + ADC12SC; // Sampling and conversion start

	while (!(ADC12IFG & 0x8000)); //End when something is written in. Can't sleep because we wanted to keep interrupts for users (not in core)

	rt_volts = ADC12MEM15; //(0.5*Vin)/2.5V * 4095
	ADC12IFG &= ~0x8000; //Unset IFG Flag
	ADC12CTL0 &= ~ENC;
	ADC12CTL0 &= ~(REFON + ADC12ON); // turn off A/D to save power

	rt_volts = (rt_volts * 5) / 41;
	return (rt_volts);
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Send the confirm packet
//!
//!  Confirm packet includes all the data received so CP Board can confirm it
//!  is correct.
//!
//!   \param none
//!   \sa core.h
///////////////////////////////////////////////////////////////////////////////
vCORE_Send_ConfirmPKT()
{
	uint8 ucaMsg_Buff[MAXMSGLEN];

	// Send confirm packet that we received message
	ucaMsg_Buff[MSG_TYP_IDX] = CONFIRM_COMMAND;
	ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE;
	ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;
	ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

	// Send the message
	vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
}

///////////////////////////////////////////////////////////////////////////////
//! \brief Send the confirm packet
//!
//!  Confirm packet includes all the data received so CP Board can confirm it
//!  is correct.
//!
//!   \param none
//!   \sa core.h
///////////////////////////////////////////////////////////////////////////////
void vCORE_Send_ErrorMsg(uint8 ucErrMsg)
{
	uint8 ucaMsg_Buff[MAXMSGLEN];

	// Send confirm packet that we received message
	ucaMsg_Buff[MSG_TYP_IDX] = REPORT_ERROR;
	ucaMsg_Buff[MSG_LEN_IDX] = 5;
	ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;
	ucaMsg_Buff[MSG_FLAGS_IDX] = 0;
	ucaMsg_Buff[MSG_PAYLD_IDX] = ucErrMsg;

	// Send the message
	vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
}

///////////////////////////////////////////////////////////////////////////////
//! \brief This functions runs the core
//!
//! This function runs the core. This function does not return, so all of the
//! core setup and init must be done before the call to this function. The
//! function waits for a data packet from the CP Board, then parses and handles
//! it appropriately. A response packet is then sent and the core waits for
//! the next data packet.
//!   \param None.
//!   \return NEVER. This function never returns
//!   \sa msg.h
///////////////////////////////////////////////////////////////////////////////
void vCORE_Run(void)
{
	uint16 unTransducerReturn; //The return parameter from the transducer function
	uint8 ucaMsg_Buff[MAXMSGLEN];
	uint8 ucMsgBuffIdx;
	uint8 ucTransIdx;
	uint8 ucCmdTransNum;
	uint8 ucCmdParamLen;
	uint8 ucParamCount;
	uint8 ucParam[20];
	uint8 ucCommState;

	// First, tell the CP Board that we are ready for commands
	ucaMsg_Buff[MSG_TYP_IDX] = ID_PKT;
	ucaMsg_Buff[MSG_LEN_IDX] = 12;
	ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;
	ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

	ucMsgBuffIdx = MSG_PAYLD_IDX;

	// The unique SP identification number
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[0];
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[0] >> 8);
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[1];
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[1] >> 8);
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[2];
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[2] >> 8);
	ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[3];
	ucaMsg_Buff[ucMsgBuffIdx] = (uint8) (uiHID[3] >> 8);

	if (unCORE_GetVoltage() < MIN_VOLTAGE)
	{
		ucMsgBuffIdx = MSG_PAYLD_IDX;

		ucaMsg_Buff[MSG_TYP_IDX] = REPORT_ERROR;
		ucaMsg_Buff[MSG_LEN_IDX] = 5;
		ucaMsg_Buff[ucMsgBuffIdx++] = 0xBA;
		ucaMsg_Buff[ucMsgBuffIdx] = 0xD1;
	}

	// Wait in deep sleep for the start of a message
	ucCOMM_WaitForStartCondition();

	// Send the message
	vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);

	// The primary execution loop
	while (TRUE)
	{
		// Wait in deep sleep for the start of a message
		// If we exit this function and it is not because of a start condition
		// then assume it was an event that triggered the wake up
		if (ucCOMM_WaitForStartCondition() != 1) {

			vMain_EventTrigger();
		}
		else {

			// Once we are awake, wait for a message from the CP
			ucCOMM_WaitForMessage();

			// Pull the message from the RX buffer and load it into a local buffer
			ucCommState = ucCOMM_GrabMessageFromBuffer(ucaMsg_Buff);

			if (ucCommState == COMM_OK) {
				//Switch based on the message type
				switch (ucaMsg_Buff[MSG_TYP_IDX])
				{
					case COMMAND_PKT:
						// Send a confirmation packet
					vCORE_Send_ConfirmPKT();

						unTransducerReturn = 0; //default return value to 0

						// Read through the length of the message and execute commands as they are read
						for (ucMsgBuffIdx = MSG_PAYLD_IDX; ucMsgBuffIdx < ucaMsg_Buff[MSG_LEN_IDX];) {
							// Get the transducer number and the parameter length
							ucCmdTransNum = ucaMsg_Buff[ucMsgBuffIdx++];
							ucCmdParamLen = ucaMsg_Buff[ucMsgBuffIdx++];

							for (ucParamCount = 0; ucParamCount < ucCmdParamLen; ucParamCount++) {
								ucParam[ucParamCount] = ucaMsg_Buff[ucMsgBuffIdx++];
							}

							// Dispatch to perform the task, pass all values needed to populate the data
							unTransducerReturn |= uiMainDispatch(ucCmdTransNum, ucCmdParamLen, ucParam);
						}
					break; //END COMMAND_PKT

					case REQUEST_DATA:
						// Stuff the header
						ucaMsg_Buff[MSG_TYP_IDX] = REPORT_DATA;

						//unTransducerArray is an 'OK' message.
						//If not = to 0 then error
						if (unTransducerReturn != 0)
							ucaMsg_Buff[MSG_TYP_IDX] = REPORT_ERROR;

						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						// Load the message buffer with data.  The fetch function returns length
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE + ucMain_FetchData(&ucaMsg_Buff[MSG_PAYLD_IDX]);

						// Send the message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);

					break; //END REQUEST_DATA

					case REQUEST_LABEL:
						// Format first part of return message
						ucaMsg_Buff[MSG_TYP_IDX] = REPORT_LABEL;
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE + TRANSDUCER_LABEL_LEN;
						ucaMsg_Buff[MSG_VER_IDX] = SP_LABELMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						// Make call to main for the trans. labels.  This way the core is not constrained to a fixed number of transducers
						vMain_FetchLabel(ucaMsg_Buff[MSG_PAYLD_IDX], &ucaMsg_Buff[MSG_PAYLD_IDX]);

						// Send the label message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
					break; //END REQUEST_LABEL

						//Report the BSL password to the CP
					case REQUEST_BSL_PW:

						// Stuff the header
						ucaMsg_Buff[MSG_TYP_IDX] = REQUEST_BSL_PW;
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE + BSLPWDLEN; // BSL password is 32 bytes long
						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						//go to the flash.c file to read the value in the 0xFFE0 to 0xFFFF
						vFlash_GetBSLPW(&ucaMsg_Buff[MSG_PAYLD_IDX]);

						//once the password is obtained send it to the CP
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
					break;

						// The CP requests sensor and board information from the SP
					case INTERROGATE:
						ucaMsg_Buff[MSG_TYP_IDX] = INTERROGATE;
						ucaMsg_Buff[MSG_LEN_IDX] = 2 * ucMain_getNumTransducers() + 13; // 2 bytes for each sensor + header and ID packet length
						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						ucMsgBuffIdx = MSG_PAYLD_IDX;
						ucaMsg_Buff[ucMsgBuffIdx++] = ucMain_getNumTransducers(); // Number of transducers attached

						// Loop through the number of sensors and fetch the sensor type and sample duration
						for (ucTransIdx = 1; ucTransIdx <= ucMain_getNumTransducers(); ucTransIdx++) {
							ucaMsg_Buff[ucMsgBuffIdx++] = ucMain_getTransducerType(ucTransIdx);
							ucaMsg_Buff[ucMsgBuffIdx++] = ucMain_getSampleDuration(ucTransIdx);
						}

						// Load the board name into the message buffer
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_HI_BYTE1;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_LO_BYTE1;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_HI_BYTE2;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_LO_BYTE2;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_HI_BYTE3;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_LO_BYTE3;
						ucaMsg_Buff[ucMsgBuffIdx++] = ID_PKT_HI_BYTE4;
						ucaMsg_Buff[ucMsgBuffIdx] = ID_PKT_LO_BYTE4;

						// Send the message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
					break;

					case SET_SERIALNUM:

						ucMsgBuffIdx = MSG_PAYLD_IDX;
						uiHID[0] = (uint16) ucaMsg_Buff[ucMsgBuffIdx++];
						uiHID[0] = uiHID[0] | (uint16) (ucaMsg_Buff[ucMsgBuffIdx++] << 8);

						uiHID[1] = (uint16) ucaMsg_Buff[ucMsgBuffIdx++];
						uiHID[1] = uiHID[1] | (uint16) (ucaMsg_Buff[ucMsgBuffIdx++] << 8);

						uiHID[2] = (uint16) ucaMsg_Buff[ucMsgBuffIdx++];
						uiHID[2] = uiHID[2] | (uint16) (ucaMsg_Buff[ucMsgBuffIdx++] << 8);

						uiHID[3] = (uint16) ucaMsg_Buff[ucMsgBuffIdx++];
						uiHID[3] = uiHID[3] | (uint16) (ucaMsg_Buff[ucMsgBuffIdx] << 8);

						// Write the message header assuming success
						ucaMsg_Buff[MSG_TYP_IDX] = SET_SERIALNUM;
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE + 8;
						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						// Write the new HID to flash
						if (ucFlash_SetHID(uiHID)) {
							// Report an error if the write was unsuccessful
							ucaMsg_Buff[MSG_TYP_IDX] = REPORT_ERROR;
							ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE;

						}
						else {
							// Get the SPs serial number from flash
							vFlash_GetHID(uiHID);

							ucMsgBuffIdx = MSG_PAYLD_IDX;

							// Write the new HID to the message buffer
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[0];
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[0] >> 8);
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[1];
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[1] >> 8);
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[2];
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) (uiHID[2] >> 8);
							ucaMsg_Buff[ucMsgBuffIdx++] = (uint8) uiHID[3];
							ucaMsg_Buff[ucMsgBuffIdx] = (uint8) (uiHID[3] >> 8);
						}

						// Send the message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);

					break;

						// The CP commands the sensor types to be retrieved
					case COMMAND_SENSOR_TYPE:
					{
						uint8 ucChannel;

						// loop through each channel, get sample, and assign type
						for (ucChannel = 1; ucChannel < 5; ucChannel++) {
							// command the retrieval of sensor type
							vMAIN_RequestSensorType(ucChannel);
						}
					}
					break;

						// The CP requests the sensor types from the SPs
					case REQUEST_SENSOR_TYPE:
					{
						uint8 retVal = 0;
						uint8 ucSensorTypes[4];
						uint8 ucSensorCount = 0;

						// Format first part of return message
						ucaMsg_Buff[MSG_TYP_IDX] = 0x0D;
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE + 2;
						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION;

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						// Loop through sensors and place types on msg buffer
						for (ucSensorCount = 1; ucSensorCount < 5; ucSensorCount++) {
							// get sensor type for a specific channel
							retVal = ucMAIN_ReturnSensorType(ucSensorCount);

							// otherwise, register type
							ucSensorTypes[ucSensorCount - 1] = retVal;
						}

						// put sensor types on buffer
						for (ucSensorCount = 0; ucSensorCount < 4; ucSensorCount++) {
							ucaMsg_Buff[ucSensorCount + SP_HEADERSIZE] = ucSensorTypes[ucSensorCount];
						}

						// Send the sensor types message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);
					}
					break;

					default:
						ucaMsg_Buff[MSG_TYP_IDX] = REPORT_ERROR;
						ucaMsg_Buff[MSG_LEN_IDX] = SP_HEADERSIZE;
						ucaMsg_Buff[MSG_VER_IDX] = SP_DATAMESSAGE_VERSION; //-scb

						if (ucMain_ShutdownAllowed() == 1)
							ucaMsg_Buff[MSG_FLAGS_IDX] |= SHUTDOWN_BIT;
						else
							ucaMsg_Buff[MSG_FLAGS_IDX] = 0;

						// Send the message
						vCOMM_SendMessage(ucaMsg_Buff, ucaMsg_Buff[MSG_LEN_IDX]);

					break; //END default
				} // END: switch(ucMsgType)
			}
			else {
				vCORE_Send_ErrorMsg(ucCommState);
			} // END: if((ucCommState == COMM_OK))
		} // END: else(event trigger)
	} // END: while(TRUE)
}

//! @}
