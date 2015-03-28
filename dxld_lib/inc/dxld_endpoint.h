/*
 * dxld_endpoint.h
 *
 * Endpoints are structures that represent the memory of a Dynamixel device.
 * Each one has its own tables (for normal operation, REG_WRITE and default values), its own ID,
 * and is completely independent of the others.
 *
 *  Created on: 31 janv. 2015
 *      Author: Xevel
 */

#ifndef DXLD_LIB_INC_DXLD_ENDPOINT_H_
#define DXLD_LIB_INC_DXLD_ENDPOINT_H_
#include "chip.h"
#include <dxld_lib.h>



// TODO make this file readable!!!

void DxldEndpointSetGlobalArray(dxld_endpoint_t* array, uint8_t nb_endpoints);


//// TODO fix Set Addresses and Enable Reg Write because now they are in conflict with Init!
//// Set the variable part of the common table addresses
//// Must be done before DxlEndpointInit
//void DxldEndpointSetupSetAddresses(dxld_endpoint_t* endpoint, uint8_t  addr_return_delay_time, uint8_t	 addr_status_return_level, uint8_t  addr_registered, uint8_t  addr_internal_error );
//
//// Enable REG_WRITE and ACTION and sets their memory up.
//// Must be done before DxlEndpointInit
//void DxldEndpointSetupEnableRegWrite(dxld_endpoint_t* endpoint, uint8_t* registered_buffer, uint8_t registered_buffer_length);

//
///**
// * Init a Dynamixel endpoint.
// * @param endpoint					Pointer to the endpoint to initialize.
// * @param table_size				Size of table
// * @param ram_offset				Offset for the beginning of the RAM section in the Dynamixel table.
// * @param table						Pointer to the main dynamixel register table.
// * @param eeprom_memory_location 	Location of the eeprom memory. Parameter passed to the EEPROM management handlers.
// * @return pointer to the endpoint, or NULL if parameters are wrong or the endpoint is already initialized.
// */
//dxld_endpoint_t*  DxldEndpointInit(dxld_endpoint_t* endpoint, uint8_t table_size, uint8_t ram_offset, uint8_t* table, uint16_t eeprom_memory_location);

dxld_endpoint_t* DxldEndpointCreateMinimal(
		dxld_endpoint_t* endpoint,
		uint8_t  table_size,
		uint8_t  ram_offset,
		uint8_t* table,
		uint16_t eeprom_memory_location);

dxld_endpoint_t* DxldEndpointCreate(
		dxld_endpoint_t* endpoint,
		uint8_t  table_size,
		uint8_t  ram_offset,
		uint8_t* table,
		uint16_t eeprom_memory_location,

		uint8_t* registered_buffer,
		uint8_t  registered_buffer_length,

		uint8_t  addr_return_delay_time,
		uint8_t	 addr_status_return_level,
		uint8_t  addr_registered,
		uint8_t  addr_internal_error
		);

dxld_endpoint_t* DxldEndpointLoadTable( dxld_endpoint_t* endpoint );



/**
 * Clean up an endpoint. Memory for the tables and of the endpoint itself has to be managed by the application, it is simply returned to an clean state.
 * @param endpoint		Pointer to the endpoint too clean.
 * @param clear_eeprom	True if you want the eeprom to be marked empty.
 */
void DxldEndpointCleanup(dxld_endpoint_t* endpoint, uint8_t clear_eeprom);


// return pointer to the first endpoint satisfying the condition, or NULL.
dxld_endpoint_t* DxldEndpointGetFirst(uint8_t initialized);
// return pointer to the first endpoint with this Dynamixel ID, or NULL if not found.
dxld_endpoint_t* DxldEndpointGetFromDynamixelID(uint8_t dxl_id);
// return pointer to the endpoint at that position, or NULL if outside of table.
dxld_endpoint_t* DxldEndpointGetFromUserID(uint8_t position);



uint32_t DxldEndpointIsInitialized(    dxld_endpoint_t* endpoint);
uint32_t DxldEndpointIsRegistered(     dxld_endpoint_t* endpoint);


void 	 DxldEndpointLoadDefault(      dxld_endpoint_t* endpoint);

// EEPROM operations return true if could complete the operation, false otherwise
uint32_t DxldEEPROMReadAll(    dxld_endpoint_t* endpoint);
uint32_t DxldEEPROMWriteAll(   dxld_endpoint_t* endpoint);
uint32_t DxldEEPROMWrite(      dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t nb_to_save);
uint32_t DxldEEPROMClear(      dxld_endpoint_t* endpoint);

uint8_t      DxldEndpointGetDynamixelID(   dxld_endpoint_t* endpoint );
uint8_t      DxldEndpointGetUserID(   	   dxld_endpoint_t* endpoint );
dxld_error_t DxldEndpointRead(             dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_dst, uint8_t nb_to_read );
dxld_error_t DxldEndpointWriteInternal(    dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write);

dxld_error_t DxldEndpointWriteCheck(       dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_registered);
void         DxldEndpointWriteAfterCheck(  dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_registered);
void 		 DxldEndpointCommitRegistered( dxld_endpoint_t* endpoint );
void 	     DxldEndpointStatusPacketEmpty(dxld_endpoint_t* endpoint, dxld_error_t error, uint8_t return_level);
void 	     DxldEndpointStatusPacketRead( dxld_endpoint_t* endpoint, dxld_error_t error, uint8_t start_addr, uint8_t nb_bytes, uint8_t return_level);


static inline bool DxldRegArrayWillBeAccessed(dxld_endpoint_t* endpoint, uint8_t reg_table_addr, uint8_t reg_array_size, uint8_t start_addr, uint8_t nb_to_write ){
	return start_addr < reg_table_addr + reg_array_size
			&& start_addr + nb_to_write > reg_table_addr;
}
static inline bool DxldRegWillBeModified(dxld_endpoint_t* endpoint, uint8_t reg_table_addr, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write ){
	return DxldRegArrayWillBeAccessed(endpoint, reg_table_addr, 1, start_addr, nb_to_write)
			&& endpoint->table[reg_table_addr] != data_src[reg_table_addr - start_addr];
}


void 	 DxldInternalErrorSet(         dxld_endpoint_t* endpoint, dxld_error_t error_mask);

// makes it easy to walk through all endpoints. No interrupt protection.
dxld_endpoint_t* DxldEndpointIteratorFirst();
dxld_endpoint_t* DxldEndpointIteratorNext();


#endif /* DXLD_LIB_INC_DXLD_ENDPOINT_H_ */
