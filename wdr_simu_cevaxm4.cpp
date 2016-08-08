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
#ifdef __XM4__
#include "profiler.h"
#include "XM4_defines.h"
#endif
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

#define VECC_ONCE_LEN 		16
void wdr_simu_cevaxm4()
{
	RK_U16				light[VECC_ONCE_LEN];
	RK_U16				pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	RK_U16 				weight1[VECC_ONCE_LEN];
	RK_U16 				weight2[VECC_ONCE_LEN];
	RK_U16 				weight[VECC_ONCE_LEN];
	RK_U16 				lindex[VECC_ONCE_LEN];
	RK_U16				scale_table[1025];


	
	int 				blacklevel=256;
	ushort16* 			plight16[4];
	ushort16 			light16[4],weight16[4],resi16[4];
	
	uchar32 			weightLow16[4];

	unsigned char 		config_list[32] = {	  0,  2,  4,  6,  8,  10, 12, 14, 
	 										 16, 18, 20, 22, 24, 26, 28, 30,
	 										 32, 34, 36, 38, 40, 42, 44, 46, 
	 										 48, 50, 52, 54, 56, 58, 60, 62 };
	uchar32 			vconfig0 = *(uchar32*)(&config_list[0]);

	
 	short16 			lindex16[4],const16 	= (short16)512;
	ushort16 			const16mpy4 		= (ushort16)(blacklevel*4);
	ushort16 			const16mpy2 		= (ushort16)(blacklevel*2);
	ushort16 			const16div4 		= (ushort16)(blacklevel/4);
	uint16   			const16mpy256 	 	= (uint16)(blacklevel*256);
	short16 			inN0,inN1	;
	ushort16 			v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15;
	ushort16 			v16,v17,v18,v19,v20,v21,v22,v23,v24,v25,v26,v27,v28,v29,v30,v31;

	short 				offset,chunkStrideBytes = 256, ptrChunks[16];
	ptrChunks[0] = chunkStrideBytes * 0;
	ptrChunks[1] = chunkStrideBytes * 1;
	ptrChunks[2] = chunkStrideBytes * 2;
	ptrChunks[3] = chunkStrideBytes * 3;
	ptrChunks[4] = chunkStrideBytes * 4;
	ptrChunks[5] = chunkStrideBytes * 5;
	ptrChunks[6] = chunkStrideBytes * 6;
	ptrChunks[7] = chunkStrideBytes * 7;
	ptrChunks[8] = chunkStrideBytes * 8;
	short16 			ptrChan = *(short16*)ptrChunks;

	unsigned short 		bi0,bi1;
	short16 			bi0_vecc[4],bi1_vecc[4];
	unsigned short 		vpr0[4],vpr1[4],vpr2[4],vpr3[4],vprMask = 0xffff;
	uchar32				biBase,bifactor_xAxis[4];	
	set_char32(biBase,0);		

	int16 				vacc0,vacc1,vacc2,vacc3;

	unsigned short 		left[9],right[9];
	unsigned short 		left_vecc[9],right_vecc[9];
	
	//int x, y, w = 4164, h=3136;
	int x, y, w = 256, h=64;
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
	RK_U16 		*plight = (RK_U16*)malloc(w*h*sizeof(RK_U16));

	memcpy(scale_table,&cure_table[8][0],961*sizeof(RK_U16));

	
/*	
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


	//init plight
	for ( i = 0 ; i < h; i++ )
	    for ( j = 0 ; j < w; j++ )
			plight[i*w+j] =  (RK_U16)(CCV_rand()&0x03ff); //i*1024+j;	
*/	

#ifdef __XM4__
	PROFILER_START(h, w);
#endif
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < wStride16; x+=64) // input/output 16 pixel result.
		//DSP_CEVA_UNROLL(8)
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
				offset 		 = (y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT);				
				inN0 		 = vadd(ptrChan,(short16)offset);
				inN1 		 = vadd(inN0,	(short16)sw);				
				vpld((unsigned short*)pweight_vecc[0], inN0, v0, v2);
				vpld((unsigned short*)pweight_vecc[0], inN1, v1, v3);


				/* DO interpolation, left by v0/v1, right by v2/v3, when y is zero, do copy v0/v2 only rather than vmac3 */
				if ((y & MAX_BIT_V_MINUS1)==0)
				{
					v0 = v0;
					v2 = v2;
				}
				else 
				{					
					bi1 	= (y & MAX_BIT_V_MINUS1);
					bi0 	= (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1));
					v0 		= vmac3(psl, v0, bi0, v1, bi1, (uint16) 0, (unsigned char)SHIFT_BIT);
					v2 		= vmac3(psl, v2, bi0, v3, bi1, (uint16) 0, (unsigned char)SHIFT_BIT);
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
			/* x+VECC_ONCE_LEN for one loop */
			for  ( k = 0 ; k < VECC_ONCE_LEN ; k++ )
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
			light16[0]  	= *(ushort16*)&plight[y*w + x];
			light16[1]  	= *(ushort16*)&plight[y*w + x + 16];
			light16[2]  	= *(ushort16*)&plight[y*w + x + 32];
			light16[3]  	= *(ushort16*)&plight[y*w + x + 48];
			light16[0]  	= (ushort16)vcmpmov(lt, light16[0], (ushort16)16*1023);
			light16[1]  	= (ushort16)vcmpmov(lt, light16[2], (ushort16)16*1023);
			light16[2]  	= (ushort16)vcmpmov(lt, light16[3], (ushort16)16*1023);
			light16[3]  	= (ushort16)vcmpmov(lt, light16[4], (ushort16)16*1023);

			lindex16[0] 	= vshiftr(light16[0], (unsigned char) 11);
			lindex16[1] 	= vshiftr(light16[1], (unsigned char) 11);
			lindex16[2] 	= vshiftr(light16[2], (unsigned char) 11);
			lindex16[3] 	= vshiftr(light16[3], (unsigned char) 11);
			
			vpld((unsigned short*)left_vecc,  lindex16[0], v0, v1);
			vpld((unsigned short*)right_vecc, lindex16[0], v2, v3);

			vpld((unsigned short*)left_vecc,  lindex16[1], v4, v5);
			vpld((unsigned short*)right_vecc, lindex16[1], v6, v7);

			vpld((unsigned short*)left_vecc,  lindex16[2], v8, v9);
			vpld((unsigned short*)right_vecc, lindex16[2], v10, v11);

			vpld((unsigned short*)left_vecc,  lindex16[1], v12, v13);
			vpld((unsigned short*)right_vecc, lindex16[1], v14, v15);

			bifactor_xAxis[0] = (uchar32)vselect(vsub(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 0x00ff);
			bifactor_xAxis[1] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 16)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 0x00ff);
			bifactor_xAxis[2] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 32)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 0x00ff);
			bifactor_xAxis[3] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 48)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 0x00ff);

			v16 = vmac3(splitsrc, psl, v0, v2, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT);
			v17 = vmac3(splitsrc, psl, v1, v3, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT);
			v18 = vmac3(splitsrc, psl, v4, v6, bifactor_xAxis[1], (uint16) 0, (unsigned char)SHIFT_BIT);
			v19 = vmac3(splitsrc, psl, v5, v7, bifactor_xAxis[1], (uint16) 0, (unsigned char)SHIFT_BIT);

			v20 = vmac3(splitsrc, psl, v8, v10, bifactor_xAxis[2], (uint16) 0, (unsigned char)SHIFT_BIT);
			v21 = vmac3(splitsrc, psl, v9, v11, bifactor_xAxis[2], (uint16) 0, (unsigned char)SHIFT_BIT);
			v22 = vmac3(splitsrc, psl, v12, v14, bifactor_xAxis[3], (uint16) 0, (unsigned char)SHIFT_BIT);
			v23 = vmac3(splitsrc, psl, v13, v15, bifactor_xAxis[3], (uint16) 0, (unsigned char)SHIFT_BIT);


			// char <= 255, so 256 is overflow, need do speical.
			//if ((x & MAX_BIT_V_MINUS1)==0){
			//	//v6[0] = v1[0];
			//	//v7[0] = v4[0];
			//}

			bi1_vecc[0] = vand(light16[0], 				(unsigned short)2047);
			bi0_vecc[0] = vsub((unsigned short)2048,     bi1_vecc[0]);
			bi1_vecc[1] = vand(light16[1], 				(unsigned short)2047);
			bi0_vecc[1] = vsub((unsigned short)2048,     bi1_vecc[1]);
			bi1_vecc[2] = vand(light16[2], 				(unsigned short)2047);
			bi0_vecc[2] = vsub((unsigned short)2048,     bi1_vecc[2]);
			bi1_vecc[3] = vand(light16[3], 				(unsigned short)2047);
			bi0_vecc[3] = vsub((unsigned short)2048,     bi1_vecc[3]);

			vacc0 	 	= vmpy(v16, bi0_vecc[0]);			
			weight16[0] =  (ushort16)vmac(psl, v17, bi1_vecc[0], vacc0, (unsigned char)11);
			vacc1 	 	= vmpy(v18, bi0_vecc[1]);			
			weight16[1] =  (ushort16)vmac(psl, v19, bi1_vecc[1], vacc1, (unsigned char)11);
			vacc2 	 	= vmpy(v20, bi0_vecc[2]);			
			weight16[2] =  (ushort16)vmac(psl, v21, bi1_vecc[2], vacc2, (unsigned char)11);
			vacc3 	 	= vmpy(v22, bi0_vecc[3]);			
			weight16[3] =  (ushort16)vmac(psl, v23, bi1_vecc[3], vacc3, (unsigned char)11);

			
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
			ret += check_ushort16_vecc_result(weight,  weight16, 16);
			if (ret){
				PRINT_CEVA_VRF("weight16",weight16,stderr);
			}
			assert(ret == 0);

			// x+VECC_ONCE_LEN for one loop 
			for  ( k = 0 ; k < VECC_ONCE_LEN ; k++ )
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

			v8 		= vpld(ptmp1,(short16)0);
			v9 		= vpld((ptmp1+16),(short16)0);
			v10 	= vpld((ptmp1+32),(short16)0);
			v11 	= vpld((ptmp1+48),(short16)0);
			
			vpr3[0]	= vcmp(gt,v8,	const16mpy2);
			vpr3[1] = vcmp(gt,v9,	const16mpy2);
			vpr3[2] = vcmp(gt,v10,	const16mpy2);
			vpr3[3] = vcmp(gt,v11,	const16mpy2);
			
			resi16[0] 		= vabssub(light16[0],weight16[0]);		
			resi16[1] 		= vabssub(light16[1],weight16[1]);			
			resi16[2] 		= vabssub(light16[2],weight16[2]);		
			resi16[3] 		= vabssub(light16[3],weight16[3]);			
		
			vpr0[0] 		= vabscmp(gt, (short16)resi16[0], const16);// weight need be clip by light
			vpr0[1] 		= vabscmp(gt, (short16)resi16[1], const16);// weight need be clip by light
			vpr0[2] 		= vabscmp(gt, (short16)resi16[2], const16);// weight need be clip by light
			vpr0[3] 		= vabscmp(gt, (short16)resi16[3], const16);// weight need be clip by light

			vpr1[0] 		= vcmp(gt,weight16[0],light16[0])&vpr0[0];// weight > light
			vpr1[1] 		= vcmp(gt,weight16[1],light16[1])&vpr0[1];// weight > light
			vpr1[2] 		= vcmp(gt,weight16[2],light16[2])&vpr0[2];// weight > light
			vpr1[3] 		= vcmp(gt,weight16[3],light16[3])&vpr0[3];// weight > light

			vpr2[0] 		= vcmp(le,weight16[0],light16[0])&vpr0[0];// weight > light
			vpr2[1] 		= vcmp(le,weight16[1],light16[1])&vpr0[1];// weight > light
			vpr2[2] 		= vcmp(le,weight16[2],light16[2])&vpr0[2];// weight > light
			vpr2[3] 		= vcmp(le,weight16[3],light16[3])&vpr0[3];// weight > light

			weight16[0] 	= (ushort16)vselect(vadd((short16)light16[0] ,const16), weight16[0] , vpr1[0] ); // weight = light+512 or // weight = light+512
			weight16[1]  	= (ushort16)vselect(vadd((short16)light16[1] ,const16), weight16[1] , vpr1[1] ); // weight = light+512 or // weight = light+512
			weight16[2]  	= (ushort16)vselect(vadd((short16)light16[2] ,const16), weight16[2] , vpr1[2] ); // weight = light+512 or // weight = light+512
			weight16[3]  	= (ushort16)vselect(vadd((short16)light16[3] ,const16), weight16[3] , vpr1[3] ); // weight = light+512 or // weight = light+512

			weight16[0] 	= (ushort16)vselect(vsub((short16)light16[0],const16), weight16[0], vpr2[0]); // weight = light+512 or // weight = light+512
			weight16[1] 	= (ushort16)vselect(vsub((short16)light16[1],const16), weight16[1], vpr2[1]); // weight = light+512 or // weight = light+512
			weight16[2] 	= (ushort16)vselect(vsub((short16)light16[2],const16), weight16[2], vpr2[2]); // weight = light+512 or // weight = light+512
			weight16[3] 	= (ushort16)vselect(vsub((short16)light16[3],const16), weight16[3], vpr2[3]); // weight = light+512 or // weight = light+512

			vpr0[0] 		= vcmp(gt,weight16[0],const16mpy4);
			vpr0[1] 		= vcmp(gt,weight16[1],const16mpy4);
			vpr0[2] 		= vcmp(gt,weight16[2],const16mpy4);
			vpr0[3] 		= vcmp(gt,weight16[3],const16mpy4);

			weight16[0] 	= (ushort16)vselect(vsub(weight16[0],const16mpy4), (short16)0, vpr0[0]); // weight = light+512 or // weight = light+512
			weight16[1] 	= (ushort16)vselect(vsub(weight16[1],const16mpy4), (short16)0, vpr0[1]); // weight = light+512 or // weight = light+512
			weight16[2] 	= (ushort16)vselect(vsub(weight16[2],const16mpy4), (short16)0, vpr0[2]); // weight = light+512 or // weight = light+512
			weight16[3] 	= (ushort16)vselect(vsub(weight16[3],const16mpy4), (short16)0, vpr0[3]); // weight = light+512 or // weight = light+512

			lindex16[0] 	= vshiftr(weight16[0],  (unsigned char)4);
			lindex16[1] 	= vshiftr(weight16[1],  (unsigned char)4);
			lindex16[2] 	= vshiftr(weight16[2],  (unsigned char)4);
			lindex16[3] 	= vshiftr(weight16[3],  (unsigned char)4);
			
			vpld((unsigned short*)scale_table , lindex16[0], v0, v1);// v0 is scale_table[lindex[k]], v1 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[1], v2, v3);// v2 is scale_table[lindex[k]], v3 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[2], v4, v5);// v4 is scale_table[lindex[k]], v5 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[3], v6, v7);// v6 is scale_table[lindex[k]], v7 is scale_table[lindex[k]+1]

			weightLow16[0] = (uchar32)vand(weight16[0],(unsigned short )15); 
			weightLow16[1] = (uchar32)vand(weight16[1],(unsigned short )15); 
			weightLow16[2] = (uchar32)vand(weight16[2],(unsigned short )15); 
			weightLow16[3] = (uchar32)vand(weight16[3],(unsigned short )15); 

			weightLow16[0] = (uchar32)vperm(weightLow16[0],vsub((uchar32)16,weightLow16[0]),vconfig0);
			weightLow16[1] = (uchar32)vperm(weightLow16[1],vsub((uchar32)16,weightLow16[1]),vconfig0);
			weightLow16[2] = (uchar32)vperm(weightLow16[2],vsub((uchar32)16,weightLow16[2]),vconfig0);
			weightLow16[3] = (uchar32)vperm(weightLow16[3],vsub((uchar32)16,weightLow16[3]),vconfig0);

			weight16[0] 	= vmac3(splitsrc, psl, v1, v0, weightLow16[0], (uint16)8, (unsigned char)4);
			weight16[1] 	= vmac3(splitsrc, psl, v3, v2, weightLow16[1], (uint16)8, (unsigned char)4);
			weight16[2] 	= vmac3(splitsrc, psl, v5, v4, weightLow16[2], (uint16)8, (unsigned char)4);
			weight16[3] 	= vmac3(splitsrc, psl, v7, v6, weightLow16[3], (uint16)8, (unsigned char)4);

			vst(weight16[0],(ushort16*)ptmp5,	  vprMask); // vprMask handle with unalign in image board.
			vst(weight16[1],(ushort16*)(ptmp5+16),vprMask); // vprMask handle with unalign in image board.
			vst(weight16[2],(ushort16*)(ptmp5+32),vprMask); // vprMask handle with unalign in image board.
			vst(weight16[3],(ushort16*)(ptmp5+48),vprMask); // vprMask handle with unalign in image board.


			v8 		= (ushort16)vsub(v8,	const16mpy2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v9 		= (ushort16)vsub(v9,	const16mpy2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v10 	= (ushort16)vsub(v10,	const16mpy2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v11 	= (ushort16)vsub(v10,	const16mpy2); // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.

			v8 		= (ushort16)vmac(psl, v8,  weight16[0], const16mpy256, (unsigned char)10);
			v9 		= (ushort16)vmac(psl, v9,  weight16[1], const16mpy256, (unsigned char)10);
			v10 	= (ushort16)vmac(psl, v10, weight16[2], const16mpy256, (unsigned char)10);
			v11 	= (ushort16)vmac(psl, v11, weight16[3], const16mpy256, (unsigned char)10);

			v0 		= vselect(v8, const16div4, vpr3[0]); 
			v1 		= vselect(v9, const16div4, vpr3[1]); 
			v2 		= vselect(v10, const16div4, vpr3[2]); 
			v3 		= vselect(v11, const16div4, vpr3[3]); 

			v0 		= vclip(v0, (ushort16) 0, (ushort16) 1023);
			v1 		= vclip(v1, (ushort16) 0, (ushort16) 1023);
			v2 		= vclip(v2, (ushort16) 0, (ushort16) 1023);
			v3 		= vclip(v3, (ushort16) 0, (ushort16) 1023);

			vst(v0,(ushort16*)ptmp3,		vprMask); // vprMask handle with unalign in image board.
			vst(v1,(ushort16*)(ptmp3+16),	vprMask); // vprMask handle with unalign in image board.
			vst(v2,(ushort16*)(ptmp3+32),	vprMask); // vprMask handle with unalign in image board.
			vst(v3,(ushort16*)(ptmp3+48),	vprMask); // vprMask handle with unalign in image board.
			
			ptmp1 += 64;
			ptmp3 += 64;
			ptmp5 += 64;
			
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight,  weight16, 16);
			if (ret){
				PRINT_C_GROUP("weight",weight,16,stderr);
				PRINT_CEVA_VRF("weight16",weight16,stderr);
			}
			ret += check_wdr_result(ptmp2 - 16,  ptmp3, 16, 1);	
			if (ret){
				PRINT_CEVA_VRF("out",v1,stderr);
			}	
		#endif

		}
	}
#ifdef __XM4__
	PROFILER_END();
#endif

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
