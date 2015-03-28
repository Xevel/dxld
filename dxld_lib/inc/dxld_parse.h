/*
 * dxld_parse.h
 *
 *  Created on: 30 janv. 2015
 *      Author: Xevel
 */
// TODO license
 
#ifndef DXLD_LIB_INC_DXLD_PARSE_H_
#define DXLD_LIB_INC_DXLD_PARSE_H_

#include "chip.h"
#include "dxld_endpoint.h"


#define DXLD_RX_BUFFER_SIZE		144 // Dynamixel servos have 143, so anything bigger is probably useless
#define DXL_RECEIVE_TIMEOUT 	100 // TODO unit? value? should be 100ms like the servos


#define DXL_ID_BROADCAST     0xFE

// Dynamixel instructions. See http://support.robotis.com/en/product/dynamixel/dxl_communication.htm
#define DXL_CMD_PING         0x01
#define DXL_CMD_READ_DATA    0x02
#define DXL_CMD_WRITE_DATA   0x03
#define DXL_CMD_REG_WRITE    0x04
#define DXL_CMD_ACTION       0x05
#define DXL_CMD_RESET        0x06
#define DXL_CMD_BOOTLOAD     0x08
#define DXL_CMD_SYNC_WRITE   0x83
#define DXL_CMD_BULK_READ	 0x92 // See Darwin-OP code base, especially Framework/src/CM730.h/cpp

// status return level
#define STATUS_RETURN_PING		0x00
#define STATUS_RETURN_READ		0x01
#define STATUS_RETURN_ALL		0x02


void DxldParse(uint8_t c);


#endif /* DXLD_LIB_INC_DXLD_PARSE_H_ */
