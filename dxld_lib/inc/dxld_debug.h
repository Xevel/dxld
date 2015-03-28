/*
 * dxld_debug.h
 *
 *  Created on: 9 févr. 2015
 *      Author: Xevel
 */

#ifndef DXLD_DEBUG_H_
#define DXLD_DEBUG_H_

__attribute__ ((weak)) void DxldFatalError_Handler();


#if DXLD_DEBUG

#define D_DXLD_EXPECT_TRUE(x)  { if(!x){ DxldFatalError_Handler(); }}
#define D_DXLD_EXPECT_FALSE(x)  { if(x){ DxldFatalError_Handler(); }}
#define D_DXLD_EXPECT_VALUE(val, x) {if (x!=val){DxldFatalError_Handler();}}
#define D_DXLD_EXPECT_BETWEEN_UINT32(lower, upper, x)  { uint32_t res = x; if (res >= upper || res <= lower ){ DxldFatalError_Handler();}}

#else

#define D_DXLD_EXPECT_TRUE(x)
#define D_DXLD_EXPECT_FALSE(x)
#define D_DXLD_EXPECT_VALUE(val, x)
#define D_DXLD_EXPECT_BETWEEN_UINT32(lower, upper, x)

#endif

#endif /* DXLD_DEBUG_H_ */
