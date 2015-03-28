/*
 * dxld_lib.c
 *
 *  Created on: 26 janv. 2015
 *      Author: Nicolas Saugnier
 * // TODO license
 */
#include <dxld_lib.h>
#include "dxld_hal.h"
#include "dxld_endpoint.h"
#include "dxld_parse.h"
#include "dxld_debug.h"

// TODO protect against multiple calls, maybe...


void DxldInit(dxld_endpoint_t* array, uint8_t nb_endpoints){
	D_DXLD_EXPECT_TRUE(array);
	D_DXLD_EXPECT_TRUE(nb_endpoints);

	DxldEndpointSetGlobalArray(array, nb_endpoints);
}


// start the hardware using baudrate in the first endpoint.
// returns the baud rate in bps
uint32_t DxldStart(){
	dxld_endpoint_t* endpoint = DxldEndpointIteratorFirst();
	uint8_t baudrate = 0;
	DxldEndpointRead(endpoint, DXLD_ADDR_BAUDRATE, &baudrate, 1);
	return DxldStartBaudrate(baudrate);
}

void DxldStop(){
	DxldHALDeInit();
}


uint32_t DxldStartBaudrate(dxld_baudrate_t dxl_baud){
	uint32_t actual_baudrate = DxldHALInit(dxl_baud);
	if (actual_baudrate != 0){

		// writing baud value to one endpoint will take care of them all.
		dxld_endpoint_t* endpoint = DxldEndpointIteratorFirst();
		dxld_error_t error = DxldEndpointWriteInternal(
									endpoint,
									DXLD_ADDR_BAUDRATE,
									&dxl_baud,
									1);
		if(error){} // just to remove "value unused warning when DXL_DEBUG is not defined
		D_DXLD_EXPECT_VALUE(DXL_ERROR_NONE, error);
	}
	return actual_baudrate;
}


void DxldTask(){
	int16_t c;
	while ( (c = DxldHALGetByte()) != -1){
		DxldParse( (uint8_t) c );
	}
}



Status DxldReadByte (dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_dst){
	dxld_error_t error = DxldEndpointRead(endpoint, start_addr, data_dst, 1 );
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}


Status DxldReadWord (dxld_endpoint_t* endpoint, uint8_t start_addr, uint16_t* data_dst ){
	uint8_t tmp[2];
	dxld_error_t error = DxldEndpointRead(endpoint, start_addr, tmp, 2 );
	if (error == DXL_ERROR_NONE){
		*data_dst = ((uint32_t)tmp[1] << 8) | tmp[0] ;
	}
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}

Status DxldReadBlock(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_dst, uint8_t nb_to_read ){
	dxld_error_t error = DxldEndpointRead(endpoint, start_addr, data_dst, nb_to_read );
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}



Status DxldWriteByte (dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t byte){
	dxld_error_t error = DxldEndpointWriteInternal(endpoint, start_addr, &byte, 1);
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}

Status DxldWriteWord (dxld_endpoint_t* endpoint, uint8_t start_addr, uint16_t word){
	uint8_t data[2];
	data[0] = (uint8_t)word;
	data[1] = (uint8_t)(word >> 8);
	dxld_error_t  error = DxldEndpointWriteInternal(endpoint, start_addr, data, 2);
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}

Status DxldWriteBlock(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data, uint8_t nb_to_write ){
	dxld_error_t error = DxldEndpointWriteInternal(endpoint, start_addr, data, nb_to_write);
	return (error == DXL_ERROR_NONE) ? SUCCESS : ERROR ;
}


//----------------- Default handlers --------------------
// These should be re-implemented by the user application.
// Both EEPROM access handlers should be re-implemented by the application if some form of non volatile memory is used.


__attribute__ ((section(".after_vectors")))
size_t DxldEEPROMReadBlock_Handler( dxld_endpoint_t* endpoint, void* destination, uint16_t source, size_t nb_bytes ){
	return 0;
}

__attribute__ ((section(".after_vectors")))
size_t DxldEEPROMWriteBlock_Handler( dxld_endpoint_t* endpoint, const void* source, uint16_t destination, size_t nb_bytes ){
	return 0;
}

__attribute__ ((section(".after_vectors")))
void DxldEndpointLoadDefault_Handler( dxld_endpoint_t* endpoint, bool values_loaded_from_eeprom){
}

__attribute__ ((section(".after_vectors")))
dxld_error_t DxldEndpointCheckDataValidity_Handler(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t* data_src, uint8_t nb_to_write, uint32_t internal_use, uint8_t write_to_temp ){
	return DXL_ERROR_NONE;
}

__attribute__ ((section(".after_vectors")))
void DxldEndpointWritten_Handler(dxld_endpoint_t* endpoint, uint8_t start_addr, uint8_t nb_written, uint32_t internal_use ){
}

__attribute__ ((section(".after_vectors")))
void DxldReturnDelay_Handler(uint16_t return_delay_us){
}

__attribute__ ((section(".after_vectors")))
void DxldValidPacketReceived_Handler(uint8_t id, uint8_t instruction, uint8_t* params, uint8_t nb_params){
}

__attribute__ ((section(".after_vectors")))
void DxldRunBootloader_Handler(dxld_endpoint_t* endpoint){
}

__attribute__ ((section(".after_vectors")))
void DxldErrorOccured_Handler(dxld_endpoint_t* endpoint, dxld_error_t error){
}


void DxldInternalErrorOccured_Handler(dxld_endpoint_t* endpoint, dxld_internal_error_t error){
}


void DxldPacketSent_Handler(uint8_t dxl_id, dxld_error_t err, uint8_t* params, uint8_t nb_param_sent){
}


__attribute__ ((section(".after_vectors")))
void DxldFatalError_Handler(){
	while(1);
}
