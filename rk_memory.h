// 
//////////////////////////////////////////////////////////////////////////
// File: rk_memory.h
// Desc: Memory operation
// 
// Date: Revised by yousf 20160721
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma  once
#ifndef _RK_MEMORY_H
#define _RK_MEMORY_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"              // Type definition
#include "rk_global.h"               // Global definition

//////////////////////////////////////////////////////////////////////////
// ln
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef char			s8;
typedef unsigned char	u8;
typedef short			s16;
typedef unsigned short	u16;
typedef long			s32;
typedef unsigned long	u32;

//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, int max_scale);
//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_F32 testParams_5);
void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5); // 20160701
//////////////////////////////////////////////////////////////////////////

#define     DDR_MAX_MEM_SIZE        134217728       // DDR memory size: 128MB = 128*1024*1024 = 134217728 Byte

#define		MAX(a, b)			    ( (a) > (b) ? (a) : (b) )
#define		MIN(a, b)			    ( (a) < (b) ? (a) : (b) )


//////////////////////////////////////////////////////////////////////////
////-------- Class Definition

// class Memory
class classMemory
{
public:
    //// constructor & destructor
    classMemory(void);                              // constructor
    ~classMemory(void);                             // destructor

private: 

public: 
    // DDR Memory
    int             DDR_UsedCount;                      // DDR Memory used count

    // RawSrcs
    int			    mRawWid;					        // Raw data width
    int			    mRawHgt;					        // Raw data height
    int			    mRawFileNum;				        // number of Raw data files
    int             mRaw10Stride;                       // Raw10bit data Stride (Bytes, 4ByteAlign)
    int             mRaw10DataSize;                     // Raw10bit data Size (Bytes)
    RK_U8*          pDDR_Raw10Srcs[RK_MAX_FILE_NUM];    // Raw10Srcs data pointers: 10bit*6frame
    int             mRaw16Stride;                       // Raw16bit data Stride (Bytes, 4ByteAlign)
    int             mRaw16DataSize;                     // Raw10bit data Size (Bytes)
    RK_U8*          pDDR_Raw16Srcs[RK_MAX_FILE_NUM];    // Raw16Srcs data pointers: 16bit*6frame

    // ScaleFactors
    int             mScaleRaw2Raw;                      // scale factor for Raw to Raw (for Preview)
    int             mScaleRaw2Luma;                     // scale factor for Raw to Luma
    int             mScaleRaw2Thumb;                    // scale factor for Raw to Thumbnail

    // ThumbSrcs
    int			    mThumbWid;					        // Thumbnail data width
    int			    mThumbHgt;					        // Thumbnail data height
    int             mThumb16Stride;                     // Thumbnail data Stride (Bytes, 4ByteAlign)
    int             mThumb16DataSize;                   // Thumbnail data Size (Bytes)
    RK_U8*          pDDR_Thumb16Srcs[RK_MAX_FILE_NUM];  // Thumbnail data pointers: 16bit*6frame

    // Dst
    RK_U8*          pDDR_Raw10Dst;                      // Raw10Dst data pointer
    RK_U8*          pDDR_Raw16Dst;                      // Raw10Dst data pointer


public: 

    //// Initialization
    int Init(int wid, int hgt, int numPic);             // classMemory Initialization

    //// Memory Allocation
    int DDRMalloc(void);                                // DDR Memory Allocation

    //// Memory Operation
    int DMA_MIPIIN2DDR_Raw10(RK_U16* pRawSrcs[]);       // MIPI/IN(16bit) to DDR(10bit,4ByteAlign)
    int DMA_MIPIIN2DDR_Raw16(RK_U16* pRawSrcs[]);       // MIPI/IN(16bit) to DDR(16bit,4ByteAlign)



    //// Memory Free
    int DDRFree(void);          // DDR Memory Free 


};


int ReadRaw16Data(char* fileName, int nWid, int nHgt, RK_U16** pRawData);   // Read Raw16 Data
int WriteRaw8Data(RK_U8* pRawData, int nWid, int nHgt, char* fileName);     // Write Raw8 Data
int WriteRaw16Data(RK_U16* pRawData, int nWid, int nHgt, char* fileName);   // Write Raw16 Data


//////////////////////////////////////////////////////////////////////////

#endif // _RK_MEMORY_H



