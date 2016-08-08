/***************************************************************************
**  
**  wdr_cevaxm4_vecc.cpp
**  
** 	Do wdr for block, once store 16 short data out.
** 	need to set the width and height with corrsponding Stride.
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/12 9:12:44 version 1.0
**  
**	version 1.0 	have not MERGE to branch.
**   
**
** Copyright 2016, rockchip.
**
***************************************************************************/
#include "rk_wdr.h"
#include <assert.h>
#include <vec-c.h>



void ceva_bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5) // 20160701
{
	int		i;
	int		x, y;
	RK_U16		*pcount[9], **pcount_mat;
	RK_U32		*pweight[9], **pweight_mat;
	RK_U16		*pweight_vcc[9],**pweight_mat_vecc;
	RK_U16		sw, sh;
	RK_U16		light;

	RK_U16		*p1, *p2, *p3;
	RK_U16		*plight;
	RK_U16		scale_table[1025];

	int lindex; //
	int blacklevel=256;

#if WDR_USE_CEVA_VECC
	RK_U16	pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	ushort16* 			plight16;
	ushort16 			light16;
	ushort16 			lindex16;
	short16 inN	;
	ushort16 v0,v1,v2,v3,v4,v5;
	const unsigned short* inM;
	short chunkStrideBytes = 256, ptrChunks[16] = {0};
	short offset;
	unsigned char bi0,bi1;

	int16 vacc0,vacc1,vacc2,vacc3;
#endif	


	cul_wdr_cure2(scale_table, max_scale);


    int wThumb        = w  / SCALER_FACTOR_R2T;      // Thumb data width  (floor)
    int hThumb        = h  / SCALER_FACTOR_R2T;      // Thumb data height (floor)
	int Thumb16Stride   = (wThumb*16 + 31) / 32 * 4; 

  	sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	for (i = 0; i < 9; i++)
	{
		pcount[i] 			= (RK_U16*)malloc(sw*sh*sizeof(RK_U16));
		pweight_vcc[i] 		= (RK_U16*)malloc(sw*sh*sizeof(RK_U16));
		pweight[i] 			= (RK_U32*)malloc(sw*sh*sizeof(RK_U32));
		memset(pcount[i], 0, sw*sh*sizeof(RK_U16));
		memset(pweight[i], 0, sw*sh*sizeof(RK_U32));
	}
	pcount_mat 			= pcount;
	pweight_mat 		= pweight;
	pweight_mat_vecc 	= pweight_vcc;
	plight = (RK_U16*)malloc(w*h*sizeof(RK_U16));
	memset(plight, 0, w*h*sizeof(RK_U16));

  	p1 = g_BaseThumbBuf;
	p2 = g_BaseThumbBuf + wThumb;
	p3 = g_BaseThumbBuf + 2 * wThumb;


	for (y = 1; y < hThumb - 1 ; y++)
	{
		for (x = 1; x < wThumb - 1 ; x++)
		{
			int idx, idy; 
			int ScaleDownlight = 0;
			
			ScaleDownlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			ScaleDownlight >>= 6;

			// ScaleDownlight is 14bit
			lindex = ((RK_U32)ScaleDownlight + 1024) >> 11;

			idx = (x + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			idy = (y + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			assert(idy < sh);
			assert(idx < sw);
			// 10bit  
			pcount_mat [lindex][idy*sw + idx] = pcount_mat [lindex][idy*sw + idx] + 1;
			// 32x32 14bit is 24bit  
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + ScaleDownlight;

		}
		p1 += wThumb;
		p2 += wThumb;
		p3 += wThumb;
	}


  	p1 = pixel_in; // Gain is 8
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;
	int sumlight = 0;
	for (y = 1; y < h - 1 ; y++)
	{
		for (x = 1; x < w-1 ; x++)
		{
			sumlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			sumlight >>= 3; // 10 + 3 + 4 - 3
			plight[y*w + x] = sumlight;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}


	//filter
	// do the 2D planar array filter .
#if WDR_USE_2D_PLANAR_FILTER
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < sh; y++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i][y*sw];
			for (x = 0; x < sw; x++)
			{
				tl = tm;
				tm = tr;
				if (x < sw - 1)
					tr = pcount_mat[i][y*sw + x + 1];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr);
			}
		}
		for (x = 0; x < sw; x++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i][x];
			for (y = 0; y < sh; y++)
			{
				tl = tm;
				tm = tr;
				if (y < sh - 1)
					tr = pcount_mat[i][(y + 1)*sw + x];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr );
			}
		}
	}
#endif
	// do the 3-rd array filter .
	for (y = 0; y < sh; y++)
	{
		for (x = 0; x < sw; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pcount_mat[0][y*sw + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pcount_mat[i + 1][y*sw + x];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr);
			}
		}
	}
	//filter
	// do the 2D planar array filter .
#if WDR_USE_2D_PLANAR_FILTER
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < sh; y++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i][y*sw];
			for (x = 0; x < sw; x++)
			{
				tl = tm;
				tm = tr;
				if (x < sw - 1)
					tr = pweight_mat[i][y*sw + x + 1];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
		for (x = 0; x < sw; x++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i][x];
			for (y = 0; y < sh; y++)
			{
				tl = tm;
				tm = tr;
				if (y < sh - 1)
					tr = pweight_mat[i][(y + 1)*sw + x];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
#endif
	// do the 3-rd array filter .
	for (y = 0; y < sh; y++)
	{
		for (x = 0; x < sw; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pweight_mat[0][y*sw + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pweight_mat[i + 1][y*sw + x];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
	/* normilzing  */
	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < sh; y++)
		{
			for (x = 0; x < sw; x++)
			{
				if (pcount_mat[i][y*sw + x])
					pweight_mat_vecc[i][y*sw + x] = (RK_U16)(4*pweight_mat[i][y*sw + x] / pcount_mat[i][y*sw + x]);
				else
					pweight_mat_vecc[i][y*sw + x] = 0;

				if (pweight_mat[i][y*sw + x] > 16383)
					pweight_mat_vecc[i][y*sw + x] = 16383;
			}
		}
	}

	RK_U16  left[9], right[9];
	RK_U16 weight1;
	RK_U16 weight2;
	RK_U16 weight;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++) // input/output 16 pixel result.
		{
			
			if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
				for (i = 0; i < 9; i++)
				{
					left[i] =  (pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT)]     * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
					
					right[i] = (pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + 1] * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw + 1] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
				}

			}
		  
			light = plight[y*w + x];
			if (light>16*1023)
			{
				light=16*1023;
			}
			lindex = light >> 11;

			weight1 = (left[lindex]     * (MAX_BIT_VALUE - (x & MAX_BIT_V_MINUS1)) + right[lindex]     * (x & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
			weight2 = (left[lindex + 1] * (MAX_BIT_VALUE - (x & MAX_BIT_V_MINUS1)) + right[lindex + 1] * (x & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;

			//light >>= 2;
			//weight = (weight1*(512 - (light & 511)) + weight2*(light & 511)) / 512;
			//light <<= 2;
			weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;
			
			if (abs(weight-light)>512)
			{
				if(light>weight)
					weight = light-512;
				else
					weight = light+512;
			}

			if(weight>blacklevel*4)
				weight = weight-blacklevel*4;
			else
				weight = 0;
			lindex = weight >> 4;
			weight = (scale_table[lindex] * (16 - (weight & 15)) + scale_table[lindex + 1] * (weight & 15) + 8) >> 4;

			*(pGainMat + y*w + x) = (RK_U32)weight; // for zlf-SpaceDenoise


			//*pixel_out++ = clip10bit(((unsigned long)(*pixel_in++) - 64 * 4)*weight / 512 + 64);
			if(*pixel_in>blacklevel*2)
				*pixel_out++ = clip10bit(((*pixel_in++) - blacklevel * 2)*weight / 1024 + blacklevel/4);
			else
			{
				*pixel_out++ = blacklevel/4;
				pixel_in++;
			}
		}
	}
	free(plight);
	for (i = 0; i < 9; i++)
	{
		free(pcount[i]);
		free(pweight[i]);
	}
}



void set_char32(uchar32 &data, int offset)
{
	unsigned char chargroup[32];
	chargroup[0] = (MAX_BIT_VALUE - ((offset+0) & MAX_BIT_V_MINUS1));
	chargroup[1] = (MAX_BIT_VALUE - ((offset+1) & MAX_BIT_V_MINUS1));
	chargroup[2] = (MAX_BIT_VALUE - ((offset+2) & MAX_BIT_V_MINUS1));
	chargroup[3] = (MAX_BIT_VALUE - ((offset+3) & MAX_BIT_V_MINUS1));
	chargroup[4] = (MAX_BIT_VALUE - ((offset+4) & MAX_BIT_V_MINUS1));
	chargroup[5] = (MAX_BIT_VALUE - ((offset+5) & MAX_BIT_V_MINUS1));
	chargroup[6] = (MAX_BIT_VALUE - ((offset+6) & MAX_BIT_V_MINUS1));
	chargroup[7] = (MAX_BIT_VALUE - ((offset+7) & MAX_BIT_V_MINUS1));
	chargroup[8] = (MAX_BIT_VALUE - ((offset+8) & MAX_BIT_V_MINUS1));
	chargroup[9] = (MAX_BIT_VALUE - ((offset+9) & MAX_BIT_V_MINUS1));
	chargroup[10] = (MAX_BIT_VALUE - ((offset+10) & MAX_BIT_V_MINUS1));
	chargroup[11] = (MAX_BIT_VALUE - ((offset+11) & MAX_BIT_V_MINUS1));
	chargroup[12] = (MAX_BIT_VALUE - ((offset+12) & MAX_BIT_V_MINUS1));
	chargroup[13] = (MAX_BIT_VALUE - ((offset+13) & MAX_BIT_V_MINUS1));
	chargroup[14] = (MAX_BIT_VALUE - ((offset+14) & MAX_BIT_V_MINUS1));
	chargroup[15] = (MAX_BIT_VALUE - ((offset+15) & MAX_BIT_V_MINUS1));

	chargroup[16] = (offset+0)  & MAX_BIT_V_MINUS1 ;
	chargroup[17] = (offset+1)  & MAX_BIT_V_MINUS1 ;
	chargroup[18] = (offset+2)  & MAX_BIT_V_MINUS1 ;
	chargroup[19] = (offset+3)  & MAX_BIT_V_MINUS1 ;
	chargroup[20] = (offset+4)  & MAX_BIT_V_MINUS1 ;
	chargroup[21] = (offset+5)  & MAX_BIT_V_MINUS1 ;
	chargroup[22] = (offset+6)  & MAX_BIT_V_MINUS1 ;
	chargroup[23] = (offset+7)  & MAX_BIT_V_MINUS1 ;
	chargroup[24] = (offset+8)  & MAX_BIT_V_MINUS1 ;
	chargroup[25] = (offset+9)  & MAX_BIT_V_MINUS1 ;
	chargroup[26] = (offset+10) & MAX_BIT_V_MINUS1 ;
	chargroup[27] = (offset+11) & MAX_BIT_V_MINUS1 ;
	chargroup[28] = (offset+12) & MAX_BIT_V_MINUS1 ;
	chargroup[29] = (offset+13) & MAX_BIT_V_MINUS1 ;
	chargroup[30] = (offset+14) & MAX_BIT_V_MINUS1 ;
	chargroup[31] = (offset+15) & MAX_BIT_V_MINUS1 ;
	data = *(uchar32*)chargroup;
}

void set_short16(short16 &data, int offset)
{
	unsigned short group[16];
	group[0]  = offset+0   ;                                     
	group[1]  = offset+1   ;                                     
	group[2]  = offset+2   ;                                     
	group[3]  = offset+3   ;                                     
	group[4]  = offset+4   ;                                     
	group[5]  = offset+5   ;                                     
	group[6]  = offset+6   ;                                     
	group[7]  = offset+7   ;                                     
	group[8]  = offset+8   ;                                     
	group[9]  = offset+9   ;                                     
	group[10] = offset+10  ;                                    
	group[11] = offset+11  ;                                    
	group[12] = offset+12  ;                                    
	group[13] = offset+13  ;                                    
	group[14] = offset+14  ;                                    
	group[15] = offset+15  ;                                    

	data = *(short16*)group;
}




void wdr_cevaxm4_vecc(unsigned short *pixel_in, 
					unsigned short *pixel_out, 
					int w, 
					int h, 
					float max_scale, 
					RK_U32* pGainMat, 
					RK_F32 testParams_5)
{
	RK_U16		light[16];
	RK_U16		pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	RK_U16 		weight1[16];
	RK_U16 		weight2[16];
	RK_U16 		weight[16];
	RK_U16 		lindex[16],lindexSingle;
	RK_U16		scale_table[1025];


	int 		blacklevel=256;

	ushort16 	light16,weight16,resi16,const16_1,const16_2;
	uchar32 	weightLow16;

	unsigned char config_list[32] = {0,  2,  4,  6,  8,  10, 12, 14, 
									 16, 18, 20, 22, 24, 26, 28, 30,
									 32, 34, 36, 38, 40, 42, 44, 46, 
									 48, 50, 52, 54, 56, 58, 60, 62 };
	uchar32 vconfig0 = *(uchar32*)(&config_list[0]);

	
	short16 	lindex16,const16;
	const16 	= (short16)512;
	const16_1 	= (ushort16)(blacklevel*4);
	const16_2 	= (ushort16)(blacklevel*2);
	ushort16 const16_3 = (ushort16)(blacklevel/4);
	uint16   const16_4 = (uint16)(blacklevel*256);
	short16 inN	;
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11;
	const unsigned short* inM;
	
	short offset;
	unsigned short bi0,bi1;
	short16 bi0_vecc,bi1_vecc;
	unsigned short vpr0,vpr1,vpr2,vprMask = 0xffff;
	
	uchar32	bifactor_xAxis;	
	uint16 vacc0,vacc1,vacc2,vacc3;
	int16  vaccX;
	unsigned short left[9],right[9];
	int x, y;
  	int wStride16 = ((w + 15)/16)*16 ;
  	int sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int i,k,ret = 0;
	short chunkStrideBytes = 256, ptrChunks[16] = {0};

	// align 16.
	RK_U16 		*pixel_in_vecc	= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pGainMat_align = (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pGainMat_vecc 	= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pixel_out_align= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16)); 
	RK_U16 		*pixel_out_vecc = (RK_U16*)malloc(wStride16*h*sizeof(RK_U16)); 
	RK_U16		*ptmp0 = pixel_in;
	RK_U16		*ptmp1 = pixel_in_vecc;
	RK_U16		*ptmp2 = pixel_out_align;
	RK_U16		*ptmp3 = pixel_out_vecc;
	RK_U16		*ptmp4 = pGainMat_align;
	RK_U16		*ptmp5 = pGainMat_vecc;
	
	// copy input with stride.
	for ( i = 0 ; i < h ; i++ )
	{
		memset(ptmp1,0,sizeof(RK_U16)*wStride16);
		memcpy(ptmp1,ptmp0,sizeof(RK_U16)*w);
		ptmp0 	+= w;
		ptmp1 	+= wStride16;
	}
	ptmp1 = pixel_in_vecc;
	ptmp0 = pixel_in_vecc;
	//<<!---------------------------------------------------------------------
	//<<! Calcu The weight matrix from Thumb 512x512 of base image.

	RK_U16		*pcount[9], **pcount_mat;
	RK_U32		*pweight[9], **pweight_mat;
	RK_U16		pweight_mat_vecc[9][256];

	RK_U16		*p1, *p2, *p3;
	RK_U16		*plight;

	cul_wdr_cure2(scale_table, max_scale);

	
	int wThumb        = w  / SCALER_FACTOR_R2T;      // Thumb data width  (floor)
    int hThumb        = h  / SCALER_FACTOR_R2T;      // Thumb data height (floor)


	for (i = 0; i < 9; i++)
	{
		pcount[i] 			= (RK_U16*)malloc(sw*sh*sizeof(RK_U16));
		pweight[i] 			= (RK_U32*)malloc(sw*sh*sizeof(RK_U32));
		memset(pcount[i], 0, sw*sh*sizeof(RK_U16));
		memset(pweight[i], 0, sw*sh*sizeof(RK_U32));
	}
	pcount_mat 			= pcount;
	pweight_mat 		= pweight;

	plight = (RK_U16*)malloc(w*h*sizeof(RK_U16));
	memset(plight, 0, w*h*sizeof(RK_U16));

  	p1 = g_BaseThumbBuf;
	p2 = g_BaseThumbBuf + wThumb;
	p3 = g_BaseThumbBuf + 2 * wThumb;


	for (y = 1; y < hThumb - 1 ; y++)
	{
		for (x = 1; x < wThumb - 1 ; x++)
		{
			int idx, idy; 
			int ScaleDownlight = 0;
			
			ScaleDownlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			ScaleDownlight >>= 6;

			// ScaleDownlight is 14bit
			lindexSingle = ((RK_U32)ScaleDownlight + 1024) >> 11;

			idx = (x + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			idy = (y + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			assert(idy < sh);
			assert(idx < sw);
			// 10bit  
			pcount_mat [lindexSingle][idy*sw + idx] = pcount_mat [lindexSingle][idy*sw + idx] + 1;
			// 32x32 14bit is 24bit  
			pweight_mat[lindexSingle][idy*sw + idx] = pweight_mat[lindexSingle][idy*sw + idx] + ScaleDownlight;

		}
		p1 += wThumb;
		p2 += wThumb;
		p3 += wThumb;
	}


  	p1 = pixel_in; // Gain is 8
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;
	int sumlight = 0;
	for (y = 1; y < h - 1 ; y++)
	{
		for (x = 1; x < w-1 ; x++)
		{
			sumlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			sumlight >>= 3; // 10 + 3 + 4 - 3
			plight[y*w + x] = sumlight;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}


	//filter
	// do the 2D planar array filter .
#if WDR_USE_2D_PLANAR_FILTER
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < sh; y++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i][y*sw];
			for (x = 0; x < sw; x++)
			{
				tl = tm;
				tm = tr;
				if (x < sw - 1)
					tr = pcount_mat[i][y*sw + x + 1];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr);
			}
		}
		for (x = 0; x < sw; x++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i][x];
			for (y = 0; y < sh; y++)
			{
				tl = tm;
				tm = tr;
				if (y < sh - 1)
					tr = pcount_mat[i][(y + 1)*sw + x];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr );
			}
		}
	}
#endif
	// do the 3-rd array filter .
	for (y = 0; y < sh; y++)
	{
		for (x = 0; x < sw; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pcount_mat[0][y*sw + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pcount_mat[i + 1][y*sw + x];
				else
					tr = 0;
				pcount_mat[i][y*sw + x] = (tl) + (tm *2) + (tr);
			}
		}
	}
	//filter
	// do the 2D planar array filter .
#if WDR_USE_2D_PLANAR_FILTER
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < sh; y++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i][y*sw];
			for (x = 0; x < sw; x++)
			{
				tl = tm;
				tm = tr;
				if (x < sw - 1)
					tr = pweight_mat[i][y*sw + x + 1];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
		for (x = 0; x < sw; x++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i][x];
			for (y = 0; y < sh; y++)
			{
				tl = tm;
				tm = tr;
				if (y < sh - 1)
					tr = pweight_mat[i][(y + 1)*sw + x];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
#endif
	// do the 3-rd array filter .
	for (y = 0; y < sh; y++)
	{
		for (x = 0; x < sw; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pweight_mat[0][y*sw + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pweight_mat[i + 1][y*sw + x];
				else
					tr = 0;
				pweight_mat[i][y*sw + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
	/* normilzing  */
	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < sh; y++)
		{
			for (x = 0; x < sw; x++)
			{
				if (pcount_mat[i][y*sw + x])
					pweight_mat_vecc[i][y*sw + x] = (RK_U16)(4*pweight_mat[i][y*sw + x] / pcount_mat[i][y*sw + x]);
				else
					pweight_mat_vecc[i][y*sw + x] = 0;

				if (pweight_mat[i][y*sw + x] > 16383)
					pweight_mat_vecc[i][y*sw + x] = 16383;
			}
		}
	}



	for (y = 0; y < h; y++)
	{
		for (x = 0; x < wStride16; x+=16) // input/output 16 pixel result.
		{
			//if ((x + 16) > w)
			//	vprMask = ((1 << ((w-x) & 15)) - 1) & 0xffff;
			//else
			//	vprMask = 0xffff;
			// -------------------------------------------------------------- // 
			/* x+256 for one loop */
			if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
			#if DEBUG_VECC
				for (i = 0; i < 9; i++)
				{
					// (14BIT * 8 + 14BIT *8) / 8BIT
					left[i] =  (pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT)]     * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
					
					right[i] = (pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + 1] * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw + 1] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
				}
			#endif
				inM = pweight_mat_vecc[0];				
				offset = (y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT);
				ptrChunks[0] = chunkStrideBytes * 0 + offset;
				ptrChunks[1] = chunkStrideBytes * 1 + offset;
				ptrChunks[2] = chunkStrideBytes * 2 + offset;
				ptrChunks[3] = chunkStrideBytes * 3 + offset;
				ptrChunks[4] = chunkStrideBytes * 4 + offset;
				ptrChunks[5] = chunkStrideBytes * 5 + offset;
				ptrChunks[6] = chunkStrideBytes * 6 + offset;
				ptrChunks[7] = chunkStrideBytes * 7 + offset;
				ptrChunks[8] = chunkStrideBytes * 8 + offset;
				inN 		 = *(short16*)ptrChunks;

				vpld(inM, inN, v0, v2);
				//PRINT_CEVA_VRF("v0",v0,stderr);
				//PRINT_CEVA_VRF("v2",v2,stderr);

				offset += sw;
				ptrChunks[0] = chunkStrideBytes * 0 + offset;
				ptrChunks[1] = chunkStrideBytes * 1 + offset;
				ptrChunks[2] = chunkStrideBytes * 2 + offset;
				ptrChunks[3] = chunkStrideBytes * 3 + offset;
				ptrChunks[4] = chunkStrideBytes * 4 + offset;
				ptrChunks[5] = chunkStrideBytes * 5 + offset;
				ptrChunks[6] = chunkStrideBytes * 6 + offset;
				ptrChunks[7] = chunkStrideBytes * 7 + offset;
				ptrChunks[8] = chunkStrideBytes * 8 + offset;
				inN 		 = *(short16*)ptrChunks;
				vpld(inM, inN, v1, v3);
				//PRINT_CEVA_VRF("v1",v1,stderr);
				//PRINT_CEVA_VRF("v3",v3,stderr);

				/* DO interpolation, left by v0/v1, right by v2/v3, when y is zero, do copy v0/v2 only rather than vmac3 */
				if ((y & MAX_BIT_V_MINUS1)==0)
				{
					v0 = v0;
					v2 = v2;
				}
				else 
				{
					bi0 	= (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1));
					bi1 	= (y & MAX_BIT_V_MINUS1);
					vacc0 	= (uint16) 0;
					v0 		= vmac3(psl, v0, bi0, v1, bi1, vacc0, (unsigned char)SHIFT_BIT);
					v2 		= vmac3(psl, v2, bi0, v3, bi1, vacc0, (unsigned char)SHIFT_BIT);

				}
			#if DEBUG_VECC			
				ret  = check_ushort16_vecc_result(left,  v0, 9);
				ret += check_ushort16_vecc_result(right, v2, 9);
				if (ret){
					PRINT_CEVA_VRF("v0",v0,stderr);
					PRINT_CEVA_VRF("v2",v2,stderr);
				}
				assert(ret == 0);
			#endif
			}
			// -------------------------------------------------------------- // 
		#if DEBUG_VECC			
			/* x+16 for one loop */
			for  ( k = 0 ; k < 16 ; k++ )
			{
    			light[k] 	= plight[y*w + x + k];
				if (light[k]>16*1023)
				{
					light[k]=16*1023;
				}
				lindex[k]  	= light[k] >> 11;

				weight1[k] 	= (left[lindex[k]]     * (MAX_BIT_VALUE - ((x + k) & MAX_BIT_V_MINUS1)) 
					    + right[lindex[k]]     * ((x + k) & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
				weight2[k] 	= (left[lindex[k] + 1] * (MAX_BIT_VALUE - ((x + k) & MAX_BIT_V_MINUS1)) 
					    + right[lindex[k] + 1] * ((x + k) & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;

				weight[k] 	= (weight1[k]*(2048 - (light[k] & 2047)) + weight2[k]*(light[k]  & 2047)) / 2048; 

			}
		#endif
			light16  	= *(ushort16*)&plight[y*w + x];
			light16  	= (ushort16)vcmpmov(lt, light16, (ushort16)16*1023);
			lindex16 	= vshiftr(light16, (unsigned char) 11);

			// get left(v0) and right(v2) from VRF register by vperm.
			vpld((unsigned short*)&v0[0], lindex16, v1, v4);
			vpld((unsigned short*)&v2[0], lindex16, v3, v5);

			set_char32(bifactor_xAxis,x);		
			vacc0 		= (uint16) 0;
			v6 			= vmac3(splitsrc, psl, v1, v3, bifactor_xAxis, vacc0, (unsigned char)SHIFT_BIT);
			v7 			= vmac3(splitsrc, psl, v4, v5, bifactor_xAxis, vacc0, (unsigned char)SHIFT_BIT);

			// char <= 255, so 256 is overflow, need do speical.

			if ((x & MAX_BIT_V_MINUS1)==0){
				v6[0] = v1[0];
				v7[0] = v4[0];
			}
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight1,  v6, 16);
			ret += check_ushort16_vecc_result(weight2,  v7, 16);
			if (ret){
				PRINT_C_GROUP("w1",weight1,16,stderr);
				PRINT_CEVA_VRF("v6",v6,stderr);
				
				PRINT_C_GROUP("w2",weight2,16,stderr);
				PRINT_CEVA_VRF("v7",v7,stderr);
			}
			assert(ret == 0);
		#endif		
			bi1_vecc 	= vand(light16, 				(unsigned short)2047);
			bi0_vecc 	= vsub((unsigned short)2048,     bi1_vecc);

			vaccX 		= vmpy(v6, bi0_vecc);			
			weight16 	=  (ushort16)vmac(psl, v7, bi1_vecc, vaccX, (unsigned char)11);
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight,  weight16, 16);
			if (ret){
				PRINT_CEVA_VRF("weight16",weight16,stderr);
			}
			assert(ret == 0);

			/* x+16 for one loop */
			for  ( k = 0 ; k < 16; k++ )
			{
				if (abs(weight[k]-light[k])>512)
				{
					if(light[k]>weight[k])
						weight[k] = light[k]-512;
					else
						weight[k] = light[k]+512;
				}
				
				
				if(weight[k]>blacklevel*4)
					weight[k] = weight[k]-blacklevel*4;
				else
					weight[k] = 0;

				
				lindex[k] = weight[k] >> 4;
				// use lindex and weight to get final weight 
				weight[k] = ( scale_table[lindex[k]]     * (16 - (weight[k] & 15)) 
							+ scale_table[lindex[k] + 1] * (weight[k] & 15) + 8  ) >> 4;

				*(pGainMat_align + y*w + x + k) = (RK_U32)weight[k]; // for zlf-SpaceDenoise

				if(*ptmp0>blacklevel*2)
					*ptmp2++ = clip10bit(((*ptmp0++) - blacklevel * 2)*weight[k] / 1024 + blacklevel/4);
				else
				{
					*ptmp2++ = blacklevel/4;
					ptmp0++;
				}
				
			}
		#endif	
			// -------------------------------------------------------------- // 
			resi16 		= vabssub(light16,weight16);			
			vpr0 		= vabscmp(gt, (short16)resi16, const16);// weight need be clip by light
			vpr1 		= vcmp(gt,weight16,light16)&vpr0;// weight > light
			vpr2 		= vcmp(le,weight16,light16)&vpr0;// weight > light
			weight16 	= (ushort16)vselect(vadd((short16)light16,const16), weight16, vpr1); // weight = light+512 or // weight = light+512
			weight16 	= (ushort16)vselect(vsub((short16)light16,const16), weight16, vpr2); // weight = light+512 or // weight = light+512

			vpr0 		= vcmp(gt,weight16,const16_1);
			weight16 	= (ushort16)vselect(vsub(weight16,const16_1), (short16)0, vpr0); // weight = light+512 or // weight = light+512
			lindex16 	= vshiftr(weight16,  (unsigned char)4);

			inM 		= scale_table;		
			vpld(inM, lindex16, v1, v4);// v1 is scale_table[lindex[k]], v4 is scale_table[lindex[k]+1]

			weightLow16 = (uchar32)vand(weight16,(unsigned short )15); 
			weightLow16 = (uchar32)vperm(weightLow16,vsub((uchar32)16,weightLow16),vconfig0);

			vacc0 		= (uint16)8;	
			weight16 	= vmac3(splitsrc, psl, v4, v1, weightLow16, vacc0, (unsigned char)4);
			vst(weight16,(ushort16*)ptmp5,vprMask); // vprMask handle with unalign in image board.
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight,  weight16, 16);
			if (ret){
				PRINT_C_GROUP("weight",weight,16,stderr);
				PRINT_CEVA_VRF("weight16",weight16,stderr);
			}
		#endif		
			set_short16(inN , 0);
			v1 			= vpld(ptmp1,inN);
			vpr0 		= vcmp(gt,v1,const16_2);
			v1 			= (ushort16)vsub(v1,const16_2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v1 			= (ushort16)vmac(psl, v1, weight16, const16_4, (unsigned char)10);
			v1 			= vselect(v1, const16_3, vpr0); 
			v1 			= vclip(v1, (ushort16) 0, (ushort16) 1023);
			vst(v1,(ushort16*)ptmp3,vprMask); // vprMask handle with unalign in image board.
		#if DEBUG_VECC
			ret += check_wdr_result(ptmp2 - 16,  ptmp3, 16, 1);	
			if (ret){
				PRINT_CEVA_VRF("out",v1,stderr);
			}	
		#endif
			//if ((x + 16) > w)
			//{
			//	ptmp1  += (w-x);
			//	ptmp3 += (w-x);
			//}
			//else
			{
				ptmp1 += 16;
				ptmp3 += 16;
				ptmp5 += 16;
			}
			
		}
	}


	ptmp0 = pixel_out;
	ptmp1 = pixel_out_align;
	//ptmp2 = (RK_U16*)pGainMat;

	// copy output to pixel_out from pixel_out_align(align 16).
	for ( i = 0 ; i < h ; i++ )
	{
		memcpy(ptmp0,ptmp1,sizeof(RK_U16)*w);
		ptmp0 		+= w;
		ptmp1 		+= wStride16;
		//memcpy(ptmp2,ptmp4,sizeof(RK_U16)*w);
		//ptmp2 		+= w;
		//ptmp4 		+= wStride16;
	}
	if(plight)
		free(plight);
	if(pGainMat_align)
		free(pGainMat_align);
	if(pGainMat_vecc)
		free(pGainMat_vecc);
	if(pixel_in_vecc)
		free(pixel_in_vecc);
	if(pixel_out_align)
		free(pixel_out_align);
	if(pixel_out_vecc)
		free(pixel_out_vecc);
}
