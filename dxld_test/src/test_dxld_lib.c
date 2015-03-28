/*
 * test_dxld_lib.c
 *
 *  Created on: 8 févr. 2015
 *      Author: Xevel
 */
#include <test_dxld_lib.h>
#include "chip.h"
#include "dxld_lib.h"
#include "dxld_test.h"
#include "dxld_endpoint.h"
#include "dxld_parse.h"
#include "string.h"

// TODO right now it is duplicated, find a way to import it from the lib instead ><
#define ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL 	STATUS_RETURN_ALL


// EEPROM is voluntarily not packed properly : to pack it, EEPROM_MEMORY_LOCATION_M should be (EEPROM_MEMORY_LOCATION_N + RAM_ADDR_N)
#define TABLE_SIZE_0 		32
#define RAM_ADDR_0   		16
#define EEPROM_MEMORY_LOCATION_0 0x0000

#define TABLE_SIZE_1 		64
#define RAM_ADDR_1   		18
#define EEPROM_MEMORY_LOCATION_1 0x0014 // 20, voluntarily not packed tightly for test purpopse. Normal use can pack EEPROM

#define TABLE_DEFAULT_LEN_0   5
uint8_t table0_default[TABLE_DEFAULT_LEN_0] = { 100, 1, 2, 3, 4 }; // does not cover eeprom completely
uint8_t table1_default[] = { 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38 }; // covers eeprom and goes into the ram


// possible direct declaration of the endpoint. Still need to initialize it though, to load the table with the default or eeprom values
dxld_endpoint_t ep_test = {
	.table_size 				= TABLE_SIZE_0,
	.ram_offset 				= RAM_ADDR_0,
	.eeprom_memory_location 	= EEPROM_MEMORY_LOCATION_0
//	.table 						=  ,
//	.registered_buffer 			=  ,
//	.registered_buffer_length   =  ,
//	.registered_start_addr  	= 0,
//	.registered_nb_registered 	= 0,
//	.user_id                    = 0, // freely available to the user
//
//	.addr_return_delay_time		=  ,
//	.addr_status_return_level	=  ,
//	.addr_registered			=  ,
//	.addr_internal_error		=
} ;



void    DxldEndpointLoadDefault_Handler(dxld_endpoint_t* endpoint, bool values_loaded_from_eeprom){
	switch(endpoint->user_id){
		case 1:
			memcpy(endpoint->table, table0_default, TABLE_DEFAULT_LEN_0);
			break;
		default:
			break;
	}

}


// Emulate some EEPROM
#define VIRTUAL_EEPROM_SIZE 64
uint8_t eeprom[VIRTUAL_EEPROM_SIZE] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


uint8_t eeprom_set1[VIRTUAL_EEPROM_SIZE] = {
		0xBE, 0xEF, 0x42, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, // that line holds valid eeprom data, with the right key
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t eeprom_zeros[VIRTUAL_EEPROM_SIZE] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void load_eeprom(uint8_t* new_eeprom){
	memcpy(eeprom, new_eeprom, VIRTUAL_EEPROM_SIZE);
};

size_t DxldEEPROMReadBlock_Handler(dxld_endpoint_t* endpoint, void* destination, uint16_t source, size_t nb_bytes ){
	if ( source + nb_bytes > VIRTUAL_EEPROM_SIZE ){
		return 0;
	}

	switch(endpoint->user_id){
		case 36:
			return 0;
			break;
		default:
			memcpy(destination, eeprom + source, nb_bytes);
			break;
	}
	return nb_bytes;
}

// A real implementation using actual EEPROM would take care of banks to optimize write time
size_t DxldEEPROMWriteBlock_Handler(dxld_endpoint_t* endpoint, const void* source, uint16_t destination, size_t nb_bytes ){
	if ( destination + nb_bytes > VIRTUAL_EEPROM_SIZE ){
		return 0;
	}

	switch(endpoint->user_id){
		case 36:
			return 0;
			break;
		default:
			memcpy( eeprom + destination, source, nb_bytes);
			break;
	}
	return nb_bytes;
}



#define TABLE_SIZE 32
extern uint8_t Endpoint_default_table[ENDPOINT_DEFAULT_TABLE_LEN];

void test_DxldStart(){
	dxld_endpoint_t endpoint_array[3];
	uint8_t table0[TABLE_SIZE];
	uint8_t table1[TABLE_SIZE];
	uint8_t table2[TABLE_SIZE];
	uint8_t ram_offset = 16;
	uint8_t eeprom_location0 = 0;
	uint8_t eeprom_location1 = 16;
	uint8_t eeprom_location2 = 32;

	load_eeprom(eeprom_zeros);

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[0] ,
			TABLE_SIZE,
			ram_offset,
			table0,
			eeprom_location0)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[0]) );

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[1] ,
			TABLE_SIZE,
			ram_offset,
			table1,
			eeprom_location1)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[1]) );

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[2] ,
			TABLE_SIZE,
			ram_offset,
			table2,
			eeprom_location2)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[2]) );

	table0[DXLD_ADDR_BAUDRATE] = DXLB_BAUDRATE_3000000;

	DxldInit(endpoint_array, 3);

	EXPECT_BETWEEN_UINT32(2940000, 3060000, DxldStart());

	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[0].table[DXLD_ADDR_BAUDRATE])
	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[1].table[DXLD_ADDR_BAUDRATE])
	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[2].table[DXLD_ADDR_BAUDRATE])

	DxldStop();

}


void test_DxldStartbaudrate(){
	dxld_endpoint_t endpoint_array[3];
	uint8_t table0[TABLE_SIZE];
	uint8_t table1[TABLE_SIZE];
	uint8_t table2[TABLE_SIZE];
	uint8_t ram_offset = 16;
	uint8_t eeprom_location0 = 0;
	uint8_t eeprom_location1 = 16;
	uint8_t eeprom_location2 = 32;

	load_eeprom(eeprom_zeros);

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[0] ,
			TABLE_SIZE,
			ram_offset,
			table0,
			eeprom_location0)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[0]) );

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[1] ,
			TABLE_SIZE,
			ram_offset,
			table1,
			eeprom_location1)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[1]) );

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[2] ,
			TABLE_SIZE,
			ram_offset,
			table2,
			eeprom_location2)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[2]) );


	DxldInit(endpoint_array, 3);
	EXPECT_BETWEEN_UINT32(2940000, 3060000, DxldStartBaudrate(DXLB_BAUDRATE_3000000));

	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[0].table[DXLD_ADDR_BAUDRATE])
	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[1].table[DXLD_ADDR_BAUDRATE])
	EXPECT_VALUE(DXLB_BAUDRATE_3000000, endpoint_array[2].table[DXLD_ADDR_BAUDRATE])

	DxldStop();
}

void test_DxldEndpointInit(){

	dxld_endpoint_t endpoint_array[3];
	uint8_t table[TABLE_SIZE];
	uint8_t ram_offset = 16;
	uint8_t eeprom_location = 0;

	// no eeprom, no external defaults => only the lib defaults.
	endpoint_array[0].user_id = 0;
	load_eeprom(eeprom_zeros);
	memset(table, 0, TABLE_SIZE);
	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[0] ,
			TABLE_SIZE,
			ram_offset,
			table,
			eeprom_location)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[0]) );

	for (unsigned int  i = 0; i< ENDPOINT_DEFAULT_TABLE_LEN; i++){
		EXPECT_VALUE(Endpoint_default_table[i], table[i]);
	}
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_0, eeprom[0]);
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_1, eeprom[1]);
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_2, eeprom[2]);
	for (unsigned int  i = 3; i< ENDPOINT_DEFAULT_TABLE_LEN; i++){
		EXPECT_VALUE(Endpoint_default_table[i], eeprom[i]);
	}
	for (unsigned int i = ENDPOINT_DEFAULT_TABLE_LEN; i< TABLE_SIZE; i++){
		if (i != endpoint_array[0].addr_status_return_level){
			EXPECT_VALUE(0, table[i]);
			EXPECT_VALUE(0, eeprom[i]);
		} else {
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, table[i]);
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, eeprom[i]);
		}
	}


	//TABLE_DEFAULT_LEN_0
	// no eeprom, external defaults => lib defaults + external defaults, saved to eeprom.
	endpoint_array[1].user_id = 1;
	load_eeprom(eeprom_zeros);
	memset(table, 0, TABLE_SIZE);
	EXPECT_TRUE( DxldEndpointCreateMinimal(
				&endpoint_array[1] ,
				TABLE_SIZE,
				ram_offset,
				table,
				eeprom_location)
			);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[1]) );

	for (unsigned int  i = 0; i< TABLE_DEFAULT_LEN_0; i++){
		EXPECT_VALUE(table0_default[i], table[i]);
	}
	for (unsigned int  i = TABLE_DEFAULT_LEN_0; i< ENDPOINT_DEFAULT_TABLE_LEN; i++){
		EXPECT_VALUE(Endpoint_default_table[i], table[i]);
	}
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_0, eeprom[0]);
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_1, eeprom[1]);
	EXPECT_VALUE(DXL_EEPROM_MAGIC_KEY_2, eeprom[2]);
	for (unsigned int  i = 3; i< TABLE_DEFAULT_LEN_0; i++){
		EXPECT_VALUE(table0_default[i], eeprom[i]);
	}
	for (unsigned int  i = TABLE_DEFAULT_LEN_0; i< ENDPOINT_DEFAULT_TABLE_LEN; i++){
		EXPECT_VALUE(Endpoint_default_table[i], eeprom[i]);
	}
	for (unsigned int i = ENDPOINT_DEFAULT_TABLE_LEN; i< TABLE_SIZE; i++){
		if (i != endpoint_array[0].addr_status_return_level){
			EXPECT_VALUE(0, table[i]);
			EXPECT_VALUE(0, eeprom[i]);
		} else {
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, table[i]);
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, eeprom[i]);
		}
	}


	// some eeprom, no external defaults => lib defaults + eeprom.
	endpoint_array[0].user_id = 2;
	load_eeprom(eeprom_set1);
	memset(table, 0, TABLE_SIZE);
	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[2] ,
			TABLE_SIZE,
			ram_offset,
			table,
			eeprom_location)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[2]) );

	for (unsigned int  i = 0; i< DXL_EEPROM_MAGIC_KEY_LEN; i++){
		EXPECT_VALUE(Endpoint_default_table[i], table[i]);
	}
	for (unsigned int  i = DXL_EEPROM_MAGIC_KEY_LEN; i< ram_offset; i++){
		EXPECT_VALUE(eeprom[i], table[i]);
	}
	for (unsigned int  i = 0; i< ram_offset; i++){
		EXPECT_VALUE(eeprom_set1[i], eeprom[i]);
	}
	for (unsigned int i = MAX(ram_offset,ENDPOINT_DEFAULT_TABLE_LEN); i< TABLE_SIZE; i++){
		if (i != endpoint_array[0].addr_status_return_level){
			EXPECT_VALUE(0, table[i]);
			EXPECT_VALUE(0, eeprom[i]);
		} else {
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, table[i]);
			EXPECT_VALUE(ENDPOINT_DEFAULT_STATUS_RETURN_LEVEL, eeprom[i]);
		}
	}

}
#define TEST_READ_BUFF_SIZE 12
void test_data_read_write(){
	dxld_endpoint_t endpoint_array[1];
	uint8_t table0[TABLE_SIZE];
	uint8_t ram_offset = 16;
	uint8_t eeprom_location0 = 0;

	load_eeprom(eeprom_zeros);

	EXPECT_TRUE( DxldEndpointCreateMinimal(
			&endpoint_array[0] ,
			TABLE_SIZE,
			ram_offset,
			table0,
			eeprom_location0)
		);
	EXPECT_TRUE( DxldEndpointLoadTable(&endpoint_array[0]) );

	DxldInit(endpoint_array, 1);
	EXPECT_BETWEEN_UINT32(980000, 1020000, DxldStart() );


	uint8_t addr = 22;
	for (unsigned int  i = 0; i< ENDPOINT_DEFAULT_TABLE_LEN; i++){
		uint8_t tmp8;
		EXPECT_VALUE(SUCCESS, DxldReadByte(endpoint_array, i, &tmp8)  );
		EXPECT_VALUE(Endpoint_default_table[i], tmp8);
	}
	addr = 0;
	uint16_t model_number = Endpoint_default_table[addr] | ((uint16_t)Endpoint_default_table[addr+1] << 8);
	uint16_t tmp16;
	EXPECT_VALUE(SUCCESS, DxldReadWord(endpoint_array, addr, &tmp16));
	EXPECT_VALUE(model_number, tmp16);

	addr = 2;
	tmp16=0xFFFF;
	uint16_t word_val_2 = Endpoint_default_table[addr] | ((uint16_t)Endpoint_default_table[addr+1] << 8);
	EXPECT_VALUE(SUCCESS, DxldReadWord(endpoint_array, addr, &tmp16));
	EXPECT_VALUE(word_val_2, tmp16);

	addr = 0;
	uint8_t buff[TEST_READ_BUFF_SIZE];
	EXPECT_VALUE(SUCCESS, DxldReadBlock(endpoint_array, addr, buff, TEST_READ_BUFF_SIZE) );
	for (unsigned int i = 0; i< TEST_READ_BUFF_SIZE; i++ ){
		EXPECT_VALUE(buff[i], endpoint_array->table[ addr + i] );
	}

	uint8_t check_table[TABLE_SIZE];
	memcpy(check_table, table0, TABLE_SIZE);

	addr = 14;
	uint8_t test_byte = 0x43;
	EXPECT_VALUE(SUCCESS, DxldWriteByte(endpoint_array, addr , test_byte));
	check_table[addr] = test_byte;
	for (unsigned int i = 0; i< TABLE_SIZE; i++ ){
		EXPECT_VALUE(check_table[i], endpoint_array->table[i] )
	}

	addr = 14;
	test_byte = 0x42;
	EXPECT_VALUE(SUCCESS, DxldWriteByte(endpoint_array, addr , test_byte));
	check_table[addr] = test_byte;
	for (unsigned int i = 0; i< TABLE_SIZE; i++ ){
		EXPECT_VALUE(check_table[i], endpoint_array->table[i] )
	}

	addr = 22;
	uint16_t test_word = 8909;
	EXPECT_VALUE(SUCCESS, DxldWriteWord(endpoint_array, addr , test_word));
	check_table[addr] = test_word & 0xFF;
	check_table[addr+1] = test_word >> 8;
	for (unsigned int i = 0; i< TABLE_SIZE; i++ ){
		EXPECT_VALUE(check_table[i], endpoint_array->table[i] )
	}

	addr = 22;
	test_word = 9665;
	EXPECT_VALUE(SUCCESS, DxldWriteWord(endpoint_array, addr , test_word));
	check_table[addr] = test_word & 0xFF;
	check_table[addr+1] = test_word >> 8;
	for (unsigned int i = 0; i< TABLE_SIZE; i++ ){
		EXPECT_VALUE(check_table[i], endpoint_array->table[i] )
	}

	memset(buff, 0, TEST_READ_BUFF_SIZE);
	buff[0]=12;
	buff[3]=231;
	buff[9]=8;
	buff[10]=1;
	buff[11]=53;
	addr = 17;
	EXPECT_VALUE(SUCCESS, DxldWriteBlock(endpoint_array, addr , buff, TEST_READ_BUFF_SIZE));
	for(uint8_t i = 0; i<TEST_READ_BUFF_SIZE; i++){
		check_table[addr + i] = buff[i];
	}
	for (unsigned int i = 0; i< TABLE_SIZE; i++ ){
		EXPECT_VALUE(check_table[i], endpoint_array->table[i] )
	}



}

void run_test_dxld_lib(){

	test_DxldEndpointInit();
	test_DxldStart();
	test_DxldStartbaudrate();
	test_data_read_write();

}

