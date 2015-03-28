/*
 * hal_test.c
 *
 *  Created on: 8 févr. 2015
 *      Author: Xevel
 */
#include <test_hal.h>
#include "chip.h"
#include "dxld_test.h"
#include "dxld_lib.h"
#include "dxld_hal.h"

static uint32_t test_init(dxld_baudrate_t baud){
	uint32_t actual_baud = DxldHALInit(baud);
	DxldHALWriteByte_debug('d', false);
	DxldHALWriteByte_debug('x', false);
	DxldHALWriteByte_debug('l', false);
	DxldHALWriteByte_debug('d', true);
	DxldHALDeInit();
	return actual_baud;
}


static void test_DxldHALInit(){
	EXPECT_BETWEEN_UINT32(9408, 9792, test_init(DXLB_BAUDRATE_9600));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(18816, 19584, test_init(DXLB_BAUDRATE_19200));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(56448, 58752, test_init(DXLB_BAUDRATE_57600));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(112896, 117504, test_init(DXLB_BAUDRATE_115200));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(196000, 204000, test_init(DXLB_BAUDRATE_200000));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(490000, 510000, test_init(DXLB_BAUDRATE_500000));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(980000, 1020000, test_init(DXLB_BAUDRATE_1000000));
	Delay_ms(50);

	EXPECT_FALSE(test_init(DXLB_BAUDRATE_1000000+1));
	Delay_ms(50);

	EXPECT_BETWEEN_UINT32(2940000, 3060000, test_init(DXLB_BAUDRATE_3000000));
	Delay_ms(50);
}


static void test_DxldHALStatusPacket(){
	uint8_t dxl_id = 3;
	dxld_error_t error = DXL_ERROR_RANGE;
	uint8_t data[] = "hello";
	uint8_t nb_available_in_data = 5;
	uint8_t nb_param_to_send = 7;
	uint16_t return_delay_us = 0;

	// calls before initialization should not crash, just return false
	DxldHALWriteByte_debug('0', true);
	EXPECT_FALSE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_param_to_send, return_delay_us));
	Delay_ms(10);

	DxldHALInit(DXLB_BAUDRATE_1000000);

	DxldHALWriteByte_debug('1', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_param_to_send, return_delay_us)); // "hello\0\0\0"
	Delay_ms(10);

	DxldHALWriteByte_debug('2', true);
	EXPECT_FALSE(DxldHALStatusPacket(dxl_id,  error, NULL, nb_available_in_data, nb_param_to_send, return_delay_us));
	Delay_ms(10);

	DxldHALWriteByte_debug('3', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, 0, nb_param_to_send, return_delay_us));					// full of zeros
	Delay_ms(10);

	DxldHALWriteByte_debug('4', true);
	EXPECT_FALSE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, 255, return_delay_us));
	Delay_ms(10);

	DxldHALWriteByte_debug('5', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, 0, return_delay_us));				// empty
	Delay_ms(10);

	DxldHALWriteByte_debug('6', true);
	EXPECT_FALSE(DxldHALStatusPacket(0xFF  ,  error, data, nb_available_in_data, nb_param_to_send, return_delay_us));
	Delay_ms(10);

	DxldHALWriteByte_debug('7', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_available_in_data-1, return_delay_us));// "hell"
	Delay_ms(10);

	DxldHALWriteByte_debug('8', true);
	EXPECT_FALSE(DxldHALStatusPacket(dxl_id,  0xFF,  data, nb_available_in_data, nb_param_to_send, return_delay_us));

	DxldHALDeInit();
	Delay_ms(10);
}

extern uint8_t  tx_buffer[];

void test_checksum(){
	uint8_t dxl_id = 1;
	dxld_error_t error = 2; // read
	uint8_t data[] = { 0, 0x03 };
	uint8_t nb_available_in_data = 2;
	uint8_t nb_param_to_send = 2;

	// Simulate a read of the model and firmware version of the servo 1. (cf http://support.robotis.com/en/product/dynamixel/communication/dxl_instruction.htm)
	//Expected checksum is 0xF5

	DxldHALInit(DXLB_BAUDRATE_1000000);
	DxldHALWriteByte_debug('9', true);
	DxldHALStatusPacket(dxl_id,  error,  data, nb_available_in_data, nb_param_to_send, 0);
	EXPECT_VALUE(0xF5, tx_buffer[7]);

	DxldHALDeInit();
	Delay_ms(10);
}

static void test_DxldHALDataAvailable(){

	EXPECT_FALSE( DxldHALDataAvailable() );

	DxldHALWriteByte_debug(100, false);
	DxldHALInit(DXLB_BAUDRATE_1000000);
	EXPECT_FALSE( DxldHALDataAvailable() );
	DxldHALDeInit();


	DxldHALInit(DXLB_BAUDRATE_1000000);
	// put some bytes artificially...
	DxldHALWriteByte_debug(100, false);
	EXPECT_TRUE( DxldHALDataAvailable() );
	EXPECT_TRUE( DxldHALDataAvailable() );
	EXPECT_TRUE( DxldHALDataAvailable() );

	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_RX_RS));//clear RX FIFO
	EXPECT_FALSE( DxldHALDataAvailable() );

	DxldHALDeInit();
	Delay_ms(10);
}

//static void test_DxldHALGetByte(){
//	EXPECT_VALUE(-1, DxldHALGetByte() );
//
//	DxldHALWriteByte_debug(100, false);
//	DxldHALInit(DXLB_BAUDRATE_1000000);
//	EXPECT_VALUE(-1, DxldHALGetByte() );
//	DxldHALDeInit();
//
//	DxldHALInit(DXLB_BAUDRATE_1000000);
//	DxldHALWriteByte_debug(100, false);
//	EXPECT_VALUE(100, DxldHALGetByte() );
//	EXPECT_VALUE(-1, DxldHALGetByte() );
//
//	DxldHALWriteByte_debug(100, false);
//	Chip_UART_SetupFIFOS(LPC_USART, (UART_FCR_FIFO_EN | UART_FCR_RX_RS));//clear RX FIFO
//	EXPECT_VALUE(-1, DxldHALGetByte() );
//
//	DxldHALDeInit();
//	Delay_ms(10);
//
//}

void DxldReturnDelay_Handler(uint16_t return_delay_us){
	Delay_us(return_delay_us);
}


static void test_return_delay(){
	uint8_t dxl_id = 7;
	dxld_error_t error = DXL_ERROR_CHECKSUM;
	uint8_t data[] = "dxld";
	uint8_t nb_available_in_data = 4;
	uint8_t nb_param_to_send = 4;

	DxldHALInit(DXLB_BAUDRATE_1000000);

	DxldHALWriteByte_debug('a', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_param_to_send, 0)); // "dxld"
	Delay_ms(10);

	DxldHALWriteByte_debug('b', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_param_to_send, 100)); // "dxld"
	Delay_ms(10);

	DxldHALWriteByte_debug('c', true);
	EXPECT_TRUE(DxldHALStatusPacket(dxl_id,  error, data, nb_available_in_data, nb_param_to_send, 1000)); // "dxld"
	Delay_ms(10);

	DxldHALDeInit();

}

void run_test_hal(){
	test_DxldHALInit();
	test_DxldHALStatusPacket();
	test_checksum();
	test_DxldHALDataAvailable();
	//test_DxldHALGetByte();
	test_return_delay();
}
