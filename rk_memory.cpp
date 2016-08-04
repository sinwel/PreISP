//
/////////////////////////////////////////////////////////////////////////
// File: rk_memory.cpp
// Desc: Implementation of memory operation
// 
// Date: Revised by yousf 20160721
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_memory.h"               // Memory operation

//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: classMemory::classMemory()
// Desc: classMemory constructor
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160428
// 
/*************************************************************************/
classMemory::classMemory(void)
{
    printf("==== classMemory::classMemory() ... \n" );

} // classMemory::classMemory()


/************************************************************************/
// Func: classMemory::~classMemory()
// Desc: classMemory destructor
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160428
// 
/*************************************************************************/
classMemory::~classMemory(void)
{
    printf("==== classMemory::~classMemory() ... \n" );

} // classMemory::~classMemory()


/************************************************************************/
// Func: classMemory::Init()
// Desc: classMemory Initialization
//   In: wid            - Raw data width
//       hgt            - Raw data height
//       numPic         - Raw data file num
//  Out: 
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int classMemory::Init(int wid, int hgt, int numPic)
{
    //
    int     ret = 0; // return value

    //
    if (numPic > RK_MAX_FILE_NUM)
    {
        ret = -1;
        return ret;
    }

    // DDR Memory
    DDR_UsedCount   = 0;                                // DDR Memory used count

    // RawSrcs
    mRawWid         = wid;					            // Raw data width
    mRawHgt         = hgt;					            // Raw data height
    mRawFileNum     = numPic;				            // number of Raw data files
    mRaw10Stride    = (wid * 10 + 31) / 32 * 4;         // Raw10bit data Stride (Bytes, 4ByteAlign)
    mRaw10DataSize  = mRawHgt * mRaw10Stride;           // Raw10bit data Size (Bytes)
    mRaw16Stride    = (wid * 16 + 31) / 32 * 4;         // Raw16bit data Stride (Bytes, 4ByteAlign)
    mRaw16DataSize  = mRawHgt * mRaw16Stride;           // Raw10bit data Size (Bytes)

    // ScaleFactors
    mScaleRaw2Raw   = SCALER_FACTOR_R2R;                // scale factor for Raw to Raw (for Preview)
    mScaleRaw2Luma  = SCALER_FACTOR_R2L;                // scale factor for Raw to Luma
    mScaleRaw2Thumb = SCALER_FACTOR_R2T;                // scale factor for Raw to Thumbnail

    // ThumbSrcs
    mThumbWid        = wid / mScaleRaw2Thumb;	        // Thumbnail data width  (floor)
    mThumbHgt        = hgt / mScaleRaw2Thumb;	        // Thumbnail data height (floor)
    mThumb16Stride   = (mThumbWid * 16 + 31) / 32 * 4;  // Thumbnail data Stride (Bytes, 4ByteAlign)
    mThumb16DataSize = mThumbHgt * mThumb16Stride;      // Thumbnail data Size (Bytes)

    // 
    return ret;

} // classMemory::Init()


/************************************************************************/
// Func: classMemory::DDRMalloc()
// Desc: DDR Memory Allocation
//   In: NULL
//  Out: pDDR_RawSrcs   - RawSrcs data pointers
//       pDDR_ThumbSrcs - ThumbSrcs data pointers
//       pDDR_RawDst    - RawDst data pointer
// 
// Date: Revised by yousf 20160429
// 
/*************************************************************************/
int classMemory::DDRMalloc(void)
{
    //
    int     ret = 0; // return value

    // RawSrcs: ceil((4608 * 10)/32)*4 * 3456 * 6 = 19906560Byte * 6 = 119439360Byte = 113.9063 MB
    for (int k=0; k < RK_MAX_FILE_NUM; k++)
    {
        if (k < mRawFileNum)
        {
            // malloc Raw10Srcs
            pDDR_Raw10Srcs[k] = (RK_U8*)malloc(sizeof(RK_U8) * mRaw10DataSize);
            if(pDDR_Raw10Srcs[k] == NULL)
            {
                ret = -1;
                return ret;
            }
            // DDR Memory used count
            DDR_UsedCount += mRaw10DataSize;    // DDR Memory used count
            if (DDR_UsedCount > DDR_MAX_MEM_SIZE)
            {
                ret = -1;
                return ret;
            }

            // malloc Raw16Srcs
            pDDR_Raw16Srcs[k] = (RK_U8*)malloc(sizeof(RK_U8) * mRaw16DataSize);
            if(pDDR_Raw16Srcs[k] == NULL)
            {
                ret = -1;
                return ret;
            }
        }
        else
        {
            pDDR_Raw10Srcs[k] = NULL;
        }
    }


    // ThumbSrcs: ceil((4608/8 * 16)/32)*4 * 3456/8 * 6 = 497664Byte * 6 = 2985984Byte = 2.8477 MB
    for (int k=0; k < RK_MAX_FILE_NUM; k++)
    {
        if (k < mRawFileNum)
        {
            // malloc Thumb16Srcs
            pDDR_Thumb16Srcs[k] = (RK_U8*)malloc(sizeof(RK_U8) * mThumb16DataSize);
            if(pDDR_Thumb16Srcs[k] == NULL)
            {
                ret = -1;
                return ret;
            }
            // DDR Memory used count
            DDR_UsedCount += mThumb16DataSize;    
            if (DDR_UsedCount > DDR_MAX_MEM_SIZE)
            {
                ret = -1;
                return ret;
            }
        }
        else
        {
            pDDR_Thumb16Srcs[k] = NULL;
        }
    }

    // RawDst
    pDDR_Raw10Dst = pDDR_Raw10Srcs[0];  // point to Frame#0 (reuse)
    if(pDDR_Raw10Dst == NULL)
    {
        ret = -1;
        return ret;
    }
    pDDR_Raw16Dst = pDDR_Raw16Srcs[0];  // point to Frame#0 (reuse)
    if(pDDR_Raw16Dst == NULL)
    {
        ret = -1;
        return ret;
    }
    
    //
    return ret;

} // classMemory::MallocRawSrcs()


/************************************************************************/
// Func: classMemory::DMA_MIPIIN2DDR_Raw10()
// Desc: MIPI/IN(16bit) to DDR(10bit,4ByteAlign)
//   In: pRawSrcs       - RawSrcs data pointers
//  Out: pDDR_Raw10Srcs - RawSrcs data pointers in DDR
// 
// Date: Revised by yousf 20160429
// 
/*************************************************************************/
int classMemory::DMA_MIPIIN2DDR_Raw10(RK_U16* pRawSrcs[])
{
    //
    int     ret = 0; // return value

    // init vars
    RK_U16          Pixel0, Pixel1, Pixel2, Pixel3;     // 4Pixel
    RK_U8           PixelTmp;                           // temp Pixel Value
    RK_U8*          pDdrFrmHeader = NULL;               // temp pointer for DDR
    RK_U8*          pDdrFrmLine   = NULL;               // temp pointer for DDR

    // MIPI/IN
    for (int k=0; k < mRawFileNum; k++)
    {
        // DDR: 1 frame -- 10bit4ByteAlign
        pDdrFrmHeader   = pDDR_Raw10Srcs[k];                    // point to DDR
        for (int y=0; y < mRawHgt; y++)
        {
            pDdrFrmLine = pDdrFrmHeader + y * mRaw10Stride;     // Frame#k DDR address
            for (int x=0; x < mRawWid; x+=4)                    // 4Pixel -> 5Byte
            {
                // 4 Pixel
                Pixel0 = *(pRawSrcs[k] + y * mRawWid + x + 0);  // Pixel 0
                Pixel1 = *(pRawSrcs[k] + y * mRawWid + x + 1);  // Pixel 1
                Pixel2 = *(pRawSrcs[k] + y * mRawWid + x + 2);  // Pixel 2
                Pixel3 = *(pRawSrcs[k] + y * mRawWid + x + 3);  // Pixel 3

                // 5 Byte
                PixelTmp = (Pixel0 >> 0) & 0xFF;                // get low  8bit in Pixel 0 -> low  8bit in Byte 0   255 = 2^8-1 = 0xFF
                *(pDdrFrmLine++) = PixelTmp;                    // Byte 0

                PixelTmp = (Pixel0 >> 8) & 0x3;                 // get high 2bit in Pixel 0 -> low  2bit in Byte 1     3 = 2^2-1 = 0x3
                PixelTmp = ((Pixel1 & 0x3F) << 2) | PixelTmp;   // get low  6bit in Pixel 1 -> high 6bit in Byte 1    63 = 2^6-1 = 0x3F
                *(pDdrFrmLine++) = PixelTmp;                    // Byte 1

                PixelTmp = (Pixel1 >> 6) & 0xF;                 // get high 4bit in Pixel 1 -> low  4bit in Byte 2    15 = 2^4-1 = 0xF
                PixelTmp = ((Pixel2 & 0xF) << 4) | PixelTmp;    // get low  4bit in Pixel 2 -> high 4bit in Byte 2    15 = 2^4-1 = 0xF
                *(pDdrFrmLine++) = PixelTmp;                    // Byte 2

                PixelTmp = (Pixel2 >> 4) & 0x3F;                // get high 6bit in Pixel 2 -> low  6bit in Byte 3    63 = 2^6-1 = 0x3F
                PixelTmp = ((Pixel3 & 0xF) << 6) | PixelTmp;    // get low  2bit in Pixel 3 -> high 2bit in Byte 3    15 = 2^4-1 = 0xF
                *(pDdrFrmLine++) = PixelTmp;                    // Byte 3

                PixelTmp = (Pixel3 >> 2) & 0xFF;                // get high 8bit in Pixel 3 -> low  8bit in Byte 4   255 = 2^8-1 = 0xFF
                *(pDdrFrmLine++) = PixelTmp;                    // Byte 4

            } // for x
        } // for y

    } // for k


    //
    return ret;

} // classMemory::DMA_MIPIIN2DDR_Raw10()


/************************************************************************/
// Func: classMemory::DMA_MIPIIN2DDR_Raw16()
// Desc: MIPI/IN(16bit) to DDR(16bit,4ByteAlign)
//   In: pRawSrcs       - RawSrcs data pointers
//  Out: pDDR_Raw16Srcs - RawSrcs data pointers in DDR
// 
// Date: Revised by yousf 20160429
// 
/*************************************************************************/
int classMemory::DMA_MIPIIN2DDR_Raw16(RK_U16* pRawSrcs[])
{
    //
    int     ret = 0; // return value

    // init vars
    RK_U8*          pDdrFrmHeader = NULL;               // temp pointer for DDR
    RK_U8*          pDdrFrmLine   = NULL;               // temp pointer for DDR

    // MIPI/IN
    for (int k=0; k < mRawFileNum; k++)
    {
        // DDR: 1 frame -- 16bit4ByteAlign
        pDdrFrmHeader   = pDDR_Raw16Srcs[k];                    // point to DDR
        for (int y=0; y < mRawHgt; y++)
        {
            pDdrFrmLine = pDdrFrmHeader + y * mRaw16Stride;     // Frame#k DDR address
            memcpy(pDdrFrmLine, pRawSrcs[k] + y * mRawWid, sizeof(RK_U8) * mRaw16Stride);
        } // for y

    } // for k

    //
    return ret;

} // classMemory::DMA_MIPIIN2DDR_Raw16()





/************************************************************************/
// Func: classMemory::DDRFree()
// Desc: DDR Memory Free
//   In: pDDR_RawSrcs   - RawSrcs data pointers
//       pDDR_ThumbSrcs - ThumbSrcs data pointers
//       pDDR_RawDst    - RawDst data pointer
//  Out: NULL
// 
// Date: Revised by yousf 20160429
// 
/*************************************************************************/
int classMemory::DDRFree(void)
{
    //
    int     ret = 0; // return value

    // RawSrcs
    for (int k=0; k < RK_MAX_FILE_NUM; k++)
    {
        if (k < mRawFileNum)
        {
            if (pDDR_Raw10Srcs[k] != NULL)
            {
                free(pDDR_Raw10Srcs[k]);
                pDDR_Raw10Srcs[k] = NULL;
            }
            if (pDDR_Raw16Srcs[k] != NULL)
            {
                free(pDDR_Raw16Srcs[k]);
                pDDR_Raw16Srcs[k] = NULL;
            }
        }
    }

    // ThumbSrcs
    for (int k=0; k < RK_MAX_FILE_NUM; k++)
    {
        if (k < mRawFileNum)
        {
            if (pDDR_Thumb16Srcs[k] != NULL)
            {
                free(pDDR_Thumb16Srcs[k]);
                pDDR_Thumb16Srcs[k] = NULL;
            }
        }
    }

    // RawDst
//     if (pDDR_Raw10Dst != NULL) // = pDDR_Raw10Srcs[0]
//     {
//         free(pDDR_Raw10Dst);
//         pDDR_Raw10Dst = NULL;
//     }
//     if (pDDR_Raw16Dst != NULL) // = pDDR_Raw16Srcs[0]
//     {
//         free(pDDR_Raw16Dst);
//         pDDR_Raw16Dst = NULL;
//     }

    //
    return ret;

} // classMemory::DDRFree()


//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
////---- Global Functions
//
/************************************************************************/
// Func: ReadRaw16Data()
// Desc: Read Raw16 Data
//   In: fileName       - file name
//       nWid           - Raw data width 
//       nHgt           - Raw data height
//  Out: pRawData       - Raw data pointer
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int ReadRaw16Data(char* fileName, int nWid, int nHgt, RK_U16** pRawData)
{
    //
    int         ret = 0;
    FILE*       fp  = NULL;
    int         nDataSize;
    RK_U8*      pData = NULL;

    //
    fp = fopen(fileName, "r+b");
    //fopen_s(&fp, fileName, "r+b");
    if(fp == NULL)
    {
        ret = -1;
        return ret;
    }

    //
    fseek(fp, 0, SEEK_END);
    nDataSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    pData = (RK_U8 *)malloc(sizeof(RK_U8) * nDataSize);
    if(pData == NULL)
    {
        ret = -1;
        return ret;
    }

    // 
    if(nDataSize != fread(pData, sizeof(RK_U8), nDataSize, fp))
    {
        ret = -1;
        return ret;
    }

    // 
    *pRawData = (RK_U16*)pData;
    pData = NULL;

    // 
    return  0;

} // ReadRaw16Data()


/************************************************************************/
// Func: WriteRaw8Data()
// Desc: Write Raw8 Data
//   In: pRawData       - Raw data pointer
//       nWid           - Raw data width 
//       nHgt           - Raw data height
//  Out: fileName       - file name
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int WriteRaw8Data(RK_U8* pRawData, int nWid, int nHgt, char* fileName)
{
    //
    int         ret = 0;
    FILE*       fp  = NULL;

    // 
    fp = fopen(fileName, "w+b");
    //fopen_s(&fp, fileName, "w+b");
    if(fp == NULL)
    {
        ret = -1;
        return ret;
    }
    fwrite(pRawData, sizeof(RK_U8), nWid * nHgt, fp);
    fclose(fp);

    // 
    return  0;

} // WriteRaw8Data()


/************************************************************************/
// Func: WriteRaw16Data()
// Desc: Write Raw16 Data
//   In: pRawData       - Raw data pointer
//       nWid           - Raw data width 
//       nHgt           - Raw data height
//  Out: fileName       - file name
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int WriteRaw16Data(RK_U16* pRawData, int nWid, int nHgt, char* fileName)
{
    //
    int         ret = 0;
    FILE*       fp  = NULL;

    // 
    fp = fopen(fileName, "w+b");
    //fopen_s(&fp, fileName, "w+b");
    if(fp == NULL)
    {
        ret = -1;
        return ret;
    }
    fwrite(pRawData, sizeof(RK_U16), nWid * nHgt, fp);
    fclose(fp);

    // 
    return  0;

} // WriteRaw16Data()


//////////////////////////////////////////////////////////////////////////


