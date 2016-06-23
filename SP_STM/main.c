//*****************************************************************************
// main.c
//
// By: Samuel Boegli
//     Wireless Networks Research Lab
//     Dept of Electrical Engineering, CEFNS
//     Northern Arizona University
//
// Edited by CP
// SP-STM Main
//*****************************************************************************

#include <msp430x23x.h>
#include "core.h"


char cSTM_Initialized = 0;

//! @name Transducer Labels
//! The labels for each transducer are set in constants here.
//! Each constant must be 16 characters long.
//! @{
#define TRANSDUCER_0_LABEL_TXT "Test Function   " //00
#define TRANSDUCER_1_LABEL_TXT "STM1            " //01
#define TRANSDUCER_2_LABEL_TXT "STM2            " //02
#define TRANSDUCER_3_LABEL_TXT "STM3            " //03
#define TRANSDUCER_4_LABEL_TXT "STM4            " //04
//!@}

//! \def TRANSDUCER_0
//! \brief Transducer 0 index definition
#define TRANSDUCER_0   		0x00
//! \def TRANSDUCER_1
//! \brief Transducer 1 index definition
#define TRANSDUCER_1      0x01
//! \def TRANSDUCER_2
//! \brief Transducer 2 index definition
#define TRANSDUCER_2      0x02
//! \def TRANSDUCER_3
//! \brief Transducer 3 index definition
#define TRANSDUCER_3      0x03
//! \def TRANSDUCER_4
//! \brief Transducer 4 index definition
#define TRANSDUCER_4      0x04

//! @name SP Board configuration data
//!
//! This information allows the CP board to learn about the sensors attached to the SP board
//! and figure out how to schedule tasks dynamically
//! @{
//! \def NUM_TRANSDUCERS
//! \brief The number of transducers the SP board can have attached
#define NUM_TRANSDUCERS	4
//! \def TYPE_IS_SENSOR
//! \brief The transducer type definition for a sensor
#define TYPE_IS_SENSOR			0x53 //ascii S
//! \def TYPE_IS_ACTUATOR
//! \brief The transducer type definition for an actuator
#define TYPE_IS_ACTUATOR	0x41 //ascii A
//!@}

//! @name SP Board data structure
//! @{
//! \def NUMDATGEN
//! \brief The number of data generating elements on this board 2 per sensor plus one for the diagnostics
#define NUMDATGEN		0x09

//! \def MAXDATALEN
//! \brief This is the maximum length of a sensor reading for this board in bytes
#define MAXDATALEN	0x04

//! \def F_NEWDATA
//! \brief Flag indicating that new data is loaded into the S_Report structure
#define F_NEWDATA		0x01

//! \struct S_Report
//! \brief Customizable struct provides a generalized interface between data generators and the core
struct{
		uint8 m_ucaData[MAXDATALEN];	//!< Holds information from a data generator
		uint8 m_ucLength;							//!< Length of the data in the m_ucaData array (in bytes)
		uint8 m_ucFlags;							//!< Flags
}S_Report[NUMDATGEN];
//! @}

//! \var g_iVLOCal
//! \brief Calibration constant for the VLO.
//!This constant is the number of ticks required calibrate the VLO based on the typical frequency of 12000
signed int g_iVLOCal;

//! \var g_ucEventTrigger
//! \brief Flag indicating that an application specific event has occured and requires handling
unsigned char g_ucEventTrigger;

///////////////////////////////////////////////////////////////////////////////
//! \fn vMain_CalibrateVLO
//! \brief Generates a calibration constant for the VLO
//!
//! The SP-CM-STM board uses the VLO to time the duration the valve is on while
//! allowing the board to remain in LPM3
//!
//! The VLO frequency should be 12 kHz, but it can range from 4 kHZ to 20 kHz.
//! Therefore, we must determine a calibration constant before it can be used.
//! Two timers, timer A sourced from the SMCLK and timer B from the VLO (through ACLK)
//! run for 8.333 mS.  TBR should equal 100 ticks assuming the frequency of
//! the VLO is 12 kHz.  The deviation from 100 is measured and scaled up to the deviation
//! in ticks the VLO is from a full second of 12000 ticks.
//!
///////////////////////////////////////////////////////////////////////////////
void vMain_CalibrateVLO(void){

	// Set ACLK divider to 1
	BCSCTL1 &= ~DIVA_3;

	// Determine VLO calibration constant to improve accuracy
	TACTL = (TASSEL_2 | TACLR | ID_2);
	TACCR0 = 8333;
	TBCTL = (TBSSEL_1 | TBCLR );
	TACTL |= MC_1;
	TBCTL |= MC_2;
	while((TACCTL0 & CCIFG) == 0);

	// Set the global cal constant
	g_iVLOCal = (signed int)(TBR - 100)*120;

	TACTL = TACLR;
	TBCTL = TBCLR;
	TACCR0 = 0;

	// Set ACLK divider back to 4
	BCSCTL1 |= DIVA_2;

}

///////////////////////////////////////////////////////////////////////////////////////////////////
//!
//! \brief Used as a test function
//!
//! gets passed the data to pack data in for the core. Returns a uint16 which is a data message
//!
//! \param data	Core variables to send back the data in
//!
//! 0000
///////////////////////////////////////////////////////////////////////////////////////////////////
uint16 uiMain_Test(uint8 *ucParam)
{
	S_Report[0].m_ucaData[0] = 0xBE;
	S_Report[0].m_ucaData[1] = 0xEF;
	S_Report[0].m_ucLength = 2;
	S_Report[0].m_ucFlags = F_NEWDATA;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Handle for when Transducer 1 is called
//!
//!   \param *ucParam, parameters if required
//!   \return 1: success, 0: failure
///////////////////////////////////////////////////////////////////////////////
uint16 uiMain_STM1(uint8 *ucParam)
{
	int32 lSoil;
	int16 iTemperature;
	uint8 cResult;

	if (!cSTM_Initialized)
	{
		vSTM_Initialize();
		cSTM_Initialized = 1;
	}

	cResult = cSTM_Measure(1);

	// Write information to the S_Report structure
	if (cResult == 0)
	{
		// Get the soil moisture and soil temperature readings
		lSoil = lSTM_GetSoil();
		iTemperature = iSTM_GetTemp();

		// Start the report length at the maximum
		S_Report[1].m_ucLength = 4;

		// To avoid sending zeros at the high end of the ulSoil variable we check for the the first non-zero byte
		if ((lSoil & 0xFF800000) == 0)
		{
			S_Report[1].m_ucLength--;
			if ((lSoil & 0x00FF8000) == 0)
			{
				S_Report[1].m_ucLength--;
				if ((lSoil & 0x0000FF80) == 0)
					S_Report[1].m_ucLength--;
			}
		}
		// Left shift the data over so that leading zeros are removed
		lSoil = lSoil << (8 * (4-S_Report[1].m_ucLength));

		S_Report[1].m_ucaData[0] = (uint8) (lSoil >> 24);
		S_Report[1].m_ucaData[1] = (uint8) (lSoil >> 16);
		S_Report[1].m_ucaData[2] = (uint8) (lSoil >> 8);
		S_Report[1].m_ucaData[3] = (uint8) lSoil;

		S_Report[2].m_ucaData[0] = (uint8) (iTemperature >> 8);
		S_Report[2].m_ucaData[1] = (uint8) iTemperature;
		S_Report[2].m_ucLength = 2;
	}
	else if (cResult == 1)// Checksum fail
	{
		S_Report[1].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[1].m_ucLength = 1;

		S_Report[2].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[2].m_ucLength = 1;
	}
	else if (cResult == 2)//Timeout
	{
		S_Report[1].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[1].m_ucLength = 1;

		S_Report[2].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[2].m_ucLength = 1;
	}

	// Set the flags indicating there is data to report
	S_Report[1].m_ucFlags = F_NEWDATA;
	S_Report[2].m_ucFlags = F_NEWDATA;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Handle for when Transducer 2 is called
//!
//!   \param *ucParam, parameters if required
//!   \return 1: success, 0: failure
///////////////////////////////////////////////////////////////////////////////
uint16 uiMain_STM2(uint8 *ucParam)
{
	int32 lSoil;
	int16 iTemperature;
	uint8 cResult;

	if (!cSTM_Initialized)
	{
		vSTM_Initialize();
		cSTM_Initialized = 1;
	}

	cResult = cSTM_Measure(2);

	// Write information to the S_Report structure
	if (cResult == 0)
	{
		// Get the soil moisture and soil temperature readings
		lSoil = lSTM_GetSoil();
		iTemperature = iSTM_GetTemp();

		// Start the report length at the maximum
		S_Report[3].m_ucLength = 4;

		// To avoid sending zeros at the high end of the ulSoil variable we check for the the first non-zero byte
		if ((lSoil & 0xFF800000) == 0)
		{
			S_Report[3].m_ucLength--;
			if ((lSoil & 0x00FF8000) == 0)
			{
				S_Report[3].m_ucLength--;
				if ((lSoil & 0x0000FF80) == 0)
					S_Report[3].m_ucLength--;
			}
		}
		// Left shift the data over so that leading zeros are removed
		lSoil = lSoil << (8 * (4-S_Report[3].m_ucLength));

		S_Report[3].m_ucaData[0] = (uint8) (lSoil >> 24);
		S_Report[3].m_ucaData[1] = (uint8) (lSoil >> 16);
		S_Report[3].m_ucaData[2] = (uint8) (lSoil >> 8);
		S_Report[3].m_ucaData[3] = (uint8) lSoil;

		S_Report[4].m_ucaData[0] = (uint8) (iTemperature >> 8);
		S_Report[4].m_ucaData[1] = (uint8) iTemperature;
		S_Report[4].m_ucLength = 2;

	}
	else if (cResult == 1)// Checksum fail
	{
		S_Report[3].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[3].m_ucLength = 1;

		S_Report[4].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[4].m_ucLength = 1;

	}
	else if (cResult == 2)//Timeout
	{
		S_Report[3].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[3].m_ucLength = 1;

		S_Report[4].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[4].m_ucLength = 1;
	}

	S_Report[3].m_ucFlags = F_NEWDATA;
	S_Report[4].m_ucFlags = F_NEWDATA;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Handle for when Transducer 3 is called
//!
//!
//!   \param *ucParam, parameters if require
//!   \return 1: success, 0: failure
///////////////////////////////////////////////////////////////////////////////
uint16 uiMain_STM3(uint8 *ucParam)
{
	int32 lSoil;
	int16 iTemperature;
	uint8 cResult;

	if (!cSTM_Initialized)
	{
		vSTM_Initialize();
		cSTM_Initialized = 1;
	}

	cResult =cSTM_Measure(3);

	// Write information to the S_Report structure
	if (cResult == 0)
	{
		// Get the soil moisture and soil temperature readings
		lSoil = lSTM_GetSoil();
		iTemperature = iSTM_GetTemp();

		// Start the report length at the maximum
		S_Report[5].m_ucLength = 4;

		// To avoid sending zeros at the high end of the ulSoil variable we check for the the first non-zero byte
		if ((lSoil & 0xFF800000) == 0)
		{
			S_Report[5].m_ucLength--;
			if ((lSoil & 0x00FF8000) == 0)
			{
				S_Report[5].m_ucLength--;
				if ((lSoil & 0x0000FF80) == 0)
					S_Report[5].m_ucLength--;
			}
		}
		// Left shift the data over so that leading zeros are removed
		lSoil = lSoil << (8 * (4-S_Report[5].m_ucLength));

		S_Report[5].m_ucaData[0] = (uint8) (lSoil >> 24);
		S_Report[5].m_ucaData[1] = (uint8) (lSoil >> 16);
		S_Report[5].m_ucaData[2] = (uint8) (lSoil >> 8);
		S_Report[5].m_ucaData[3] = (uint8) lSoil;

		S_Report[6].m_ucaData[0] = (uint8) (iTemperature >> 8);
		S_Report[6].m_ucaData[1] = (uint8) iTemperature;
		S_Report[6].m_ucLength = 2;

	}
	else if (cResult == 1)// Checksum fail
	{
		S_Report[5].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[5].m_ucLength = 1;

		S_Report[6].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[6].m_ucLength = 1;
	}
	else if (cResult == 2)//Timeout
	{
		S_Report[5].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[5].m_ucLength = 1;

		S_Report[6].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[6].m_ucLength = 1;
	}

	S_Report[5].m_ucFlags = F_NEWDATA;
	S_Report[6].m_ucFlags = F_NEWDATA;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief Handle for when Transducer 4 is called
//!
//!
//!   \param *ucParam, parameters if require
//!   \return 1: success, 0: failure
///////////////////////////////////////////////////////////////////////////////
uint16 uiMain_STM4(uint8 *ucParam)
{
	int32 lSoil;
	int16 iTemperature;
	uint8 cResult;

	if (!cSTM_Initialized)
	{
		vSTM_Initialize();
		cSTM_Initialized = 1;
	}

	cResult = cSTM_Measure(4);

	if (cResult == 0)
	{
		// Get the soil moisture and soil temperature readings
		lSoil = lSTM_GetSoil();
		iTemperature = iSTM_GetTemp();

		// Start the report length at the maximum
		S_Report[7].m_ucLength = 4;

		// To avoid sending zeros at the high end of the ulSoil variable we check for the the first non-zero byte
		if ((lSoil & 0xFF800000) == 0)
		{
			S_Report[7].m_ucLength--;
			if ((lSoil & 0x00FF8000) == 0)
			{
				S_Report[7].m_ucLength--;
				if ((lSoil & 0x0000FF80) == 0)
					S_Report[7].m_ucLength--;
			}
		}
		// Left shift the data over so that leading zeros are removed
		lSoil = lSoil << (8 * (4-S_Report[7].m_ucLength));

		S_Report[7].m_ucaData[0] = (uint8) (lSoil >> 24);
		S_Report[7].m_ucaData[1] = (uint8) (lSoil >> 16);
		S_Report[7].m_ucaData[2] = (uint8) (lSoil >> 8);
		S_Report[7].m_ucaData[3] = (uint8) lSoil;

		S_Report[8].m_ucaData[0] = (uint8) (iTemperature >> 8);
		S_Report[8].m_ucaData[1] = (uint8) iTemperature;
		S_Report[8].m_ucLength = 2;

	}
	else if (cResult == 1)// Checksum fail
	{

		S_Report[7].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[7].m_ucLength = 1;

		S_Report[8].m_ucaData[0] = STM_ERROR_CODE_1;
		S_Report[8].m_ucLength = 1;
	}
	else if (cResult == 2)//Timeout
	{
		S_Report[7].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[7].m_ucLength = 1;

		S_Report[8].m_ucaData[0] = STM_ERROR_CODE_2;
		S_Report[8].m_ucLength = 1;
	}

	S_Report[7].m_ucFlags = F_NEWDATA;
	S_Report[8].m_ucFlags = F_NEWDATA;

	return 0;
}


///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Initializes the data storage structure
//!
//! \param none
//! \return none
///////////////////////////////////////////////////////////////////////////////
void vMain_CleanDataStruct(void)
{
	uint8 ucDataGenCnt;
	uint8 ucByteCnt;

	// Clean out the data storage structure
	for (ucDataGenCnt = 0; ucDataGenCnt < NUMDATGEN; ucDataGenCnt++) {
		S_Report[ucDataGenCnt].m_ucFlags = 0;
		S_Report[ucDataGenCnt].m_ucLength = 0;

		for (ucByteCnt = 0; ucByteCnt < MAXDATALEN; ucByteCnt++) {
			S_Report[ucDataGenCnt].m_ucaData[ucByteCnt] = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Loads the passed buffer with data stored in the S_Report structure
//!
//! \param *pBuff
//! \return ucLength, the amount of bytes added to the passed buffer
///////////////////////////////////////////////////////////////////////////////
uint8 ucMain_FetchData(volatile uint8 * pucBuff)
{
	uint8 ucDataGenCnt;
	uint8 ucByteCnt;
	uint8 ucLength;

	// Assume no data
	ucLength = 0;

	// Check all the data generators for new data
	for (ucDataGenCnt = 0; ucDataGenCnt < NUMDATGEN; ucDataGenCnt++)
	{
		// If there is new data to report then write to the passed buffer
		if (S_Report[ucDataGenCnt].m_ucFlags & F_NEWDATA)
		{
			// write the data generator ID and the length of this message
			*pucBuff++ = ucDataGenCnt;
			*pucBuff++ = S_Report[ucDataGenCnt].m_ucLength;

			// write the data
			for (ucByteCnt = 0; ucByteCnt < S_Report[ucDataGenCnt].m_ucLength; ucByteCnt++)
			{
				*pucBuff++ = S_Report[ucDataGenCnt].m_ucaData[ucByteCnt];
			}

			// Update the length variable. Includes raw data, data generator ID, and length byte lengths
			ucLength += (S_Report[ucDataGenCnt].m_ucLength + 2);
		}
	}

return ucLength;
}


///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Fetches the requested transducer label and writes it to the passed array
//!
//!	\param ucTransNum, the desired transducer; *pucLabelArray, pointer to the label array
//! \return none
///////////////////////////////////////////////////////////////////////////////
void vMain_FetchLabel(uint8 ucTransNum, volatile uint8 * pucLabelArray)
{
	uint8 ucLoopCount;

	switch (ucTransNum)
	{
		// For each transducer, use the table to get the label
		case TRANSDUCER_0:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = TRANSDUCER_0_LABEL_TXT[ucLoopCount];
		break;
		case TRANSDUCER_1:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = TRANSDUCER_1_LABEL_TXT[ucLoopCount];
		break;

		case TRANSDUCER_2:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = TRANSDUCER_2_LABEL_TXT[ucLoopCount];
		break;

		case TRANSDUCER_3:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = TRANSDUCER_3_LABEL_TXT[ucLoopCount];
		break;

		case TRANSDUCER_4:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = TRANSDUCER_4_LABEL_TXT[ucLoopCount];
		break;
		default:
			for (ucLoopCount = 0x00; ucLoopCount < TRANSDUCER_LABEL_LEN; ucLoopCount++)
				*pucLabelArray++ = "CANNOT COMPUTE!!"[ucLoopCount];
		break;

	} // END: switch(ucTransNum)

}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief invokes application specific function requesting sensor types
//!
//! This function is invoked from the core for uniformity sake - the function
//! which actually requests sensor types is called from this function because
//! the mechanism by which the types are gathered is application specific, and
//! therefore must be called outside of the core
//!
///////////////////////////////////////////////////////////////////////////////
void vMAIN_RequestSensorType(uint8 ucChannel)
{
	// tell the SP to gather sensor types
	cSTM_RequestSensorType(ucChannel);
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief invokes application specific function returning sensor types
//!
//! This function is invoked from the core for uniformity sake - the function
//! which actually returns sensor types is called from this function because
//! the mechanism by which the types are gathered is application specific, and
//! therefore must be called outside of the core
//!
///////////////////////////////////////////////////////////////////////////////
uint8 ucMAIN_ReturnSensorType(uint8 ucSensorCount)
{
	uint8 retVal = 0;
	retVal = cSTM_ReturnSensorType(ucSensorCount);

	return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the maximum number of sensors possible for this SP
//!
///////////////////////////////////////////////////////////////////////////////
uint8 ucMain_getNumTransducers(void)
{
	return NUM_TRANSDUCERS;
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the type of transducer (sensor vs. actuator)
//!
//! \param ucTransNum, the transducer number
///////////////////////////////////////////////////////////////////////////////
uint8 ucMain_getTransducerType(uint8 ucTransNum)
{
	uint8 ucRetVal;

	switch (ucTransNum)
	{
		case TRANSDUCER_1:
		case TRANSDUCER_2:
		case TRANSDUCER_3:
		case TRANSDUCER_4:
			ucRetVal = TYPE_IS_SENSOR; // For SP-STM board all the transducers are sensors
		break;

			// This is an error, we should not ever return 0
		default:
			ucRetVal = 0;
		break;
	}

	return ucRetVal;
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Returns the sample duration required for a sensor
//!
//! \param ucTransNum, the transducer number
///////////////////////////////////////////////////////////////////////////////
uint8 ucMain_getSampleDuration(uint8 ucTransNum)
{
	return 1;// For SP-STM board all the transducers use 1 second to sample
}

///////////////////////////////////////////////////////////////////////////////
//!
//! \brief Dispatches to the sensor functions
//!
//! This is called from the core.  Calling a function in the application layer
//! to dispatch to the sensor functions decouples the core since it does not
//! need to "know" anything about the number of transducers
//!
//! \param ucCmdTransNum, the transducer number; ucCmdParamLen, length of parameters
//! *ucParam, pointer to parameter array
//! \return
//!
///////////////////////////////////////////////////////////////////////////////
uint16 uiMainDispatch(uint8 ucCmdTransNum, uint8 ucCmdParamLen, uint8 *ucParam)
{
	uint8 ucRetVal;
	
	switch (ucCmdTransNum)
	{
		case 0:
			ucRetVal = uiMain_Test(ucParam);
		break;

		case 1:
			ucRetVal = uiMain_STM1(ucParam);
		break;

		case 2:
			ucRetVal = uiMain_STM2(ucParam);
		break;

		case 3:
			ucRetVal = uiMain_STM3(ucParam);
		break;

		case 4:
			ucRetVal = uiMain_STM4(ucParam);
		break;

		default:
			ucRetVal = 1;
		break;

	}

	return ucRetVal;
}

///////////////////////////////////////////////////////////////////////////////
//! \brief The handler for event triggered functions
//!
//! Most of the tasks performed by the SP boards are at the immediate request
//! of the CP board, but this is not always true.  This functions exists to handle the
//!	case that there is an event that triggers the SP to exit to core.  Therefore
//! the SP is capable of handling complex tasks while awaiting commands from the CP
//!
///////////////////////////////////////////////////////////////////////////////
void vMain_EventTrigger(void)
{

//	// If the turn off the valve flag is true then dispatch to the function
//	if (g_ucEventTrigger & some flag) {
//
//		// Clear the flag
//		g_ucEventTrigger &= ~some flag;
//
//		// Pull the interrrupt line high to tell the CP that we have data
//		P_INT_OUT |= INT_PIN;
//	}
}

///////////////////////////////////////////////////////////////////////////////
//! \fn ucMain_Shutdown
//!	\brief Checks to see if all processes are complete allowing the CP to cut power
//!
//! \return 1 if shutdown is OK
///////////////////////////////////////////////////////////////////////////////
uint8 ucMain_ShutdownAllowed(void){
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
//!   \brief The main file for the SP-ST SP Board
//!
//!   Initializes Core, and flags
//!	  Then runs vCORE_Run()
//!
//!   \param none
//!   \return never
///////////////////////////////////////////////////////////////////////////////


void main(void)
{
	// Initialize core
	vCORE_Initilize();

	// Clean the data storage structure
	vMain_CleanDataStruct();

	// Clear the event trigger flags
	g_ucEventTrigger = 0;

	//Run core
	vCORE_Run();
}
