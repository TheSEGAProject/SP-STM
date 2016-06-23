///////////////////////////////////////////////////////////////////////////////
//! \file core.h
//! \brief Header file for the \ref core Module.
//!
//! This file provides all of the defines and function prototypes for the
//! \ref core Module.
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
// Edited by CP
//*****************************************************************************

#ifndef CORE_H_
  #define CORE_H_

  //! @name System Defines
  //! These defines are used by the entire system
  //! @{
  #define TRUE  1
  #define FALSE 0

  #define true  1
  #define false 0

  #define NULL 0

  // Maximum number of transducers
  //! \def MAX_NUM_TRANSDUCERS
  //! \brief This sets the maximum number of transducers the core can handle
  #define MAX_NUM_TRANSDUCERS 0x10

  //! \def TRANSDUCER_LABEL_LEN
  //! \brief The fixed length of the transducer labels
  #define TRANSDUCER_LABEL_LEN 0x10

  //! \def VERSION_LABEL_LEN
  //! \brief The fixed length of the version labels
  #define VERSION_LABEL_LEN    0x10

  //! \def MIN_VOLTAGE
  //! \brief When checking the voltage, it should be at least 2.2V or we send an error
  //! The value is 2.2V
  #define MIN_VOLTAGE	       0xDC

  //! \def PACKET_ERROR_CODE
  //! \brief This error code is sent to the CP if the packet type is not recognized
  #define PACKET_ERROR_CODE	   0xF1

  //! @}

  // Size typedefs
  //! @name System Typedefs
  //! These typedefs are used for the entire core and wrapper. This makes
  //! porting code easier and variable types faster to write.
  //! @{
  typedef unsigned char uint8;
  typedef signed   char int8;

  typedef unsigned int  uint16;
  typedef signed   int  int16;

  typedef unsigned long uint32;
  typedef signed   long int32;

  unsigned int uiCORE_GetVoltage(void);

  //! @name Control Functions
  //! These functions are used to control the \ref core Module.
  //! @{
  void vCORE_Initilize(void);
  void vCORE_InitilizeTransducerTable(void); //Now also sets functions from header
  void vCORE_Run(void);
  //! @}

  // Core modules to include
  #include "comm/msg.h"
  #include "comm/comm.h"
  #include "changeable_core_header.h"
  #include "flash.h"


#endif /*CORE_H_*/
//! @}
