///////////////////////////////////////////////////////////////////////////////
//! \file flash.h
//! \brief Header file for the flash module
//!
//!
//! @addtogroup core
//! @{

#ifndef FLASH_H_
#define FLASH_H_

//! \def PROTECTFLASH
//! \brief Value to be stored in the flash data word location to prevent erasure of flash
#define PROTECTFLASH 	0x0000

//! \def FLASHDATAWRD
//! \brief location in memory that controls security options of flash memory
#define FLASHDATAWRD 	0xFFDE

//! \def BSLPWSTARTADDR
//! \brief starting point in memory where the bootstrap loader password is stored
#define BSLPWSTARTADDR	0xFFE0

//! \def BSLPWDLEN
//! \brief Length of the BSL password
#define BSLPWDLEN			0x20

//! \def INFO_SEGMENTLENGTH
//! \brief Length of a segment in information memory
#define INFO_SEGMENTLENGTH		64

//! \def FLASH_INFO_B
//! \brief The address of sector B of information memory
#define FLASH_INFO_B	0x1080

//! \def FLASH_INFO_C
//! \brief The address of sector C of information memory
#define FLASH_INFO_C	0x1040

//! \def FLASH_INFO_D
//! \brief The address of sector D of information memory
#define FLASH_INFO_D	0x1000

//! \def HID_ADDRESS
//! \brief The address in info memory sector D
#define HID_ADDRESS	0

// flash.c function prototypes
//! @name flash module Functions
//! These functions handle controlling the on CPU flash memory module
//! @{
void vFlash_GetBSLPW(uint8 *p_ucBuff);
void vFlash_DisIncorrect_BSLPW_Erase(void);
void vFlash_GetHID(uint16 *uiHID);
uint8 ucFlash_SetHID(uint16 *uiHID);
//flash_dco_cal
//! @}

#endif /*FLASH_H_*/
//! @}
