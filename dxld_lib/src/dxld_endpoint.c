/*
 * dxld_endpoint.c
 *
 *  Created on: 31 janv. 2015
 *      Author: Xevel
 */
#include "dxld_endpoint.h"
#include "dxld_parse.h"
#include "dxld_hal.h"
#include "string.h"


// default values used if no default table is given

#define ENDPOINT_DEFAULT_MODEL_NUMBER_L			43
#define ENDPOINT_DEFAULT_MODEL_NUMBER_H			1
#define ENDPOINT_DEFAULT_VERSION				1
#define ENDPOINT_DEFAULT_ID						1
#define ENDPOINT_DEFAULT_BAUDRATE				1 // 1Mbps
#define ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL 	STATUS_RETURN_ALL

uint8_t Endpoint_default_table[ENDPOINT_DEFAULT_TABLE_LEN] = {
		ENDPOINT_DEFAULT_MODEL_NUMBER_L,
		ENDPOINT_DEFAULT_MODEL_NUMBER_H,
		ENDPOINT_DEFAULT_VERSION,
		ENDPOINT_DEFAULT_ID,
		ENDPOINT_DEFAULT_BAUDRATE
		};

static dxld_endpoint_t* Endpoint_array = NULL; // memory pointed to is managed by the application
static uint8_t Endpoint_nb_endpoints = 0; // number of elements in Endpoint_array



static uint8_t Endpoint_EEPROMReadBlock(dxld_endpoint_t* endpoint, void* destination, uint16_t source_addr, size_t nb_bytes ){
	uint8_t success = ( DxldEEPROMReadBlock_Handler( endpoint, destination, source_addr, nb_bytes ) == nb_bytes);
	if (!success){
		DxldInternalErrorSet(endpoint, DXLD_INTERNAL_ERROR_EEPROM);
	}
	return success;
}

static uint8_t Endpoint_EEPROMWriteBlock(dxld_endpoint_t* endpoint, const void* source, uint16_t destination_addr, size_t nb_bytes ){
	uint8_t success = ( DxldEEPROMWriteBlock_Handler( endpoint, source, destination_addr, nb_bytes ) == nb_bytes);
	if (!success){
		DxldInternalErrorSet(endpoint, DXLD_INTERNAL_ERROR_EEPROM);
	}
	return success;
}


void DxldEndpointSetGlobalArray(dxld_endpoint_t* array, uint8_t nb_endpoints){
	if (array == NULL || nb_endpoints == 0){
		Endpoint_array = NULL;
		Endpoint_nb_endpoints = 0;
	} else {
		Endpoint_array = array;
		Endpoint_nb_endpoints = nb_endpoints;
	}
}


dxld_endpoint_t* DxldEndpointCreateMinimal(
		dxld_endpoint_t* endpoint,
		uint8_t  table_size,
		uint8_t  ram_offset,
		uint8_t* table,
		uint16_t eeprom_memory_location){
	return DxldEndpointCreate(
			endpoint,
			table_size,
			ram_offset,
			table,
			eeprom_memory_location,

			NULL,
			0,

			DXLD_DEFAULT_ADDR_RETURN_DELAY_TIME,
			DXLD_DEFAULT_ADDR_STATUS_RETURN_LEVEL,
			DXLD_DEFAULT_ADDR_REGISTERED,
			DXLD_DEFAULT_ADDR_INTERNAL_ERROR);
}


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
		){

	dxld_endpoint_t* ep = endpoint;
	if ( ep != NULL
		&& table_size <= DXLD_ENDPOINT_TABLE_SIZE_MAX
		&& table_size > DXLD_ENDPOINT_TABLE_SIZE_MIN
		&& ram_offset > DXLD_ENDPOINT_RAM_OFFSET_MIN
		&& table != 0){

		ep->table_size 					= table_size;
		ep->ram_offset 					= ram_offset;
		ep->eeprom_memory_location 		= eeprom_memory_location;
		ep->table 						= table;

		ep->registered_buffer 			= registered_buffer;
		ep->registered_buffer_length	= registered_buffer_length;
		ep->registered_start_addr 		= 0;
		ep->registered_nb_registered 	= 0;

		// user_id is not initialized, it's use is completely left to the user

		ep->addr_return_delay_time 		= addr_return_delay_time;
		ep->addr_status_return_level	= addr_status_return_level;
		ep->addr_registered 			= addr_registered;
		ep->addr_internal_error 		= addr_internal_error;

		return ep;
	}
	return NULL;
}


dxld_endpoint_t* DxldEndpointLoadTable( dxld_endpoint_t* endpoint ){
	if (DxldEndpointIsInitialized(endpoint)){
		memset(endpoint->table, 0, endpoint->table_size);

		// load library defaults
		memcpy(endpoint->table, Endpoint_default_table, ENDPOINT_DEFAULT_TABLE_LEN);
		endpoint->table[endpoint->addr_status_return_level] = ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL;

		uint32_t eeprom_loaded = DxldEEPROMReadAll(endpoint);
		DxldEndpointLoadDefault_Handler(endpoint, eeprom_loaded);
		DxldEEPROMWriteAll(endpoint);

		return endpoint;
	}
	return NULL;
}


void DxldEndpointCleanup(dxld_endpoint_t* endpoint, uint8_t clear_eeprom){
	if (endpoint != NULL){
		if (DxldEndpointIsInitialized(endpoint)){
			endpoint->table_size = 0; // make it look unintialized so that no interrupt try to modify the endpoint anymore

			if (clear_eeprom){
				DxldEEPROMClear(endpoint);
			}

			//actually clear the endpoint
			memset( endpoint, 0, sizeof(dxld_endpoint_t) );
		}
	}
}



dxld_endpoint_t* DxldEndpointGetFirst(uint8_t initialized ){
	for(unsigned int i = 0; i < Endpoint_nb_endpoints; i++ ){
		dxld_endpoint_t* ep = &Endpoint_array[i];
		if (DxldEndpointIsInitialized(ep) == initialized){
			return ep;
		}
	}
	return NULL;
}


dxld_endpoint_t*  DxldEndpointGetFromDynamixelID(uint8_t dxl_id){
	for(unsigned int i = 0; i < Endpoint_nb_endpoints; i++ ){
		dxld_endpoint_t* ep = &Endpoint_array[i];
		if (DxldEndpointIsInitialized(ep) && ep->table[DXLD_ADDR_ID] == dxl_id ){
			return ep;
		}
	}
	return NULL;
}

dxld_endpoint_t* DxldEndpointGetFromUserID(uint8_t user_id){
	for(unsigned int i = 0; i < Endpoint_nb_endpoints; i++ ){
		dxld_endpoint_t* ep = &Endpoint_array[i];
		if (DxldEndpointIsInitialized(ep) && ep->user_id == user_id ){
			return ep;
		}
	}
	return NULL;
}

uint8_t DxldEndpointGetUserID(dxld_endpoint_t* endpoint ){
	return endpoint->user_id;
}


uint32_t DxldEndpointIsInitialized(dxld_endpoint_t* endpoint){
	return endpoint->table != 0;
}

uint32_t DxldEndpointIsRegistered(dxld_endpoint_t* endpoint){
	return endpoint->table[ endpoint->addr_registered ];
}


void DxldEndpointLoadDefault(dxld_endpoint_t* endpoint){
	// deactivate that endpoint
	uint8_t table_size = endpoint->table_size;
	endpoint->table_size = 0;

	memset(endpoint->table, 0, table_size);
	memcpy(endpoint->table, Endpoint_default_table, ENDPOINT_DEFAULT_TABLE_LEN); // load library defaults
	DxldEndpointLoadDefault_Handler(endpoint, false);
	DxldEEPROMWriteAll(endpoint);
	endpoint->registered_nb_registered = 0;

	//  reactivate endpoint
	endpoint->table_size = table_size;
}


uint32_t DxldEEPROMReadAll(dxld_endpoint_t* endpoint){
	uint8_t magic_key[DXL_EEPROM_MAGIC_KEY_LEN];

	uint32_t eeprom_loaded = false;
	uint32_t read_success = Endpoint_EEPROMReadBlock(
								endpoint,
								(void*)(magic_key),
								endpoint->eeprom_memory_location,
								DXL_EEPROM_MAGIC_KEY_LEN);
	if ( read_success
			&& magic_key[0] == DXL_EEPROM_MAGIC_KEY_0
			&& magic_key[1] == DXL_EEPROM_MAGIC_KEY_1
			&& magic_key[2] == DXL_EEPROM_MAGIC_KEY_2 ){
		// we verified that we have values previously saved in eeprom, so load them
		eeprom_loaded = Endpoint_EEPROMReadBlock(
				endpoint,
				(void*)(endpoint->table + DXL_EEPROM_MAGIC_KEY_LEN),
				endpoint->eeprom_memory_location + DXL_EEPROM_MAGIC_KEY_LEN,
				endpoint->ram_offset - DXL_EEPROM_MAGIC_KEY_LEN );
	}
	return eeprom_loaded;
}

uint32_t DxldEEPROMWriteAll(dxld_endpoint_t* endpoint){
	return DxldEEPROMWrite(endpoint, DXL_EEPROM_MAGIC_KEY_LEN, endpoint->ram_offset - DXL_EEPROM_MAGIC_KEY_LEN);
}

uint32_t DxldEEPROMWrite(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t nb_to_save){
	uint8_t magic_key[DXL_EEPROM_MAGIC_KEY_LEN] = {DXL_EEPROM_MAGIC_KEY_0, DXL_EEPROM_MAGIC_KEY_1, DXL_EEPROM_MAGIC_KEY_2};

	uint32_t write_success = Endpoint_EEPROMWriteBlock(
			endpoint,
			(void*)(endpoint->table + start_addr),
			endpoint->eeprom_memory_location + start_addr,
			nb_to_save );
	if (write_success){
		write_success = Endpoint_EEPROMWriteBlock(
			endpoint,
			(void*)(magic_key),
			endpoint->eeprom_memory_location,
			DXL_EEPROM_MAGIC_KEY_LEN );
	}
	return write_success;
}

uint32_t DxldEEPROMClear(dxld_endpoint_t* endpoint){
	uint8_t not_magic_key[DXL_EEPROM_MAGIC_KEY_LEN] = {0xFF, 0xFF, 0xFF};
	uint32_t write_success = Endpoint_EEPROMWriteBlock(
			endpoint,
			(void*)(not_magic_key),
			endpoint->eeprom_memory_location,
			DXL_EEPROM_MAGIC_KEY_LEN );
	return write_success;
}



uint8_t DxldEndpointGetDynamixelID(dxld_endpoint_t* endpoint){
	if ( DxldEndpointIsInitialized(endpoint)){
		return endpoint->table[DXLD_ADDR_ID];
	} else {
		return 0xFF;
	}
}

// TODO protect more? Look at the outline of the lib and add required checks

// for internal use only, the dynamixel interface uses DxldEndpointStatusPacketRead
dxld_error_t DxldEndpointRead(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_dst, uint8_t nb_to_read){
	if (start_addr + nb_to_read > endpoint->table_size){
		return DXL_ERROR_RANGE;
	}

	uint32_t primask = __get_PRIMASK(); // save global interrupt enable state
	__disable_irq();
	memcpy(data_dst, &endpoint->table[start_addr], nb_to_read);
	__set_PRIMASK(primask); //restore global interrupt state

	return DXL_ERROR_NONE;
}



void Endpoint_UpdateBaudrateInOthers(dxld_endpoint_t* endpoint, uint8_t new_baudrate){

	// write it to all other endpoints
	dxld_endpoint_t* endpoint_iter = DxldEndpointIteratorFirst();
	while ( endpoint_iter != NULL ){
		if (endpoint_iter != endpoint
				&& endpoint_iter->table[DXLD_ADDR_BAUDRATE] != new_baudrate){
			endpoint_iter->table[DXLD_ADDR_BAUDRATE] = new_baudrate;
			Endpoint_EEPROMWriteBlock(
				endpoint_iter,
				&new_baudrate,
				endpoint_iter->eeprom_memory_location + DXLD_ADDR_BAUDRATE,
				1 );
		}
		endpoint_iter = DxldEndpointIteratorNext();
	}
}


dxld_error_t DxldEndpointWriteInternal(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write){
	dxld_error_t error = DxldEndpointWriteCheck(endpoint, start_addr, data_src, nb_to_write, true, false);
	if (error == DXL_ERROR_NONE){
		DxldEndpointWriteAfterCheck(endpoint, start_addr, data_src, nb_to_write, true, false);
	}
	return error;
}


// in both cases, do not accept bad ID or bad baudrate
// internal use can write wherever in the main table, verifications must be done by the application before calling
//      DXL_ERROR_RANGE when writing outside of table
// external use writes only in the potentially writable part of the table, and it has to pass through the application verifications before being considered
//      DXL_ERROR_RANGE writing outside of table or on read_only or only one byte of a 2 byte register or bad value
//      Can return other errors based on the application evaluation of the data
// The data should only be written if there are no error reported
dxld_error_t DxldEndpointWriteCheck(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_registered){
	// common checks : write to existing table, write inside the table
	if ( endpoint->table == NULL
			|| start_addr + nb_to_write > endpoint->table_size){
		return DXL_ERROR_RANGE;
	}
	// checks for registered : that it fits in the registered buffer
	if (write_to_registered
			&& nb_to_write > endpoint->registered_buffer_length){
		return DXL_ERROR_RANGE;
	}

	// ID values of 0xFF(restricted), 0xFD (broadcast) and 0xFD (usb2ax) are not accepted
	if ( DxldRegWillBeModified(endpoint, DXLD_ADDR_ID, start_addr, data_src, nb_to_write) ){
		uint8_t new_id = data_src[DXLD_ADDR_ID - start_addr];
		if ( new_id >= 0xFD ){
			return DXL_ERROR_RANGE;
		}
	}

	// check if baudrate is acceptable
	if ( DxldRegWillBeModified(endpoint, DXLD_ADDR_BAUDRATE, start_addr, data_src, nb_to_write) ){
		if ( !DxldHALBaudrateIsValid( data_src[DXLD_ADDR_BAUDRATE - start_addr] )){
			return DXL_ERROR_RANGE;
		}
	}

	// checks for external use : not in read-only, and application checks
	if( !internal_use ){
		if (start_addr < DXLD_ADDR_ID){
			return DXL_ERROR_RANGE;
		}

		uint8_t error = DxldEndpointCheckDataValidity_Handler(endpoint, start_addr, data_src, nb_to_write, internal_use, write_to_registered);
		if (error != DXL_ERROR_NONE){
			return error;
		}
	}
	return DXL_ERROR_NONE;
}

void DxldEndpointWriteAfterCheck(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_registered){
	if (nb_to_write){
		if (write_to_registered){
			memcpy(endpoint->registered_buffer, data_src, nb_to_write);
			endpoint->registered_start_addr    = start_addr;
			endpoint->registered_nb_registered = nb_to_write;

			endpoint->table[endpoint->addr_registered] = 1;
		} else {
			bool need_baudrate_update = start_addr <= DXLD_ADDR_BAUDRATE && DXLD_ADDR_BAUDRATE < start_addr + nb_to_write;
			uint8_t new_baudrate, old_baudrate;
			if (need_baudrate_update){
				old_baudrate = endpoint->table[DXLD_ADDR_BAUDRATE];
				new_baudrate = data_src[DXLD_ADDR_BAUDRATE - start_addr];
			}

			uint32_t primask = __get_PRIMASK(); // save global interrupt enable state
			__disable_irq();
			memcpy(&endpoint->table[start_addr], data_src, nb_to_write );
			__set_PRIMASK(primask); //restore global interrupt state

			DxldEndpointWritten_Handler(endpoint, start_addr, nb_to_write, internal_use);

			if (need_baudrate_update){
				if (new_baudrate != old_baudrate){
					DxldHALInit(new_baudrate); // the validity of the baudrate should have been tested before
				}
				Endpoint_UpdateBaudrateInOthers(endpoint, new_baudrate );
			}

			if ( start_addr < endpoint->ram_offset ){
				Endpoint_EEPROMWriteBlock(
							endpoint,
							&endpoint->table[start_addr],
							endpoint->eeprom_memory_location + start_addr,
							MIN( nb_to_write, endpoint->ram_offset - start_addr ) );
			}
		}


	}
}


void DxldEndpointCommitRegistered(dxld_endpoint_t* endpoint){
	DxldEndpointWriteAfterCheck(
			endpoint,
			endpoint->registered_start_addr,
			endpoint->registered_buffer,
			endpoint->registered_nb_registered,
			false,
			false);
	endpoint->table[endpoint->addr_registered] = 0;
}



void DxldEndpointStatusPacketEmpty(dxld_endpoint_t* endpoint, dxld_error_t error, uint8_t return_level){
	DxldEndpointStatusPacketRead(endpoint, error, endpoint->table_size, 0, return_level);
}


void DxldEndpointStatusPacketRead(dxld_endpoint_t* endpoint, dxld_error_t error, uint8_t start_addr, uint8_t nb_bytes, uint8_t return_level){
	if( return_level <= endpoint->table[endpoint->addr_status_return_level] ){
		uint8_t dxl_id = endpoint->table[DXLD_ADDR_ID];
		uint8_t return_delay_us = 4 * (uint16_t)endpoint->table[endpoint->addr_return_delay_time];
		uint8_t* data = NULL;
		uint8_t available_param = 0;  // number of available bytes in the table starting from start_addr
		if ( start_addr < endpoint->table_size ){
			data = &endpoint->table[start_addr];
			available_param = endpoint->table_size - start_addr;
		}
		if ( error != DXL_ERROR_NONE ){ // TODO check that it's the only place we need it
			DxldErrorOccured_Handler(endpoint, error);
		}
		DxldHALStatusPacket(dxl_id, error, data, available_param, nb_bytes, return_delay_us);
	}
}


void DxldInternalErrorSet(dxld_endpoint_t* endpoint, dxld_error_t error_mask){
	uint32_t primask = __get_PRIMASK(); // save global interrupt enable state
	__disable_irq();
	endpoint->table[endpoint->addr_internal_error] |= (error_mask & 0x7F);
	__set_PRIMASK(primask); //restore global interrupt state

	DxldInternalErrorOccured_Handler(endpoint, error_mask);
}




static uint8_t Endpoint_iter_pos = 0;
static dxld_endpoint_t* Endpoint_IteratorGet(){
	if ( Endpoint_iter_pos < Endpoint_nb_endpoints && DxldEndpointIsInitialized( &Endpoint_array[Endpoint_iter_pos] ) ){
		return &Endpoint_array[Endpoint_iter_pos];
	}
	return NULL;
}

dxld_endpoint_t* DxldEndpointIteratorFirst(){
	Endpoint_iter_pos = 0;
	return Endpoint_IteratorGet();
}

dxld_endpoint_t* DxldEndpointIteratorNext(){
	Endpoint_iter_pos++;
	return Endpoint_IteratorGet();
}


