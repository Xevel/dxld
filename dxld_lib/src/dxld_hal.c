/*
 * dxld_hal.c
 *
 *  Created on: 4 févr. 2015
 *      Author: Xevel
 */
#include "dxld_hal.h"
#include "dxld_endpoint.h"
#include "chip.h"
#include "string.h"
#include "RingBuffer.h"
#include "dxld_parse.h" // TODO TMP

#define DXLD_UART  LPC_USART
#define DXLD_TX_EN_PORT 0 // TX_EN: P0_1, active low
#define DXLD_TX_EN_PIN  1
#define DXLD_MODE_RX	1
#define DXLD_MODE_TX	0
#define DXLD_RX_INT_LVL UART_FCR_TRG_LEV1 //TODO check at @ 3Mbps
#define DXLD_INT_MASK   (UART_IER_RBRINT) // use RX RDA / CTI interrupts only (both activated with RBRINT)

#define DXLD_TX_BUFFER_SIZE 128
uint8_t tx_buffer[DXLD_TX_BUFFER_SIZE];

static uint8_t HAL_initialized = false;

// number of incoming bytes that we can ignore directly
// as being part of a packet we know is not for us
uint8_t nb_bytes_to_ignore;


// TODO speed up important parts:
// - remove unnecessary checks
// use specialized functions to interact with the UART faster (simplify functions to exploit the specificities of the hald-duplex dynamixel bus)



bool DxldHALBaudrateIsValid(uint8_t dxl_baud){
	return     dxl_baud == DXLB_BAUDRATE_3000000
			|| dxl_baud == DXLB_BAUDRATE_1000000
			|| dxl_baud == DXLB_BAUDRATE_500000
			|| dxl_baud == DXLB_BAUDRATE_200000
			|| dxl_baud == DXLB_BAUDRATE_115200
			|| dxl_baud == DXLB_BAUDRATE_57600
			|| dxl_baud == DXLB_BAUDRATE_19200
			|| dxl_baud == DXLB_BAUDRATE_9600;
}


// Init hardware, returns baudrate in bps
uint32_t DxldHALInit(dxld_baudrate_t dxl_baud){
	// pre-computed values for a 48MHz clock using baudrate_finder.py
	uint16_t divisor_latch, divadd_val, mul_val;

	// Dynamixel servos, due to the detail of their implementation, do not use the exact
	// standard baud rates, and the masters (USB2Dynamixel, USB2AX, OpenCM...) have similar differences.
	// We target the middle value between the FTDI (USB2Dynamixel) and the servos.
	switch (dxl_baud){
		case DXLB_BAUDRATE_9600:
			//baudrate: servos/usb2ax 9615, FTDI 9600, this 9607
			divisor_latch = 229;
			divadd_val = 4;
			mul_val = 11;
			break;
		case DXLB_BAUDRATE_19200:
			//baudrate: servos/usb2ax 19231, FTDI 19200, this 19215
			divisor_latch = 145;
			divadd_val = 1;
			mul_val = 13;
			break;
		case DXLB_BAUDRATE_57600:
			//baudrate: servos/usb2ax 57142, FTDI 57692, this 57416
			divisor_latch = 33;
			divadd_val = 7;
			mul_val = 12;
			break;
		case DXLB_BAUDRATE_115200:
			//baudrate: servos/usb2ax 117647, FTDI 115384, this 116788
			divisor_latch = 15;
			divadd_val = 5;
			mul_val = 7;
			break;
		case DXLB_BAUDRATE_200000:
			//baudrate: servos/usb2ax 200000, FTDI ?, this 200000
			divisor_latch = 8;
			divadd_val = 7;
			mul_val = 8;
			break;
		case DXLB_BAUDRATE_500000:
			//baudrate: servos/usb2ax 500000, FTDI ?, this 500000
			divisor_latch = 4;
			divadd_val = 1;
			mul_val = 2;
			break;
		case DXLB_BAUDRATE_1000000:
			//baudrate: servos/usb2ax 1000000, FTDI 1000000, this 1000000
			divisor_latch = 3;
			divadd_val = 0;
			mul_val = 1;
			break;
		case DXLB_BAUDRATE_3000000:
			//baudrate = servos/usb2mx 3000000, FTDI none, this 3000000
			divisor_latch = 1;
			divadd_val = 0;
			mul_val = 1;
			break;
		default:
			return 0;
			break;
	}
	// Return actual baud rate
	uint32_t baudrate = 48000000UL / (16 * divisor_latch + 16 * divisor_latch * divadd_val / mul_val);

	// set TX_EN (P0_1) to output, starting in RX mode
	Chip_GPIO_SetPinState(    LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN, DXLD_MODE_RX);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN);

	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_UART0);
	Chip_Clock_SetUARTClockDiv(1);
	Chip_UART_ConfigData(DXLD_UART, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS)); // Setup UART for 8N1

	// Update UART registers
	Chip_UART_EnableDivisorAccess( DXLD_UART);
	Chip_UART_SetDivisorLatches(   DXLD_UART, UART_LOAD_DLL(divisor_latch), UART_LOAD_DLM(divisor_latch));
	Chip_UART_DisableDivisorAccess(DXLD_UART);

	// Set fractional divider
	DXLD_UART->FDR = (UART_FDR_MULVAL(mul_val) | UART_FDR_DIVADDVAL(divadd_val));

	Chip_UART_TXDisable(DXLD_UART);

	HAL_initialized = true;
	tx_buffer[0] = 0xFF;
	tx_buffer[1] = 0xFF;

	Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS | DXLD_RX_INT_LVL)); // reset and enable FIFOs

	Chip_UART_IntEnable(LPC_USART, DXLD_INT_MASK);
	NVIC_SetPriority(UART0_IRQn, 0);
	NVIC_EnableIRQ(UART0_IRQn);


	return baudrate;
}

// undo initialization
void DxldHALDeInit(){
	NVIC_DisableIRQ(UART0_IRQn);
	HAL_initialized = false;
	Chip_UART_DeInit(DXLD_UART);
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN);
}


static inline void HAL_SetTX(){
	Chip_UART_SetRS485Flags(DXLD_UART, UART_RS485CTRL_RX_DIS); // disable RX
	Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_TX_RS | DXLD_RX_INT_LVL)); // Clear TX FIFO
	Chip_UART_TXEnable(DXLD_UART);
	Chip_GPIO_SetPinState(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN, DXLD_MODE_TX);
}


static inline void HAL_SetRX(){
	while ( (Chip_UART_ReadLineStatus(DXLD_UART) & UART_LSR_TEMT) == 0 ); // wait for all bytes sent
	// TEMT is set when only 50% of the stop bit is sent, but the line logic level is High,
	// so we can go on without fear of spurious signal

	Chip_UART_TXDisable(DXLD_UART);
	Chip_GPIO_SetPinState(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN, DXLD_MODE_RX);
	Chip_UART_ClearRS485Flags(DXLD_UART, UART_RS485CTRL_RX_DIS); //enable RX
}


#define DXL_PACKET_ID 		2
#define DXL_PACKET_LENGTH 	3
#define DXL_PACKET_ERROR 	4
#define DXL_PACKET_PARAMS 	5


static uint32_t HAL_MakeTXPacket(uint8_t dxl_id, dxld_error_t err, uint8_t* data, uint8_t nb_available_in_data, uint8_t nb_param_to_send){
	// create the packet in a buffer while all interrupts are disabled to avoid incoherent values
	uint8_t length = 2 + nb_param_to_send; // value of the length byte in the packet
	uint16_t checksum = dxl_id + length + err;

	tx_buffer[DXL_PACKET_ID]     = dxl_id;
	tx_buffer[DXL_PACKET_LENGTH] = length;
	tx_buffer[DXL_PACKET_ERROR]  = err;

	uint8_t nb_to_copy = MIN( nb_available_in_data, nb_param_to_send );

	// Copy as much data as possible in the buffer. All interrupts disabled to avoid incoherent values
	uint32_t primask = __get_PRIMASK(); // save global interrupt enable state
	__disable_irq();
	memcpy( tx_buffer + DXL_PACKET_PARAMS, data, nb_to_copy );
	__set_PRIMASK(primask); //restore global interrupt state

	// pad with zeros if needed
	memset( tx_buffer + DXL_PACKET_PARAMS + nb_to_copy, 0, nb_param_to_send - nb_to_copy );

	for (unsigned int i = DXL_PACKET_PARAMS; i < DXL_PACKET_PARAMS + nb_to_copy ; i++ ){
		checksum += tx_buffer[i];
	}
	tx_buffer[DXL_PACKET_PARAMS + nb_param_to_send] = (uint8_t)(~checksum);

	return nb_param_to_send + 6;  // includes header and checksum
}



uint32_t DxldHALStatusPacket(uint8_t dxl_id, dxld_error_t err, uint8_t* data, uint8_t nb_available_in_data, uint8_t nb_param_to_send, uint16_t return_delay_us){
	if (HAL_initialized  // superfluous?
			&& dxl_id != 0xFF  // superfluous?
			&& (err & 0x80) == 0  // superfluous?
			&& !(nb_param_to_send != 0 && data == NULL)  // superfluous?
			&& nb_param_to_send < DXLD_TX_BUFFER_SIZE - 6){
		// Given the half-duplex nature of the bus and the slave status of the device, we know
		// that the TX buffer is empty and that everything has to be written on the bus before
		// we can get any more action from the Dynamixel side.

		uint8_t nb_to_transmit = HAL_MakeTXPacket(dxl_id, err, data, nb_available_in_data, nb_param_to_send);
		DxldReturnDelay_Handler(return_delay_us);
		HAL_SetTX();
		Chip_UART_SendBlocking(DXLD_UART, tx_buffer, nb_to_transmit);
		HAL_SetRX();

		DxldPacketSent_Handler( dxl_id, err, tx_buffer + DXL_PACKET_PARAMS, nb_param_to_send);

		return true;
	} else {
		return false;
	}
}

void Hal_InternalRxError(){
	dxld_endpoint_t* endpoint = DxldEndpointIteratorFirst();
	while ( endpoint != NULL ){
		DxldInternalErrorSet( endpoint, DXLD_INTERNAL_ERROR_RX );
		endpoint = DxldEndpointIteratorNext();
	}

	Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_RX_RS | DXLD_RX_INT_LVL));
}

inline uint32_t DxldHALDataAvailable(){
	// reading line status tells us if data is available,
	// but also if errors occurred. Reading it clears the errors so if we want to handle them it has to be here
	uint32_t line_status = Chip_UART_ReadLineStatus(DXLD_UART);

	// Let errors be discovered by the checksum
//	if ( (line_status & (UART_LSR_OE | UART_LSR_FE | UART_LSR_BI)) != 0 ){
//		Hal_InternalRxError();
//		return 0;
//	}

	return line_status & UART_LSR_RDR ;
}

//TODO rebuild that part to make polling possible...
//int32_t DxldHALGetByte(){
//	if ( ! RingBufferByte_IsEmpty(rx_ringbuffer) ){
//		uint8_t c = RingBufferByte_Remove(rx_ringbuffer);
//		return (int32_t) c;
//	} else {
//		return -1;
//	}
//}


void DxldHalSetNbToIgnore(uint8_t nb_to_ignore){
	nb_bytes_to_ignore = nb_to_ignore;
}

void UART_IRQHandler (){
	// only sources of USART interrupts are the ones that say there is data available : CTI and RDA
	// Both are cleared by reading data from the FIFO
	//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0,8); // TMP TODO

	while ( DxldHALDataAvailable() ){
		//Chip_GPIO_SetPinOutHigh(LPC_GPIO, 0,8); // TMP TODO

		uint8_t c = Chip_UART_ReadByte(DXLD_UART);

		if (nb_bytes_to_ignore){
			nb_bytes_to_ignore--;
		} else {
			DxldParse(c);
		}
		//Chip_GPIO_SetPinOutLow(LPC_GPIO, 0,8);// TMP TODO
	}

	// Potential spurious pending interrupt:
	// see AN10414.pdf : "Handling of spurious interrupts in the LPC2000"
	// Here the RDA/CTI interrupt could be made pending if immediately after we read the first byte
	// in the FIFO another one arrives. Even though the FIFO would then be emptied, making RDR = 0,
	// the interrupt pending flag would remain...


	//Chip_GPIO_SetPinOutLow(LPC_GPIO, 0,9);// TMP TODO

}


//----------------------- TEST FUNCTIONS --------------------------

// For testing. Writes a byte and waits for it to be completely output.
void DxldHALWriteByte_debug(uint8_t c, uint32_t return_to_rx){
	if (HAL_initialized){
		Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_TX_RS | UART_FCR_RX_RS | DXLD_RX_INT_LVL)); // Clear TX FIFO
		Chip_UART_TXEnable(DXLD_UART);
		Chip_GPIO_SetPinState(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN, DXLD_MODE_TX);
		while ( (Chip_UART_ReadLineStatus(DXLD_UART) & UART_LSR_THRE) == 0 ); // wait for free space in TX fifo
		Chip_UART_SendByte(DXLD_UART, c);
		while ( (Chip_UART_ReadLineStatus(DXLD_UART) & UART_LSR_TEMT) == 0  ); // wait for all bytes sent
		if(return_to_rx){
			HAL_SetRX();
			Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_TX_RS | UART_FCR_RX_RS | DXLD_RX_INT_LVL)); // Clear TX FIFO
		}
	}
}

// For hardware testing. Writes stuff and checks that it can be read on RX
bool DxldHALCheckEcho_debug(){
	if (HAL_initialized){
		NVIC_DisableIRQ(UART0_IRQn);

		Chip_UART_SetupFIFOS(DXLD_UART, (UART_FCR_FIFO_EN | UART_FCR_TX_RS | UART_FCR_RX_RS)); // Clear TX and RX FIFO
		Chip_GPIO_SetPinState(LPC_GPIO, DXLD_TX_EN_PORT, DXLD_TX_EN_PIN, DXLD_MODE_TX); // set pin direction to TX

		uint8_t test_data[] = "echo test\r\n";

		for (int i = 0; i < sizeof(test_data); i++ ){
			Chip_UART_SendByte(DXLD_UART, test_data[i]);
		}
		while ( (Chip_UART_ReadLineStatus(DXLD_UART) & UART_LSR_TEMT) == 0  ); // wait for all bytes sent

		bool success = true;
		for (int i = 0; i < sizeof(test_data); i++ ){
			if ( Chip_UART_ReadByte(DXLD_UART) != test_data[i] ){
				success = false;
				break;
			}
		}
		HAL_SetRX();
		NVIC_EnableIRQ(UART0_IRQn);
		return success;
	}
	return false;
}


