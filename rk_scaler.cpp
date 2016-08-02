//
/////////////////////////////////////////////////////////////////////////
// File: rk_scaler.c
// Desc: Implementation of Scaler
// 
// Date: Revised by yousf 20160512
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_scaler.h"               // Scaler


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: classScaler::classScaler()
// Desc: classScaler constructor
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160324
// 
/*************************************************************************/
classScaler::classScaler(void)
{
    //

} // classScaler::classScaler()


/************************************************************************/
// Func: classScaler::~classScaler()
// Desc: classScaler destructor
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160324
// 
/*************************************************************************/
classScaler::~classScaler(void)
{
    //

} // classScaler::~classScaler()


/************************************************************************/
// Func: classScaler::ScaleDown_R2R()
// Desc: Raw to Raw
//   In: pRawSrc        - Raw Src pointer
//       nSrcWid        - Src width
//       nSrcHgt        - Src height
//       nDstWid        - Dst width
//       nDstHgt        - Dst height
//  Out: pRawDst        - Raw Dst pointer
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int classScaler::ScaleDown_R2R(RK_U16* pRawSrc, int nSrcWid, int nSrcHgt, int nDstWid, int nDstHgt, RK_U16* pRawDst)
{
    // common vars
    int             ret             = 0;			    // status init

    // init vars
    RK_U16*         pTmp0           = NULL;             // temp pointer for RawSrc data
    RK_U16*         pTmp1           = NULL;             // temp pointer for RawSrc data
    RK_U16*         pTmp2           = NULL;             // temp pointer for RawSrc data
    RK_U16*         pTmp3           = NULL;             // temp pointer for RawSrc data
    RK_U16*         pTmp4           = NULL;             // temp pointer for RawDst data
    int             RawValue;                           // Dst Raw Value

    // scale factor
    int scale_R2R = SCALER_FACTOR_R2R;                  // scale factor for Raw to Raw

    //
    if (nSrcWid/scale_R2R != nDstWid || nSrcHgt/scale_R2R != nDstHgt)
    {
        ret = -1;
        return ret;
    }

    // Raw to Raw -- 1/(2x2)
    // bayer00
    pTmp0   = pRawSrc;                  
    pTmp1   = pRawSrc + 2;
    pTmp2   = pRawSrc + 2*nSrcWid;
    pTmp3   = pRawSrc + 2*nSrcWid + 2;
    pTmp4   = pRawDst; 
    for (int i=0; i < nDstHgt/2; i++)
    {
        for (int j=0; j < nDstWid/2; j++)
        {
            // sum
            RawValue  = 0;
            RawValue += *pTmp0;
            RawValue += *pTmp1;
            RawValue += *pTmp2;
            RawValue += *pTmp3;
            RawValue  = RawValue >> 2; // scale=2x2
            *pTmp4 = (RK_U16)( (double) (RawValue) + 0.5 );
            // temp pointer
            pTmp0 += 4;
            pTmp1 += 4;
            pTmp2 += 4;
            pTmp3 += 4;
            pTmp4 += 2;
        }
        pTmp0 += 3*nSrcWid;
        pTmp1 += 3*nSrcWid;
        pTmp2 += 3*nSrcWid;
        pTmp3 += 3*nSrcWid;
        pTmp4 += nDstWid;
    }

    // bayer01
    pTmp0   = pRawSrc + 1;                  
    pTmp1   = pRawSrc + 3;
    pTmp2   = pRawSrc + 2*nSrcWid + 1;
    pTmp3   = pRawSrc + 2*nSrcWid + 3;
    pTmp4   = pRawDst + 1; 
    for (int i=0; i < nDstHgt/2; i++)
    {
        for (int j=0; j < nDstWid/2; j++)
        {
            // sum
            RawValue  = 0;
            RawValue += *pTmp0;
            RawValue += *pTmp1;
            RawValue += *pTmp2;
            RawValue += *pTmp3;
            RawValue  = RawValue >> 2; // scale=2x2
            *pTmp4 = (RK_U16)( (double) (RawValue) + 0.5 );
            // temp pointer
            pTmp0 += 4;
            pTmp1 += 4;
            pTmp2 += 4;
            pTmp3 += 4;
            pTmp4 += 2;
        }
        pTmp0 += 3*nSrcWid;
        pTmp1 += 3*nSrcWid;
        pTmp2 += 3*nSrcWid;
        pTmp3 += 3*nSrcWid;
        pTmp4 += nDstWid;
    }

    // bayer10
    pTmp0   = pRawSrc + nSrcWid;                  
    pTmp1   = pRawSrc + nSrcWid + 2;
    pTmp2   = pRawSrc + 3*nSrcWid;
    pTmp3   = pRawSrc + 3*nSrcWid + 2;
    pTmp4   = pRawDst + nDstWid; 
    for (int i=0; i < nDstHgt/2; i++)
    {
        for (int j=0; j < nDstWid/2; j++)
        {
            // sum
            RawValue  = 0;
            RawValue += *pTmp0;
            RawValue += *pTmp1;
            RawValue += *pTmp2;
            RawValue += *pTmp3;
            RawValue  = RawValue >> 2; // scale=2x2
            *pTmp4 = (RK_U16)( (double) (RawValue) + 0.5 );
            // temp pointer
            pTmp0 += 4;
            pTmp1 += 4;
            pTmp2 += 4;
            pTmp3 += 4;
            pTmp4 += 2;
        }
        pTmp0 += 3*nSrcWid;
        pTmp1 += 3*nSrcWid;
        pTmp2 += 3*nSrcWid;
        pTmp3 += 3*nSrcWid;
        pTmp4 += nDstWid;
    }

    // bayer11
    pTmp0   = pRawSrc + nSrcWid + 1;                  
    pTmp1   = pRawSrc + nSrcWid + 3;
    pTmp2   = pRawSrc + 3*nSrcWid + 1;
    pTmp3   = pRawSrc + 3*nSrcWid + 3;
    pTmp4   = pRawDst + nDstWid + 1; 
    for (int i=0; i < nDstHgt/2; i++)
    {
        for (int j=0; j < nDstWid/2; j++)
        {
            // sum
            RawValue  = 0;
            RawValue += *pTmp0;
            RawValue += *pTmp1;
            RawValue += *pTmp2;
            RawValue += *pTmp3;
            RawValue  = RawValue >> 2; // scale=2x2
            *pTmp4 = (RK_U16)( (double) (RawValue) + 0.5 );
            // temp pointer
            pTmp0 += 4;
            pTmp1 += 4;
            pTmp2 += 4;
            pTmp3 += 4;
            pTmp4 += 2;
        }
        pTmp0 += 3*nSrcWid;
        pTmp1 += 3*nSrcWid;
        pTmp2 += 3*nSrcWid;
        pTmp3 += 3*nSrcWid;
        pTmp4 += nDstWid;
    }

    //
    return ret;

} // classScaler::ScaleDown_R2R()


/************************************************************************/
// Func: classScaler::ScaleDown_R2L()
// Desc: Raw to Luma
//   In: pRawData       - Raw data pointer
//       nRawWid        - Raw data width
//       nRawHgt        - Raw data height
//       nLumaWid       - Luma data width
//       nLumaHgt       - Luma data height
//  Out: pLumaData      - Luma data pointer
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int classScaler::ScaleDown_R2L(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, RK_U16* pLumaData)
{
    // common vars
    int             ret             = 0;			    // status init

	// init vars
	RK_U16*         pTmp0           = NULL;             // temp pointer for Raw data
	RK_U16*         pTmp1           = NULL;             // temp pointer for Raw data
	RK_U16*         pTmp2           = NULL;             // temp pointer for Luma data
    RK_U16          LumaValue       = 0;                // Luma Value

    // scale factor
    int scale_R2L = SCALER_FACTOR_R2L;                  // scale factor for Raw to Luma

    //
    if (nRawWid/scale_R2L != nLumaWid || nRawHgt/scale_R2L != nLumaHgt)
    {
        ret = -1;
        return ret;
    }

	// Raw to Luma -- 1/(2x2)
	pTmp2 = pLumaData;
	for (int i=0; i < nLumaHgt; i++)
	{
		for (int j=0; j <nLumaWid; j++)
		{
			// 2x2 Rect left-top
			pTmp0 = pRawData + (i * nRawWid + j) * scale_R2L;

			// average(2x2Raw) -> LumaValue
			LumaValue = 0;
			for (int m=0; m<scale_R2L; m++)
			{
				// m-th line in 2x2 Rect
				pTmp1 = pTmp0 + m * nRawWid;
				for (int n=0; n<scale_R2L; n++)
				{
					LumaValue += *(pTmp1++);
				}
			}
            *pTmp2 = LumaValue;
            pTmp2++;

		}
	}

    //
    return ret;

} // classScaler::ScaleDown_R2L()


/************************************************************************/
// Func: classScaler::ScaleDown_R2T()
// Desc: Raw to Thumbnail
//   In: pRawData       - Raw data pointer
//       nRawWid        - Raw data width
//       nRawHgt        - Raw data height
//       nThumbWid      - Thumb data width
//       nThumbHgt      - Thumb data height
//  Out: pThumbData     - Thumb data pointer
// 
// Date: Revised by yousf 20160512
// 
/*************************************************************************/
int classScaler::ScaleDown_R2T(RK_U16* pRawData, int nRawWid, int nRawHgt, int nThumbWid, int nThumbHgt, RK_U16* pThumbData)
{
    // common vars
    int             ret             = 0;			    // status init

    // init vars
    RK_U16*         pTmp0           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp1           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp2           = NULL;             // temp pointer for Thumb data
    RK_U16          ThumbValue      = 0;                // Luma Value

    // scale factor
    int scale_R2T = SCALER_FACTOR_R2T;                  // scale factor for Raw to Thumbnail

    //
    if (nRawWid/scale_R2T != nThumbWid || nRawHgt/scale_R2T != nThumbHgt)
    {
        ret = -1;
        return ret;
    }

    // Raw to Thumbnail -- 1/(8x8)
    pTmp2 = pThumbData;
    for (int i=0; i < nThumbHgt; i++)
    {
        for (int j=0; j <nThumbWid; j++)
        {
            // 8x8 Rect left-top
            pTmp0 = pRawData + (i * nRawWid + j) * scale_R2T;
            
            // average(8x8Raw) -> ThumbValue
            ThumbValue = 0;
            for (int m=0; m<scale_R2T; m++)
            {
                // m-th line in 8x8 Rect
                pTmp1 = pTmp0 + m * nRawWid;
                for (int n=0; n<scale_R2T; n++)
                {
                    ThumbValue += *(pTmp1++);
                }
            }
            *pTmp2 = ThumbValue;
            pTmp2++;

        }
    }

    //
    return ret;

} // classScaler::ScaleDown_R2T()


//////////////////////////////////////////////////////////////////////////



