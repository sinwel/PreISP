// 
//////////////////////////////////////////////////////////////////////////
// File: rk_wdr.h
//
//////////////////////////////////////////////////////////////////////////
// 
//#pragma  once
#ifndef _RK_WDR_H
#define _RK_WDR_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"              // Type definition
#include "rk_global.h"               // Global definition
#include "vec-c.h"
//////////////////////////////////////////////////////////////////////////
// ln
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SHIFT_BIT		 	8 // 4 , 5 , 6
#define SHIFT_BIT_SCALE 	(SHIFT_BIT - 3)
#define MAX_BIT_VALUE  		(1<<SHIFT_BIT) 
#define MAX_BIT_V_MINUS1 	((1<<SHIFT_BIT) - 1)
#define SPLIT_SIZE 			MAX_BIT_VALUE


#define WDR_WEIGHT_STRIDE   256  



#define PRINT_CEVA_VRF(namestr,vReg,fp,...) \
    {\
        if (fp) \
        fprintf(fp,"[%s]: ",namestr);\
        else \
        fprintf(stderr,"[%s]: ",namestr); \
        for (int elem = 0; elem < vReg.num_of_elements; elem++) \
        { \
            if (fp) \
                fprintf(fp,"0x%04x ",vReg[elem]);\
            else \
                fprintf(stderr,"0x%04x ",vReg[elem]); \
        } \
        if (fp) \
        fprintf(fp,"\n");\
        else \
        fprintf(stderr,"\n"); \
    }


extern RK_U16      g_BaseThumbBuf[409600];                         // Thumb data pointers


//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, int max_scale);
//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_F32 testParams_5);
void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5); // 20160701
void ceva_bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5); // 20160701


//inline 
unsigned short clip16bit(unsigned long x);
//inline 
unsigned short clip10bit(unsigned short x);
void cul_wdr_cure2(unsigned short *table, float exp_times);

void cul_wdr_cure(unsigned short *table, unsigned short exp_times);

int check_wdr_result(RK_U16* data1, RK_U16* data2,int Wid ,int  Hgt);
int check_ushort16_vecc_result(RK_U16* data1, ushort16 data2, int  num);

void interpolationYaxis();

//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

#endif // _RK_WDR_H




