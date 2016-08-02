//
//////////////////////////////////////////////////////////////////////////
// File: rk_global.h
// Desc: Global definition
// 
// Date: Revised by yousf 20160721
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_UTILS_H
#define _RK_UTILS_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rk_typedef.h"              // Type definition


// RK3288+VIVO Params Setting
#define     RK_MAX_FILE_NUM	        10              // max input images

#ifndef RAW_IMG_BUF_COUNT
    #define RAW_IMG_BUF_COUNT       6               // Input 
#endif

// XM4
#define     XM4_DEBUG               0               // 1/0 for CEVA_XM4 IDE Debug-View

//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition
//---- Debug Params Setting
#define     MY_DEBUG_PRINTF         0				// for printf()        1 or 0
#define     MY_DEBUG_WRT2RAW        0				// for WriteRawData()  1 or 0
#define     MY_DEBUG_WRT_DST        1				// for Write Dst...    1 or 0

#define     USE_DEFECT_PIXEL        0//1               // 1-Use Defect Pixel, 0-Not Use

//---- Memory Params Setting
#define     DDR_MEM_SIZE            134217728       // DDR memory size: 128MB = 128*1024*1024 = 134217728 Byte
#define     DSP_MEM_SIZE            131072          // DSP memory size: 128KB = 128*1024      =    131072 Byte

//---- Scaler Params Setting
#define     SCALER_FACTOR_R2R       2               // Raw to Raw
#define     SCALER_FACTOR_R2L       2               // Raw to Luma
#define     SCALER_FACTOR_R2T       8               // Raw to Thumbnail

//////////////////////////////////////////////////////////////////////////

#define     WDR_USE_2D_PLANAR_FILTER       1 
#define     WDR_USE_THUMB_LUMA             1 



#endif // _RK_UTILS_H



