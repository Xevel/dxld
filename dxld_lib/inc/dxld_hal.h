/*
 * dxld_hal.h
 *
 *  Created on: 4 févr. 2015
 *      Author: Xevel
 */

#ifndef DXLD_LIB_INC_DXLD_HAL_H_
#define DXLD_LIB_INC_DXLD_HAL_H_

#include <dxld_lib.h>

//returns the actual baud rate in bps, or 0 if dxl_baud is invalid.
uint32_t DxldHALInit(dxld_baudrate_t dxl_baud);
void     DxldHALDeInit();

// returns false if failed
uint32_t DxldHALStatusPacket(uint8_t dxl_id, dxld_error_t error, uint8_t* data, uint8_t nb_available_in_data, uint8_t nb_param_to_send, uint16_t return_delay);

bool DxldHALBaudrateIsValid(uint8_t dxl_baud);

//return the next byte from RX or -1 if no data was available.
int32_t  DxldHALGetByte();
uint32_t DxldHALDataAvailable();

void DxldHalSetNbToIgnore(uint8_t nb_to_ignore);

//----------- debug functions-------------

void DxldHALWriteByte_debug(uint8_t c, uint32_t return_to_rx);
bool DxldHALCheckEcho_debug();

#endif /* DXLD_LIB_INC_DXLD_HAL_H_ */
