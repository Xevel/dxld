/*
 * dxld_parse.c
 *
 *  Created on: 4 févr. 2015
 *      Author: Xevel
 */
#include "dxld_parse.h"
#include "dxld_hal.h"


static struct {
	dxld_endpoint_t* endpoint; 	// which endpoint is this dedicated to. NULL if it's a broadcast
	uint8_t id;					// dynamixel ID of the packet being received
	uint8_t nb_expected_params; // length-2
	uint8_t instruction;
	uint8_t params[DXLD_RX_BUFFER_SIZE];
} dxld_packet;


typedef enum {
	DXLD_SEARCH_FIRST_FF = 0,
	DXLD_SEARCH_SECOND_FF,
	DXLD_SEARCH_ID,
	DXLD_SEARCH_LENGTH,
	DXLD_SEARCH_COMMAND,
	DXLD_GET_PARAMETERS,
	DXLD_GET_SYNC_WRITE_PARAMETERS,
} dxld_parser_state_t ;

#define dxld_state LPC_USART->SCR  // state of the Dynamixel parser state machine
unsigned int dxld_parser_rx_timer; // TODO increment regularly




static uint8_t Parse_Checksum(){
	unsigned int checksum = dxld_packet.id + dxld_packet.nb_expected_params+2 + dxld_packet.instruction;
	for (int i = 0; i < dxld_packet.nb_expected_params; i++ ){
		checksum += dxld_packet.params[i];
	}
	return (uint8_t)(~checksum);
}




static void Parse_SendStatusPacket(dxld_endpoint_t* endpoint, dxld_error_t error, uint8_t return_level){
	if ( dxld_packet.id != DXL_ID_BROADCAST ){
		DxldEndpointStatusPacketEmpty(endpoint, error, return_level);
	}
}


static void DxldManageSyncWrite(){
	if( dxld_packet.nb_expected_params >= 4 ){
		uint8_t start_addr = dxld_packet.params[0];
		uint8_t nb_to_write = dxld_packet.params[1];

		if ( nb_to_write >= dxld_packet.nb_expected_params - 3){

			// hop from the start of one subpacket to the next.
			// If there is some data at the end that do not amount to a full subpacket, it is ignored
			for (unsigned int i = 2; i < dxld_packet.nb_expected_params - nb_to_write; i += nb_to_write + 1){
				uint8_t dxl_id =  dxld_packet.params[i];
				uint8_t* dxl_data =  &dxld_packet.params[i+1];

				dxld_endpoint_t* endpoint = DxldEndpointGetFromDynamixelID(dxl_id);

				// ignore subpackets that are not for any endpoint, and those addressed to broadcast too
				if ( endpoint != NULL && dxl_id != DXL_ID_BROADCAST ){
					dxld_error_t error = DxldEndpointWriteCheck(endpoint, start_addr, dxl_data, nb_to_write, false, false);
					if (error == DXL_ERROR_NONE){
						DxldEndpointWriteAfterCheck( endpoint, start_addr, dxl_data, nb_to_write, false, false );
					}
				}
			}
		}
	}
}



static void Parse_ManageBulkRead(){
	//TODO
}



static void Parse_LocalRead(dxld_endpoint_t* endpoint){
	if ( dxld_packet.id != DXL_ID_BROADCAST ){
		uint8_t start_addr = dxld_packet.params[0];
		uint8_t nb_to_read = dxld_packet.params[1];

		if (dxld_packet.nb_expected_params == 2){
			DxldEndpointStatusPacketRead(
					endpoint,
					DXL_ERROR_NONE,
					start_addr,
					nb_to_read,
					STATUS_RETURN_READ);
		} else {
			DxldEndpointStatusPacketEmpty(endpoint, DXL_ERROR_INSTRUCTION, STATUS_RETURN_READ);
		}
	}
}


// writing to endpoint (both tables)
static void Parse_LocalWrite(dxld_endpoint_t* endpoint){
	if (dxld_packet.nb_expected_params >= 2){
		uint8_t  start_addr  =  dxld_packet.params[0];
		uint8_t* data        = &dxld_packet.params[1];
		uint8_t  nb_to_write =  dxld_packet.nb_expected_params - 1;
		uint32_t internal_use = false;
		uint32_t write_to_reg = (dxld_packet.instruction == DXL_CMD_REG_WRITE);


		dxld_error_t error = DxldEndpointWriteCheck(endpoint, start_addr, data, nb_to_write, internal_use, write_to_reg);
		Parse_SendStatusPacket(endpoint, error, STATUS_RETURN_ALL);
		if (error == DXL_ERROR_NONE){
			DxldEndpointWriteAfterCheck(endpoint, start_addr, data, nb_to_write, internal_use, write_to_reg);
		}
	} else {
		Parse_SendStatusPacket(endpoint, DXL_ERROR_INSTRUCTION, STATUS_RETURN_ALL);
	}
}





// Currently, all handled errors are only for the last command.
// If more errors are added, then it might be needed to composite errors.


// check what the packet means, verify it's valid, and do it
static void Parse_ManageValidPacket(dxld_endpoint_t* endpoint){
	switch (dxld_packet.instruction){
		case DXL_CMD_PING:
			// PING returns a status packet whatever the ID (mine or broadcast) and whatever the Status Return Level
			DxldEndpointStatusPacketEmpty(endpoint, DXL_ERROR_NONE, STATUS_RETURN_PING);
			break;

		case DXL_CMD_READ_DATA:
			Parse_LocalRead(endpoint);
			break;

		case DXL_CMD_WRITE_DATA:
		case DXL_CMD_REG_WRITE:
			Parse_LocalWrite(endpoint);
			break;

		case DXL_CMD_ACTION:
			if ( DxldEndpointIsRegistered(endpoint) ){
				// the values have already been checked before being written in the temp table, so we only need to apply them.
				// potentially long operation, performed after sending the return packet
				Parse_SendStatusPacket(endpoint, DXL_ERROR_NONE, STATUS_RETURN_ALL);
				DxldEndpointCommitRegistered(endpoint);
			} else {
				Parse_SendStatusPacket(endpoint, DXL_ERROR_INSTRUCTION, STATUS_RETURN_ALL);
			}
			break;

		case DXL_CMD_RESET:
			// potentially long operation, performed after sending the return packet
			Parse_SendStatusPacket(endpoint, DXL_ERROR_NONE, STATUS_RETURN_ALL);
			DxldEndpointLoadDefault(endpoint);
			break;

		case DXL_CMD_BOOTLOAD:
			// potentially long operation, performed after sending the return packet
			Parse_SendStatusPacket(endpoint, DXL_ERROR_NONE, STATUS_RETURN_ALL);
			DxldRunBootloader_Handler(endpoint);
			break;

		default:
			Parse_SendStatusPacket(endpoint, DXL_ERROR_INSTRUCTION, STATUS_RETURN_ALL);
			break;
	}
}




void DxldParse(uint8_t c){
	dxld_parser_rx_timer = 0;

	static uint8_t params_count = 0;
	uint32_t dxld_state_local = dxld_state;


	switch(dxld_state_local){
		case DXLD_SEARCH_FIRST_FF:
			if (c == 0xFF){
				dxld_state_local = DXLD_SEARCH_SECOND_FF;
			}
			break;

		case DXLD_SEARCH_SECOND_FF:
			if (c == 0xFF){
				dxld_state_local = DXLD_SEARCH_ID;
			} else {
				dxld_state_local = DXLD_SEARCH_FIRST_FF;
			}
			break;

		case DXLD_SEARCH_ID:
			if (c != 0xFF){
				dxld_packet.id = c;
				dxld_state_local = DXLD_SEARCH_LENGTH;
			}  // stay in this state if there is a third 0xFF
			break;

		case DXLD_SEARCH_LENGTH:
			if ( ( dxld_packet.id == DXL_ID_BROADCAST || (dxld_packet.endpoint = DxldEndpointGetFromDynamixelID(dxld_packet.id)) != NULL )
					&& c > 1
					&& c < DXLD_RX_BUFFER_SIZE){ // accept messages that fit in the buffer, ignore otherwise
				dxld_packet.nb_expected_params = c - 2;
				params_count = 0;
				dxld_state_local = DXLD_SEARCH_COMMAND;
			} else {
				DxldHalSetNbToIgnore(c);
				dxld_state_local = DXLD_SEARCH_FIRST_FF;
			}
			break;

		case DXLD_SEARCH_COMMAND:
			dxld_packet.instruction = c;
			dxld_state_local = DXLD_GET_PARAMETERS;
			break;

		case DXLD_GET_PARAMETERS:
			if (params_count >= dxld_packet.nb_expected_params){
				// it's the checksum
				if ( c == Parse_Checksum() ){
					DxldValidPacketReceived_Handler(dxld_packet.id, dxld_packet.instruction, dxld_packet.params, params_count);
					if ( dxld_packet.id == DXL_ID_BROADCAST ){
						if( dxld_packet.instruction == DXL_CMD_SYNC_WRITE ){
							DxldManageSyncWrite();
						} else if ( dxld_packet.instruction == DXL_CMD_BULK_READ ){
							Parse_ManageBulkRead();
						} else {
							dxld_endpoint_t* endpoint = DxldEndpointIteratorFirst();
							while ( endpoint != NULL ){
								Parse_ManageValidPacket(endpoint);
								endpoint = DxldEndpointIteratorNext();
							}
						}
					} else {
						Parse_ManageValidPacket(dxld_packet.endpoint);
					}
					dxld_state_local = DXLD_SEARCH_FIRST_FF;
				} else {
					Parse_SendStatusPacket(dxld_packet.endpoint, DXL_ERROR_CHECKSUM, STATUS_RETURN_ALL);
					if (c == 0xFF){
						dxld_state_local = DXLD_SEARCH_SECOND_FF;
					} else {
						dxld_state_local = DXLD_SEARCH_FIRST_FF;
					}
				}
			} else {
				// it's a parameter
				dxld_packet.params[params_count++] = c;
			}
			break;

		default:
			break;
	}

	// Timeout on state machine while waiting on further data
	if( dxld_state_local != DXLD_SEARCH_FIRST_FF && dxld_parser_rx_timer > DXL_RECEIVE_TIMEOUT ){
		dxld_state_local = DXLD_SEARCH_FIRST_FF;
	}

	LPC_USART->SCR = dxld_state_local;

}


