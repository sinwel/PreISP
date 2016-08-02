//
//////////////////////////////////////////////////////////////////////////
// File: rk_scaler.h
// Desc: Scaler
// 
// Date: Revised by yousf 20160512
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_SCALER_H
#define _RK_SCALER_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_typedef.h"              // Type definition
#include "rk_global.h"               // Global definition


//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition


//////////////////////////////////////////////////////////////////////////
////-------- Class Definition

// class Scaler
class classScaler
{
public:
    //// constructor & destructor
    classScaler(void);                              // constructor
    ~classScaler(void);                             // destructor
    
public: 
    //// Down Scale
    int ScaleDown_R2R(RK_U16* pRawSrc, int nSrcWid, int nSrcHgt, int nDstWid, int nDstHgt, RK_U16* pRawDst); // Raw to Raw
    int ScaleDown_R2L(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, RK_U16* pLumaData);  // Raw to Luma
    int ScaleDown_R2T(RK_U16* pRawData, int nRawWid, int nRawHgt, int nThumbWid, int nThumbHgt, RK_U16* pThumbData);  // Raw to Thumbnail

};


//////////////////////////////////////////////////////////////////////////

#endif // _RK_SCALER_H



