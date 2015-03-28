/*
 * dxl_device_test.h
 *
 *  Created on: 8 févr. 2015
 *      Author: Xevel
 */

#ifndef DXL_DEVICE_TEST_H_
#define DXL_DEVICE_TEST_H_
#include "chip.h"

#define TIMER_MS  		LPC_TIMER32_1
#define TIMER_MS_HZ 	1000

void Delay_us (uint32_t us);// Hacked for 48MHz only. Not very precise.
void Delay_ms (uint32_t delay_duration_ms);

void fatal_error();

#define EXPECT_TRUE(x)  {if (!x){ fatal_error();}}
#define EXPECT_FALSE(x)  {if (x){ fatal_error();}}
#define EXPECT_BETWEEN_UINT32(lower, upper, x)  { uint32_t res = x; if (res >= upper || res <= lower ){ fatal_error();}}
#define EXPECT_VALUE(val, x) {if (x!=val){fatal_error();}}


#endif /* DXL_DEVICE_TEST_H_ */
