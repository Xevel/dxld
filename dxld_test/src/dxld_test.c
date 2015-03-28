/*
===============================================================================
 Name        : dxl_device_test.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/


#include "dxld_test.h"
#include "chip.h"
#include <cr_section_macros.h>

#include <dxld_lib.h>
#include <dxld_endpoint.h>
#include <dxld_hal.h>

#include <test_hal.h>
#include "test_dxld_lib.h"
#include "test_endpoint.h"
#include "test_parse.h"



void fatal_error(){
	while(1);
}


// Hacked for 48MHz only. Not very precise.
void Delay_us (uint32_t us){
	us = us * 4;
	while ( us ){
		__NOP();
		us--;
	}
}

void Delay_ms (uint32_t delay_duration_ms) {
    uint32_t delay_end = Chip_TIMER_ReadCount(TIMER_MS) + delay_duration_ms;
    while ( Chip_TIMER_ReadCount(TIMER_MS) < delay_end ) ;
}


#define TEST_Delay_us  (0)

int main(void) {
	// Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
//    // Set up and initialize all required blocks and
//    // functions related to the board hardware
//    Chip_Init();

	// use 32 bits timer to count ms directly. Overflows in about 49 days
	Chip_TIMER_Init(       TIMER_MS);
	Chip_TIMER_PrescaleSet(TIMER_MS, Chip_Clock_GetMainClockRate() / TIMER_MS_HZ);
	Chip_TIMER_Reset(      TIMER_MS);
	Chip_TIMER_Enable(     TIMER_MS);

#if (TEST_Delay_us)
    Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 1);
    while(1) {
    	Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 1);
    	Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 1);
    	Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 0);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, top);
		Delay_us (30000);
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 1, 0);
		Delay_us (100);
	}
#else
    while(1) {
    	run_test_hal();
		run_test_dxld_lib();

		// TODO endpoint and parse
    	Delay_ms(100);
    }
#endif

    return 0 ;
}
