///////////////////////////////////////////////////////////////////////////////

//! \file flash.c
//! \brief This modules is used to read and write to flash memory
//!
//!
//!
//! @addtogroup core
//! @{
//!

#include <msp430F235.h>
#include "core.h" // for data type definitions
#include "flash.h"

/*need to write a function that will unlock flash memory before programming and lock it after.  This can be incorporated in some other
 * set of functions since it is only a one liner and disable pw security and check program code are 2 functions that come before and after programming
 */

//////////////////////////vFlash_init()////////////////////////////////////
//! \brief check for calibration data and initialize the timing generator
//!
//! \param none
//! \return none
////////////////////////////////////////////////////////////////////////// 
void vFlash_init(void)
{
	if (CALBC1_16MHZ == 0xFF || CALDCO_16MHZ == 0xFF)
	{
		while (1);		// If calibration constants erased
									// do not load, trap CPU!!
					 	 	 	 	//this while(1) will be replaced with the vFlashcalDCO()

	}

	//SMCLK source, divider = 0x0A to get 400Khz Flash timing generator
	FCTL2 = FWKEY + FSSEL0 + (FN1 | FN3);

} //END: vFlash_init()

//////////////////////////vFlash_Write_Byte()////////////////////////////////////
//! \brief Writes the passed byte to flash in the provided address
//!
//! This function should not be called directly, it assumes vFlash_init() has already been called
//!
//! 
//! \param ucData
//! \return success
////////////////////////////////////////////////////////////////////////// 
static uint8 ucFlash_Write_Byte(uint8 ucData, uint16 unAddress)
{
	char *ucFlashPtr;
	//initialize the flash pointer to point to the given address
	ucFlashPtr = (char *) unAddress;

	//clear the lock bits
	FCTL3 = FWKEY;
	//Set the erase bit
	FCTL1 = FWKEY + ERASE;
	//set the write bit
	FCTL1 = FWKEY + WRT;

	//wait statements prevent writing to flash while module is busy
	while (!(FCTL3 & WAIT));
	*ucFlashPtr = ucData;
	while (!(FCTL3 & WAIT));

	//clear the write bit
	FCTL1 = FWKEY;
	//set the lock bit
	FCTL3 = FWKEY + LOCK;

	//if the operation failed report it to the calling function
	if ((FCTL3 & 0x80) == 0x9680)
	{
		return 1;
	}

	return 0;
} //END: ucFlash_Write_Byte()

//////////////////////////vFlash_Write_Byte()////////////////////////////////////
//! \brief Writes the passed byte to flash in the provided address
//!
//! This function should not be called directly, it assumes vFlash_init() has already been called
//!
//!
//! \param ucData
//! \return success
//////////////////////////////////////////////////////////////////////////
static uint8 ucFlash_Write_Int(uint16 uiData, uint16 uiAddress)
{
	uint16 *uiFlashPtr;
	//initialize the flash pointer to point to the given address
	uiFlashPtr = (uint16 *)uiAddress;

	//clear the lock bits
	FCTL3 = FWKEY;
	//Set the erase bit
	FCTL1 = FWKEY + ERASE;
	//set the write bit
	FCTL1 = FWKEY + WRT;

	//wait statements prevent writing to flash while module is busy
	while (!(FCTL3 & WAIT))
		;
	*uiFlashPtr = uiData;
	while (!(FCTL3 & WAIT))
		;

	//clear the write bit
	FCTL1 = FWKEY;
	//set the lock bit
	FCTL3 = FWKEY + LOCK;

	//if the operation failed report it to the calling function
	if ((FCTL3 & 0x80) == 0x9680)
	{
		return 1;
	}

	return 0;
} //END: ucFlash_Write_Byte()

//////////////////////////vFlash_Read_Byte()////////////////////////////////////
//! \brief Reads a byte from flash at the provided address
//!
//! This function should not be called directly, it assumes vFlash_init() has already been called
//! it also reads integers since odd addresses of flash cannot be read
//!
//! \param unAddress
//! \return ucData
/////////////////////////////////////////////////////////////////////////////
static uint16 uiFlash_Read_Int(uint16 unAddress)
{
	uint16 *unFlashPtr;
	uint16 unData;

	//initialize the flash pointer to point to the given address
	unFlashPtr = (uint16 *) unAddress;

	//clear the lock bits
	FCTL3 = FWKEY;
	//Set the erase bit
	FCTL1 = FWKEY + ERASE;

	//wait statements prevent writing to flash while module is busy
	while (!(FCTL3 & WAIT));
	unData = *unFlashPtr;
	while (!(FCTL3 & WAIT));

	//clear the write bit
	FCTL1 = FWKEY;
	//set the lock bit
	FCTL3 = FWKEY + LOCK;

	return (unData);

} //END: vFlash_Read_Int()

///////////////////////////////////////////////////////////////////////////
//! \brief Reads a segment from flash at the provided address
//!This function should not be called directly, it assumes vFlash_init() has already been called
//! \param unAddress
//! \return ucData
/////////////////////////////////////////////////////////////////////////////
void vFlash_Read_Segment(uint16 * uiData, uint16 uiAddress)
{
	uint16 *uiFlashPtr;;
	uint16 uiIndex;
	uint8 ucFlashSzInt;

	// Number of integers in flash memory
	ucFlashSzInt = INFO_SEGMENTLENGTH/2;

	//initialize the flash pointer to point to the given address
	uiFlashPtr = (uint16 *) uiAddress;

	//clear the lock bits
	FCTL3 = FWKEY;

	// Loop through the segment and read out the data
	for (uiIndex = 0; uiIndex < ucFlashSzInt; uiIndex++)
	{
		//wait statements prevent writing to flash while module is busy
		while (!(FCTL3 & WAIT));
		*uiData++ = *uiFlashPtr++;
	}

	//clear the write bit
	FCTL1 = FWKEY;
	//set the lock bit
	FCTL3 = FWKEY + LOCK;

} //END: vFlash_Read_Segment()

///////////////////////////////////////////////////////////////////////////
//! \brief Writes a segment of memory specified by the address.
//!
//!
//! \param ucData, ulAddress
//! \return success
//////////////////////////////////////////////////////////////////////////
static void vFlash_Write_Segment(uint16 * uiData, uint16 uiAddress)
{
	uint16 *uiFlashPtr;
	uint16 uiIndex;
	uint8 ucFlashSzInt;

	// Number of integers in flash memory
	ucFlashSzInt = INFO_SEGMENTLENGTH/2;

	//initialize the flash pointer to point to the given address
	uiFlashPtr = (uint16 *)uiAddress;

	//clear the lock bits
	FCTL3 = FWKEY;
	//Set the erase bit
	FCTL1 = FWKEY + ERASE;

	//set the write bit
	FCTL1 = FWKEY + WRT;

	// Loop through the segment and write the data
	for (uiIndex = 0; uiIndex < ucFlashSzInt; uiIndex++)
	{
		//wait statements prevent writing to flash while module is busy
		while (!(FCTL3 & WAIT));
		*uiFlashPtr++ = *uiData++;
	}

	//clear the write bit
	FCTL1 = FWKEY;
	//set the lock bit
	FCTL3 = FWKEY + LOCK;

} //END: ucFlash_Write_Segment()


////////////////////////// vFlash_Erase_Seg() ////////////////////////////////////
//! \brief Erases a segment in Flash
//!
//!
//! \param unAddress
//! \return none
//////////////////////////////////////////////////////////////////////////
void vFlash_Erase_Seg(uint16 unAddress)
{
	uint16 *unFlashPtr;

	//initialize the flash pointer to point to the given address
	unFlashPtr = (uint16 *) unAddress;

	// Erase Flash
	while (BUSY & FCTL3); // Check if Flash being used
	FCTL3 = FWKEY; // Clear Lock bit
	FCTL1 = FWKEY + ERASE; // Set Erase bit
	*unFlashPtr = 0; // Dummy write to erase Flash seg
	while (BUSY & FCTL3); // Check if Erase is done
}

//////////////////////////vFlash_DisIncorrect_BSLPW_Erase()////////////////////////////////////
//! \brief Prevents the MCU from losing all of flash memory in the event of an incorrect 
//! Bootstrap loader password
//!
//! 
//! \param none
//! \return none
////////////////////////////////////////////////////////////////////////// 
void vFlash_DisIncorrect_BSLPW_Erase(void)
{
	uint16 PrtctFlsh;
	PrtctFlsh = PROTECTFLASH;

	//initialize the flash controller
	vFlash_init();

	if (PROTECTFLASH != uiFlash_Read_Int(FLASHDATAWRD))
	{
		ucFlash_Write_Byte((uint8) PrtctFlsh, FLASHDATAWRD);
		ucFlash_Write_Byte((uint8) (PrtctFlsh >> 8), (FLASHDATAWRD + 1));
	}

} //END: vFlash_DisIncorrect_BSLPW_Erase()

//////////////////////////vFlash_GetBSLPW()////////////////////////////////////
//! \brief Prevents the MCU from losing all of flash memory in the event of an incorrect 
//! Bootstrap loader password
//!
//! 
//! \param *p_ucBuff, pointer to the destination of the password
//! \return none
////////////////////////////////////////////////////////////////////////// 
void vFlash_GetBSLPW(uint8 *p_ucBuff)
{
	uint8 ucPWLoopCnt;
	uint8 ucDataLoopCnt;
	uint16 uiPassword[0x10];

	//initialize the flash controller
	vFlash_init();

	//Write 0x0000 into location 0xFFDE to disable security feature
	vFlash_DisIncorrect_BSLPW_Erase();

	// Initialize the counter
	ucDataLoopCnt = 0x00;

	for (ucPWLoopCnt = 0; ucPWLoopCnt < 0x20; (ucPWLoopCnt += 2))
	{
		uiPassword[ucDataLoopCnt] = uiFlash_Read_Int(BSLPWSTARTADDR + ucPWLoopCnt);
		ucDataLoopCnt++;
	}

	//loop through the integer array and move the values into a character array
	//had to be creative to get the char array incrementing 2x faster than the integer array
	for (ucDataLoopCnt = 0x00; ucDataLoopCnt < 0x10; ucDataLoopCnt++)
	{
		*p_ucBuff++ = (uint8) (uiPassword[ucDataLoopCnt]);
		*p_ucBuff++ = (uint8) (uiPassword[ucDataLoopCnt] >> 8);
	}
} //END: vFlash_GetBSLPW()

////////////////////////// ucFlash_SetHID() ////////////////////////////////////
//! \brief Sets the hardware ID (HID) in flash.  The  HID is unique for every SP board and
//! is set before deployment.
//!
//! \param ucSPID
//! \return ucErrCode
////////////////////////////////////////////////////////////////////////////////
uint8 ucFlash_SetHID(uint16 *uiHID)
{
	uint8 ucErrCode;
	uint16 uiSegmentData[INFO_SEGMENTLENGTH/2];
	uint8 uiIndex;

	// Assume success
	ucErrCode = 0;

	//initialize the flash controller
	vFlash_init();

	vFlash_Read_Segment(uiSegmentData, FLASH_INFO_D);

	vFlash_Erase_Seg(FLASH_INFO_D);

	for(uiIndex = 0; uiIndex < HID_ADDRESS + 4; uiIndex++)
	{
		uiSegmentData[uiIndex] = *uiHID++;
	}

	vFlash_Write_Segment(uiSegmentData, FLASH_INFO_D);

	//if the operation failed report it to the calling function
	if ((FCTL3 & 0x80) == 0x9680)
	{
		ucErrCode = 1;
	}
	return ucErrCode;
}

////////////////////////// ulFlash_GetHID() ////////////////////////////////////
//! \brief Gets the hardware ID (HID) from flash.  The HID is unique for every SP board.
//!
//! \param *uiHID
//! \return none
//////////////////////////////////////////////////////////////////////////
void vFlash_GetHID(uint16 *uiHID)
{
 uint16 uiIndex;

	//initialize the flash controller
	vFlash_init();

	// Read the HID from the segment
	for (uiIndex = 0; uiIndex < 4; uiIndex++)
	{
		*uiHID++ = uiFlash_Read_Int(FLASH_INFO_D + HID_ADDRESS + uiIndex*2);
	}

}

//! @}

