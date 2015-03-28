/*
 * test_parse.c
 *
 *  Created on: 8 févr. 2015
 *      Author: Xevel
 */
#include "test_parse.h"
#include "chip.h"
#include "dxld_parse.h"



//TODO !

uint8_t packet_1[] = {0xff, 0xff, 0x01, 0x02, 0x01, 0xfb}; // ping to ID1

uint8_t packet_2[] = {0xff, 0xff, 0x00, 0x02, 0x06, 0xf7}; // reset to ID0
// sync_write Dynamixel actuator with an ID of 0: to position 0x010 with a speed of 0x150
// Dynamixel actuator with an ID of 1: to position 0x220 with a speed of 0x360
// Dynamixel actuator with an ID of 2: to position 0x030 with a speed of 0x170
// Dynamixel actuator with an ID of 0: to position 0x220 with a speed of 0x380

uint8_t packet_3[] = {0xff, 0xff, 0xfe, 0x18, 0x83, 0x1e, 0x04,
		0x00, 0x10, 0x00, 0x50, 0x01,
		0x01, 0x20, 0x02, 0x60, 0x03,
		0x02, 0x30, 0x00, 0x70, 0x01,
		0x03, 0x20, 0x02, 0x80, 0x03,
		0x12};

void spoof_dxl_communication( uint8_t* packet, uint8_t nb_bytes){
	for (unsigned int i = 0; i < nb_bytes; i++){
		DxldParse(packet[i]);
	}
}


void run_test_parse(){
	//    	spoof_dxl_communication(packet_1, sizeof(packet_1));
	//    	spoof_dxl_communication(packet_2, sizeof(packet_2));
	//    	spoof_dxl_communication(packet_3, sizeof(packet_3));

}



