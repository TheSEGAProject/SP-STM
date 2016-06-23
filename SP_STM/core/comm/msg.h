///////////////////////////////////////////////////////////////////////////////
//! \file msg.h
//! \brief The file contains the Messages module
//!
//! @addtogroup core
//! @{
//!
//! @addtogroup msg Messages
//! The Messages Module (msg.h) contains all of the data structures and defines
//! to communicate back and forth between the SP Board boards and the brain
//! board
//! @{
///////////////////////////////////////////////////////////////////////////////
//*****************************************************************************
// By: Kenji Yamamoto
//     Wireless Networks Research Lab
//     Dept Of Electrical Engineering, CEFNS
//     Northern Arizona University
//
//*****************************************************************************

#ifndef MSG_H_
#define MSG_H_
//****************  SP Board Data Messages  *********************************//
//! @defgroup msg_data SP Board Data Message
//! The SP Board Data Message is used to sent data back and forth between the
//! SP Board and the CP Board. The request message is ALWAYS a data message, the
//! return message can be a data message or a label message.
//! @{
#define SP_DATAMESSAGE_VERSION 120     //!< Version 1.20
// Message Types
//! @name Data Message Types
//! These are the possible data message types.
//! @{
//! \def COMMAND_PKT
//! \brief This packet contains the command from the CP Board to the SP Board on
//! what functions to execute.
//!
//! The SP Board replies with a CONFIRM_COMMAND if the command is valid
#define COMMAND_PKT   		0x01

//! \def REPORT_DATA
//! \brief This packet reports a transducer measurement to the CP Board
//!
//! This packet is only sent from the SP Board board to the CP Board. The
//! transducer measurement is contained in data1 through data 8.
//! The SP sends a 128 (1-8) bit or 32 (1-2) bit data packet, depending on the SP function
#define REPORT_DATA     	0x02

//! \def PROGRAM_CODE
//! \brief This packet contains program code
//!
//! This packet is only sent from the real-time-data-center to the target. The
//! packet contains a portion of a programming update.

#define PROGRAM_CODE     	0x03

//! \def REQUEST_DATA
//! \brief This packet requests data from the SP Board board
//!
//! This packet is only sent from the CP Board to the SP Board.
//! The SP will send a REPORT_DATA packet as a reply, either 32 bit or 128 bit
//! packet depending on the SP function.
#define REQUEST_DATA   		0x04

//! \def REQUEST_LABEL
//! \brief This packet asks the SP Board board to send a label message back
//!
//! The packet is sent from the CP Board to the SP Board and the CP Board will
//! expect a SP_LabelMessage in return. At this time the data1 and
//! data2 fields in the message contain do-not-care values.
#define REQUEST_LABEL   	0x05

//! \def ID_PKT
//! \brief This packet identifies the SP Board board to the CP Board
//!
//! This packet is only ever sent when the SP Board starts up and should
//! never be sent by the CP Board.
#define ID_PKT          	0x06

//! \def CONFIRM_COMMAND
//! \brief This packet is used by the SP Board to confirm that the COMMAND_PKT
//! sent by the CP Board is valid and that the commands will be executed.
#define CONFIRM_COMMAND   0x07

//! \def REPORT_ERROR
//! \brief This packet is used by the SP Board to report an error to the CP.
#define REPORT_ERROR   		0x08

//! \def REQUEST_BSL_PW
//! \brief This packet is used by the CP board to request the bootstrap loader password
//!
//! The SP will look into flash address 0xFFE0 to 0xFFFF and send the contents to the
//! CP board.
#define REQUEST_BSL_PW   	0x09

//! \def INTERROGATE
//! \brief This packet is used by the CP board to request the transducer information
#define INTERROGATE   		0x0A

//! \def SET_SERIALNUM
//! \brief Used to set the serial number on the SP board from the CP.
#define SET_SERIALNUM			0x0B

//! \def REQUEST_SENSOR_TYPE
//! \brief This packet is used by the CP board to command the acquisition of the sensor type
#define COMMAND_SENSOR_TYPE			0x0C

//! \def REQUEST_SENSOR_TYPE
//! \brief This packet is used by the CP board to request the sensor type
#define REQUEST_SENSOR_TYPE			0x0D
//! @}

//! \def MAXMSGLEN
//! \brief maximum message size between CP and SP
#define 	MAXMSGLEN			0x40

//! \def SP_HEADERSIZE
//! \brief The size of the header portion of the SP message
#define SP_HEADERSIZE		4

//! @name Message Indices
//! \brief Indices for elements of a message
//! @{
//! \def MSG_TYP_IDX
#define MSG_TYP_IDX			0
//! \def MSG_LEN_IDX
#define MSG_LEN_IDX			1
//! \def MSG_VER_IDX
#define MSG_VER_IDX			2
//! \def MSG_FLAGS_IDX
#define MSG_FLAGS_IDX		3
//! \def MSG_PAYLD_IDX
#define MSG_PAYLD_IDX		4
//! @}

//! \def SP_CORE_VERSION
//! \brief This sensor number is for requesting the version string
//!
//! This sensor number should only EVER be used with REQUEST_LABEL packet.
//! Otherwise the system will try to access an array index that does
//! not exist, which will fault the hardware and lock up the software.
#define SP_CORE_VERSION 0x10
//! \def WRAPPER_VERSION
//! \brief This sensor number is for requesting the wrapper version string
//!
//! This sensor number should only EVER be used with REQEUST_LABEL packet.
//! Otherwise the system will try to access an array index that does not
//! exist, which will fault the hardware and lock up the software.
#define WRAPPER_VERSION 0x11

//****************  SP Board Label Message  *********************************//
//! @defgroup msg_label SP Board Label Message
//! The SP Board Label Message is used to send labels (byte strings) to the
//! CP board. The label message is only ever sent in reponse to a
//! LABEL_REQUEST Data Message. (See \ref msg_data).
//! @{
#define SP_LABELMESSAGE_VERSION 102        //!< v1.02
// Message Types
//! \name Label Message Types
//! These are the possible Label Message types
//! @{
//! \def REPORT_LABEL
//! \brief This packet contains a label indicated by the sensor number.
//!
//! This type of label packet is only sent by the SP Board board in response
//! to a LABEL_REQUEST Data Message. It has a different packet length and
//! is longer than a Data Message and therefore can be receieved as a
//! Data Message in error.
#define REPORT_LABEL             0x0A
//! @}


#endif /*MSG_H_*/
//! @}
//! @}

