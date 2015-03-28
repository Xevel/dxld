/*
 * dxld_lib.h
 *
 *  Created on: 26 janv. 2015
 *      Author: Xevel
 */
// TODO license
#ifndef DXLD_LIB_INC_DXLD_LIB_H_
#define DXLD_LIB_INC_DXLD_LIB_H_
#include "chip.h"

// available Dynamixel baud rates. See http://support.robotis.com/en/product/dynamixel/mx_series/mx-28.htm
typedef enum {
	DXLB_BAUDRATE_9600    = 207,
	DXLB_BAUDRATE_19200   = 103,
	DXLB_BAUDRATE_57600   =  34,
	DXLB_BAUDRATE_115200  =  16,
	DXLB_BAUDRATE_200000  =   9,
	DXLB_BAUDRATE_500000  =   3,
	DXLB_BAUDRATE_1000000 =   1,
	DXLB_BAUDRATE_3000000 = 252
} dxld_baudrate_t;

// Error flags for status packets
typedef enum {
	DXL_ERROR_NONE 				= 0x00,
	//DXL_ERROR_INPUT_VOLTAGE	= 0x01
	//DXL_ERROR_ANGLE_LIMIT		= 0x02
	//DXL_ERROR_OVERHEAT		= 0x04
	DXL_ERROR_RANGE 			= 0x08, // command tried to write to unauthorized place or value outside of range, tried to read too many bytes
	DXL_ERROR_CHECKSUM 			= 0x10, // bad checksum in last command
	//DXL_ERROR_OVERLOAD		= 0x20,
	DXL_ERROR_INSTRUCTION		= 0x40  // bad instruction or ACTION without a REG_WRITE in last command
} dxld_error_t;

// internal error codes
typedef enum {
	DXLD_INTERNAL_ERROR_NONE		= 0x00,
	DXLD_INTERNAL_ERROR_EEPROM		= 0x01, // Error while accessing EEPROM
	DXLD_INTERNAL_ERROR_RX		 	= 0x02  // RX FIFO overrun, framing error, line break
} dxld_internal_error_t;


#define DXLD_ENDPOINT_TABLE_SIZE_MAX		128 // values up to 255 included are valid
#define DXLD_ENDPOINT_TABLE_SIZE_MIN 		24 // TODO
#define DXLD_ENDPOINT_RAM_OFFSET_MIN 		12 // TODO

// magic key written to EEPROM to mark that the non-volatile part of the table has been saved
#define DXL_EEPROM_MAGIC_KEY_LEN    3
#define DXL_EEPROM_MAGIC_KEY_0 		0xBE
#define DXL_EEPROM_MAGIC_KEY_1 		0xEF
#define DXL_EEPROM_MAGIC_KEY_2 		0x42
#define ENDPOINT_DEFAULT_TABLE_LEN  5


// TODO update that description
//Structure of the table, as seen by the Dynamixel master:
// <Read-only : 0 => 2> <Non-volatile: 3 => (ram_offset - 1) > < RAM: ram_offset => table_size-1 >
// Read-only: model number and firmware version, it is set by the application through the default table.
// Non-volatile: all the non-volatile values exposed to the Dynamixel Bus. The application handles the actual saving/retreiving.
//               EEPROM storage uses ram_offset bytes. Instead of saving the Read-only bytes, the first 3 bytes in EEPROM memory
//               are used as a key to check if the EEPROM contains previously written values.
// RAM: contains all the values exposed to the Dynamixel Bus that do not need saving between powerup.
// registered_data should be as long as the registered_length, that's the length of written bytes that can accepted for REG_WRITE.
// See DxldEndpointInit() for more info.

typedef struct {
	uint8_t  table_size;
	uint8_t  ram_offset;
	uint16_t eeprom_memory_location;
	uint8_t* table;

	uint8_t* registered_buffer;
	uint8_t  registered_buffer_length;
	uint8_t	 registered_start_addr;
	uint8_t  registered_nb_registered;

	uint8_t  user_id; // freely available to the user

	uint8_t  addr_return_delay_time;
	uint8_t	 addr_status_return_level;
	uint8_t  addr_registered;
	uint8_t  addr_internal_error;
} dxld_endpoint_t;



// The first 5 bytes are fixed and needed.
// The rest of the addresses needed by the library do not have a fixed place.

// can not be saved in eeprom
#define DXLD_ADDR_MODEL_NUMBER_L	0x00 // (2) Model number of the hardware
#define DXLD_ADDR_MODEL_NUMBER_H	0x01
#define DXLD_ADDR_VERSION			0x02 // (1) Firmware version
// can be saved in eeprom
#define DXLD_ADDR_ID				0x03 // (1) Dynamixel ID
#define DXLD_ADDR_BAUDRATE			0x04 // (1) Baudrate (see dxld_lib.h for values)


#define DXLD_DEFAULT_ADDR_RETURN_DELAY_TIME		0x05// (1) Time to wait before responding, x4us
#define DXLD_DEFAULT_ADDR_STATUS_RETURN_LEVEL	0x06// (1) Describes when to return a status packet

#define DXLD_DEFAULT_ADDR_REGISTERED			0x07// (1) 1 when there are pending reg_write changes
#define DXLD_DEFAULT_ADDR_INTERNAL_ERROR		0x08// (1) internal error flags (eeprom error, ...), should be cleared by the application.



//------ Error checking, trust issues...-------
// This library is intended to be lightweight and fast, while making sure no input on the
// Dynamixel side can get it in an unknown state or an error state.
// However, inside the lib, full trust is assumed and redundant checking are
// limited to a minimum.
// TODO separate basic checks in macros that can be activated for debugging
//---------------------------------------------


// DxldInit must be called before anything else. The default baudrate comes from the first endpoint, or can be set before calling DxldStart.
// DxldStop stops the operation of the hardware and a subsequent call to start restarts where it left of.
// The application is still responsible for the physical memory.
void     DxldInit(dxld_endpoint_t* array, uint8_t nb_endpoints);
uint32_t DxldStart(); 	// like StartBaudrate of the first endpoint baudrate
uint32_t DxldStartBaudrate(dxld_baudrate_t baud); // returns the baud rate in bps, uses 1000000 as default if baudrate invalid
void     DxldStop();

// must be called regularly. Internal receive buffer is 16 bytes which can fill in 35us @3Mbps, 100us @1Mbps ...
// Baudrate is synchronized across all endpoints so if one endpoint receives a write that changes it, every endpoint will change.
void DxldTask();


// Read and write from the application. Safe to call in an interrupt.
Status DxldReadByte  (dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t*  data_dst);
Status DxldReadWord  (dxld_endpoint_t* endpoint, uint8_t start_addr, uint16_t* data_dst);
Status DxldReadBlock (dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t*  data_dst, uint8_t nb_to_read );

Status DxldWriteByte (dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t   byte);
Status DxldWriteWord (dxld_endpoint_t* endpoint, uint8_t start_addr, uint16_t  word);
Status DxldWriteBlock(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t*  data, uint8_t nb_to_write );

//---------- callback functions, can be re-implemented by application ---------------------


// If implemented, access to the data (source/destination arrays) should be interrupt-protected.
__attribute__ ((weak)) size_t  DxldEEPROMReadBlock_Handler    (dxld_endpoint_t* endpoint, void* destination,  uint16_t source_addr,      size_t nb_bytes ); // Returns nb bytes read/written.
__attribute__ ((weak)) size_t  DxldEEPROMWriteBlock_Handler   (dxld_endpoint_t* endpoint, const void* source, uint16_t destination_addr, size_t nb_bytes ); // Returns nb bytes read/written.

// called when the Dynamixel instruction BOOTLOAD is received, after sending status packet.
__attribute__ ((weak)) void    DxldRunBootloader_Handler      (dxld_endpoint_t* endpoint);

// called after initializing standard default values and eeprom (at start or upon receiving RESET instruction (after sending status packet)). Redefine if you want to set your own default values
__attribute__ ((weak)) void    DxldEndpointLoadDefault_Handler(dxld_endpoint_t* endpoint, bool values_loaded_from_eeprom);

//called when a write (WRITE_DATA, REG_WRITE or from the application) is about to happen.
// Should perform checks: read only, partial write to multiple bytes value, bad value (either outside of range or not one of the acceptable values).
// Return a DXLD_ERROR_...   RANGE is the go-to error if anything is not right, but some error values are available for more specific user-defined errors.
__attribute__ ((weak)) dxld_error_t DxldEndpointCheckDataValidity_Handler(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_temp );

// Called when a valid packet has been received.
__attribute__ ((weak)) void    DxldValidPacketReceived_Handler(uint8_t id, uint8_t instruction, uint8_t* params, uint8_t nb_params);

// Called after data written to the main table but before eeprom saving. Redefine if you want to do event-driven operations upon changes to the table.
__attribute__ ((weak)) void    DxldEndpointWritten_Handler(          dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t nb_written, uint32_t internal_use );

// Called when an error has been detected. Redefine it to create a behavior similar to the AX's ALARM.
__attribute__ ((weak)) void    DxldErrorOccured_Handler(dxld_endpoint_t* endpoint, dxld_error_t error);
__attribute__ ((weak)) void    DxldInternalErrorOccured_Handler(dxld_endpoint_t* endpoint, dxld_internal_error_t error);

// Called after a packet has been sent.
__attribute__ ((weak)) void    DxldPacketSent_Handler(uint8_t dxl_id, dxld_error_t err, uint8_t* params, uint8_t nb_param_sent);

// Called before sending a status packet, it should provide the delay
__attribute__ ((weak)) void    DxldReturnDelay_Handler(uint16_t return_delay_us);


#endif /* DXLD_LIB_INC_DXLD_H_ */
