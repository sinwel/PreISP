/***************************************************************************
**  
**  wdr_simu_cevaxm4.cpp
**  
** 	To calcu the cycle of WDR.
**
**  NOTE: dependency on:
**      	profiler_lib. 
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/11 10:53:06 version 1.0
**  
**	version 1.0 	have not add profiler.
**   
**
** Copyright 2016, rockchip.
**
***************************************************************************/
//<<! Test for cylce for ceva xm4 platform.
//<<! 
#include "rk_wdr.h"
#include <assert.h>
#include <vec-c.h>
#include "profiler.h"

extern unsigned short cure_table[24][961];

static unsigned long rand_next = 1;

void CCV_srand(unsigned long seed)
{
    rand_next = seed;
}

int CCV_rand()
{
    return ((rand_next = rand_next * (long)1103515245 + 12345l) & 0x7fffffff);
}

void wdr_simu_cevaxm4()
{
	RK_U16		light[16];
	RK_U16		pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	RK_U16 		weight1[16];
	RK_U16 		weight2[16];
	RK_U16 		weight[16];
	RK_U16 		lindex[16];
	RK_U16		scale_table[1025];

	memcpy(scale_table,&cure_table[8][0],961*sizeof(RK_U16));

	
	int 		blacklevel=256, tmp[16] ,predLarge;
	ushort16* 	plight16;
	ushort16 	light16,weight16,resi16,const16_1,const16_2;
	uchar32 	weightLow16;

	unsigned char config_list[32] = {0,  2,  4,  6,  8,  10, 12, 14, 
									 16, 18, 20, 22, 24, 26, 28, 30,
									 32, 34, 36, 38, 40, 42, 44, 46, 
									 48, 50, 52, 54, 56, 58, 60, 62 };
	uchar32 vconfig0 = *(uchar32*)(&config_list[0]);

	
	short16 			lindex16,const16;
	const16 	= (short16)512;
	const16_1 	= (ushort16)(blacklevel*4);
	const16_2 	= (ushort16)(blacklevel*2);
	ushort16 const16_3 = (ushort16)(blacklevel/4);
	uint16   const16_4 = (uint16)(blacklevel*256);
	short16 inN	;
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11;
	const unsigned short* inM;
	short chunkStrideBytes = 256, ptrChunks[16] = {0};
	short offset;
	unsigned short bi0,bi1;
	short16 bi0_vecc,bi1_vecc;
	unsigned short vpr0,vpr1,vpr2,vprMask = 0xffff;
	
	uchar32	bifactor_xAxis;	
	uint16 vacc0,vacc1,vacc2,vacc3;
	int16  vaccX;
	unsigned short left[9],right[9];
	unsigned short left_vecc[9],right_vecc[9];
	
	int x, y, w = 4164, h=3136;
  	int wStride16 = ((w + 15)/16)*16 ;
  	int sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int i,j,k,ret = 0;

	RK_U16 		*pixel_in 		= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pixel_in_vecc	= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pGainMat 		= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pGainMat_vecc 	= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pixel_out 		= (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16 		*pixel_out_vecc = (RK_U16*)malloc(wStride16*h*sizeof(RK_U16));
	RK_U16		*ptmp0 = pixel_in;
	RK_U16		*ptmp1 = pixel_in_vecc;
	RK_U16		*ptmp2 = pixel_out;
	RK_U16		*ptmp3 = pixel_out_vecc;
	RK_U16		*ptmp5 = pGainMat_vecc;
	
	// copy input with stride.
	for ( i = 0 ; i < h ; i++ )
	    for ( j = 0 ; j < wStride16 ; j++ )
		{	
			pixel_in[i*w+j] 		=  (RK_U16)(CCV_rand()&0x03ff); //i*1024+j;		
			pixel_in_vecc[i*w+j] 	=  pixel_in[i*w+j]	;	
		}


	//init pweight_vecc
	for ( i = 0 ; i < 9 ; i++ )
	    for ( j = 0 ; j < sw*sh ; j++ )
			pweight_vecc[i][j] =  (RK_U16)(CCV_rand()&0xffff); //i*1024+j;	

	RK_U16 *plight = (RK_U16*)malloc(w*h*sizeof(RK_U16));

	//init plight
	for ( i = 0 ; i < h; i++ )
	    for ( j = 0 ; j < w; j++ )
			plight[i*w+j] =  (RK_U16)(CCV_rand()&0x03ff); //i*1024+j;	
	

	
	PROFILER_START(h, w);
	for (y = 1; y < h; y++)
	{
		for (x = 0; x < wStride16; x+=16) // input/output 16 pixel result.
		{
			// -------------------------------------------------------------- // 
			/* x+256 for one loop */
			if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
			#if DEBUG_VECC
				for (i = 0; i < 9; i++)
				{
					// (14BIT * 8 + 14BIT *8) / 8BIT
					left[i] =  (pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT)]     * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
					
					right[i] = (pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + 1] * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw + 1] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
				}
			#endif
				inM 		 = pweight_vecc[0];				
				offset 		 = (y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT);
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
				vst(v0,(ushort16*)left_vecc,0x1ff); // store 9 points.
 				vst(v2,(ushort16*)right_vecc,0x1ff); // store 9 points.
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
			//vpld((unsigned short*)&v0[0], lindex16, v1, v4);
			//vpld((unsigned short*)&v2[0], lindex16, v3, v5);
			vpld((unsigned short*)left_vecc, lindex16, v1, v4);
			vpld((unsigned short*)right_vecc, lindex16, v3, v5);

			set_char32(bifactor_xAxis,x);		
			vacc0 		= (uint16) 0;
			v6 			= vmac3(splitsrc, psl, v1, v3, bifactor_xAxis, vacc0, (unsigned char)SHIFT_BIT);
			v7 			= vmac3(splitsrc, psl, v4, v5, bifactor_xAxis, vacc0, (unsigned char)SHIFT_BIT);

			// char <= 255, so 256 is overflow, need do speical.

			if ((x & MAX_BIT_V_MINUS1)==0){
				//v6[0] = v1[0];
				//v7[0] = v4[0];
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
			bi1_vecc = vand(light16, 				(unsigned short)2047);
			bi0_vecc = vsub((unsigned short)2048,     bi1_vecc);

			vaccX = vmpy(v6, bi0_vecc);			
			weight16 =  (ushort16)vmac(psl, v7, bi1_vecc, vaccX, (unsigned char)11);
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight,  weight16, 16);
			if (ret){
				PRINT_CEVA_VRF("weight16",weight16,stderr);
			}
			assert(ret == 0);

			/* x+16 for one loop */
			for  ( k = 0 ; k < 16 ; k++ )
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

				*(pGainMat + y*w + x + k) = (RK_U32)weight[k]; // for zlf-SpaceDenoise

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
			v1 = vpld(ptmp1,inN);
			vpr0 = vcmp(gt,v1,const16_2);
			v1 = (ushort16)vsub(v1,const16_2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v1 = (ushort16)vmac(psl, v1, weight16, const16_4, (unsigned char)10);
			v1 = vselect(v1, const16_3, vpr0); 
			v1 = vclip(v1, (ushort16) 0, (ushort16) 1023);
			vst(v1,(ushort16*)ptmp3,vprMask); // vprMask handle with unalign in image board.
		#if DEBUG_VECC
			ret += check_wdr_result(ptmp2 - 16,  ptmp3, 16, 1);	
			if (ret){
				PRINT_CEVA_VRF("out",v1,stderr);
			}	
		#endif
			ptmp1 += 16;
			ptmp3 +=16;
			ptmp5 += 16;
			
		}
	}
	PROFILER_END();


	if(pixel_in)
		free(pixel_in);	
	if(plight)
		free(plight);
	if(pGainMat)
		free(pGainMat);
	if(pGainMat_vecc)
		free(pGainMat_vecc);
	if(pixel_out)
		free(pixel_out);
	if(pixel_out_vecc)
		free(pixel_out_vecc);
}
