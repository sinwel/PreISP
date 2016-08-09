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
**	version 1.1 	add profiler and 1.7 cycle/pixel.
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



void wdr_simu_cevaxm4()
{
	RK_U16				light[VECC_ONCE_LEN];
	RK_U16				pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	RK_U16 				weight1[VECC_ONCE_LEN];
	RK_U16 				weight2[VECC_ONCE_LEN];
	RK_U16 				weight[VECC_ONCE_LEN],weight_bak[VECC_ONCE_LEN];
	RK_U16 				lindex[VECC_ONCE_LEN];
	RK_U16				scale_table[1025];


	
	int 				blacklevel=256;
	//ushort16* 			plight16[VECC_GROUP_SIZE];
	ushort16 			light16[VECC_GROUP_SIZE],weight16[VECC_GROUP_SIZE],resi16[VECC_GROUP_SIZE];
	
	uchar32 			weightLow16[VECC_GROUP_SIZE];

	unsigned char 		config_list[32] = {	  0,  2,  4,  6,  8,  10, 12, 14, 
	 										 16, 18, 20, 22, 24, 26, 28, 30,
	 										 32, 34, 36, 38, 40, 42, 44, 46, 
	 										 48, 50, 52, 54, 56, 58, 60, 62 };
	uchar32 			vconfig0 = *(uchar32*)(&config_list[0]);
	short				offset_in_C[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
	short16				offset_in = *(short16*)offset_in_C;
	
 	short16 			lindex16[VECC_GROUP_SIZE],const16 	= (short16)512;
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
	ptrChunks[9] = 0;
	ptrChunks[10] = 0;
	ptrChunks[11] = 0;
	ptrChunks[12] = 0;
	ptrChunks[13] = 0;
	ptrChunks[14] = 0;
	ptrChunks[15] = 0;
	short16 			ptrChan = *(short16*)ptrChunks;

	unsigned short 		bi0,bi1;
	short16 			bi0_vecc[VECC_GROUP_SIZE],bi1_vecc[VECC_GROUP_SIZE];
	//unsigned short 		vpr0[VECC_GROUP_SIZE];
	//unsigned short 		vpr1[VECC_GROUP_SIZE];
	unsigned short 		vpr2[VECC_GROUP_SIZE];
	//unsigned short 		vpr3[VECC_GROUP_SIZE];
	unsigned short 		vprMask = 0xffff,vprRightMask = 0xffff, vprYinterMask = 0xffff;
	uchar32				biBase,bifactor_xAxis[VECC_GROUP_SIZE];	
	set_char32(biBase,0);		

	int16 				vacc0,vacc1,vacc2,vacc3;

	unsigned short 		left[9],right[9];
	unsigned short 		left_vecc[9],right_vecc[9];
#if WIN32	
	int x, y, w = 4164, h=3136;
#else
	int x, y, w = 256, h=64;
#endif
  	int wStride16 = ((w + (VECC_ONCE_LEN-1))/VECC_ONCE_LEN)*VECC_ONCE_LEN ;
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

#ifdef WIN32	
	
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
			plight[i*w+j] =  (RK_U16)(CCV_rand()&0x3fff); //i*1024+j;	
#endif

#ifdef __XM4__
	PROFILER_START(h, w);
#endif
	for (y = 0; y < h; y++)
	{
	/*
		if ((y & MAX_BIT_V_MINUS1)==0)
			vprYinterMask = 0x0;
		
		bi1 	= (y & MAX_BIT_V_MINUS1);
		bi0 	= (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1));
		offset 	= (y >> SHIFT_BIT)*sw;
	*/

		
		for (x = 0; x < wStride16; x+=VECC_ONCE_LEN) // input/output 16 pixel result.
		//DSP_CEVA_UNROLL(8)
		{
			// -------------------------------------------------------------- // 
			/* x+256 for one loop */
			if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
				if ((y & MAX_BIT_V_MINUS1)==0)
					vprYinterMask = 0x0;
				else
					vprYinterMask = 0xffff;
				
				bi1 	= (y & MAX_BIT_V_MINUS1);
				bi0 	= (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1));
				offset 	= (y >> SHIFT_BIT)*sw;

				vprRightMask = 0xfffe;
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
				offset 		+= (x >> SHIFT_BIT);				
				inN0 		 = vadd(ptrChan,(short16)offset);
				inN1 		 = vadd(inN0,	(short16)sw);				
				vpld((unsigned short*)pweight_vecc[0], inN0, v0, v2);
				vpld((unsigned short*)pweight_vecc[0], inN1, v1, v3);


				/* DO interpolation, left by v0/v1, right by v2/v3, when y is zero, 
					do copy v0/v2 only rather than vmac3 */
				v0 = vselect(vmac3(psl, v0, bi0, v1, bi1, (uint16) 0, (unsigned char)SHIFT_BIT),v0, vprYinterMask); 
				v2 = vselect(vmac3(psl, v2, bi0, v3, bi1, (uint16) 0, (unsigned char)SHIFT_BIT),v2, vprYinterMask);
				
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
			else
				vprRightMask = 0xffff;
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
			// light[k] 	= plight[y*w + x + k];
			light16[0]  	= *(ushort16*)&plight[y*w + x];
			light16[1]  	= *(ushort16*)&plight[y*w + x + 16];
			light16[2]  	= *(ushort16*)&plight[y*w + x + 32];
			light16[3]  	= *(ushort16*)&plight[y*w + x + 48];


			// light[k]>16*1023
			light16[0]  	= (ushort16)vcmpmov(lt, light16[0], (ushort16)16*1023);
			light16[1]  	= (ushort16)vcmpmov(lt, light16[1], (ushort16)16*1023);
			light16[2]  	= (ushort16)vcmpmov(lt, light16[2], (ushort16)16*1023);
			light16[3]  	= (ushort16)vcmpmov(lt, light16[3], (ushort16)16*1023);

			// lindex[k]  	= light[k] >> 11;
			lindex16[0] 	= vshiftr(light16[0], (unsigned char) 11);
			lindex16[1] 	= vshiftr(light16[1], (unsigned char) 11);
			lindex16[2] 	= vshiftr(light16[2], (unsigned char) 11);
			lindex16[3] 	= vshiftr(light16[3], (unsigned char) 11);


			// load left[lindex[k]],left[lindex[k]+1]
			// load right[lindex[k]],left[lindex[k]+1]
			vpld((unsigned short*)left_vecc,  lindex16[0], v0, v1);
			vpld((unsigned short*)right_vecc, lindex16[0], v2, v3);

			vpld((unsigned short*)left_vecc,  lindex16[1], v4, v5);
			vpld((unsigned short*)right_vecc, lindex16[1], v6, v7);

			vpld((unsigned short*)left_vecc,  lindex16[2], v8, v9);
			vpld((unsigned short*)right_vecc, lindex16[2], v10, v11);
		
			vpld((unsigned short*)left_vecc,  lindex16[3], v12, v13);
			vpld((unsigned short*)right_vecc, lindex16[3], v14, v15);

			// (MAX_BIT_VALUE - ((x + k) & MAX_BIT_V_MINUS1)) || (x + k) & MAX_BIT_V_MINUS1)
			bifactor_xAxis[0] = (uchar32)vselect(vsub(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)(x&MAX_BIT_V_MINUS1)), 0x0000ffff);
			bifactor_xAxis[1] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 16)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)((x + 16)&MAX_BIT_V_MINUS1)), 0x0000ffff);
			bifactor_xAxis[2] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 32)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)((x + 32)&MAX_BIT_V_MINUS1)), 0x0000ffff);
			bifactor_xAxis[3] = (uchar32)vselect(vsub(biBase,(uchar32)((x + 48)&MAX_BIT_V_MINUS1)), 
									 vadd(biBase,(uchar32)((x + 48)&MAX_BIT_V_MINUS1)), 0x0000ffff);
		
			// char <= 255, so 256 is overflow, need do speical.
			// v0 the first unit do copy v0[0] when "0 == (x&MAX_BIT_V_MINUS1) "

			//v0 = vmac3(splitsrc, psl, v0, v2, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT) & vprRightMask;
			//v1 = vmac3(splitsrc, psl, v1, v3, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT);

			v0 = vselect(vmac3(splitsrc, psl, v0, v2, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT),v0, vprRightMask); 
			v1 = vselect(vmac3(splitsrc, psl, v1, v3, bifactor_xAxis[0], (uint16) 0, (unsigned char)SHIFT_BIT),v1, vprRightMask); 
			v2 = vmac3(splitsrc, psl, v4, v6, bifactor_xAxis[1], (uint16) 0, (unsigned char)SHIFT_BIT);
			v3 = vmac3(splitsrc, psl, v5, v7, bifactor_xAxis[1], (uint16) 0, (unsigned char)SHIFT_BIT);
			v4 = vmac3(splitsrc, psl, v8, v10, bifactor_xAxis[2], (uint16) 0, (unsigned char)SHIFT_BIT);
			v5 = vmac3(splitsrc, psl, v9, v11, bifactor_xAxis[2], (uint16) 0, (unsigned char)SHIFT_BIT);
			v6 = vmac3(splitsrc, psl, v12, v14, bifactor_xAxis[3], (uint16) 0, (unsigned char)SHIFT_BIT);
			v7 = vmac3(splitsrc, psl, v13, v15, bifactor_xAxis[3], (uint16) 0, (unsigned char)SHIFT_BIT);
		 

			
			//weight[k] 	= (weight1[k]*(2048 - (light[k] & 2047)) + weight2[k]*(light[k]  & 2047)) / 2048;
			bi1_vecc[0] = vand(light16[0], 				(unsigned short)2047);
			bi1_vecc[1] = vand(light16[1], 				(unsigned short)2047);
			bi1_vecc[2] = vand(light16[2], 				(unsigned short)2047);
			bi1_vecc[3] = vand(light16[3], 				(unsigned short)2047);

			bi0_vecc[0] = vsub((unsigned short)2048,     bi1_vecc[0]);
			bi0_vecc[1] = vsub((unsigned short)2048,     bi1_vecc[1]);
			bi0_vecc[2] = vsub((unsigned short)2048,     bi1_vecc[2]);
			bi0_vecc[3] = vsub((unsigned short)2048,     bi1_vecc[3]);

			vacc0 	 	= vmpy(v0, bi0_vecc[0]);			
			vacc1 	 	= vmpy(v2, bi0_vecc[1]);			
			vacc2 	 	= vmpy(v4, bi0_vecc[2]);			
			vacc3 	 	= vmpy(v6, bi0_vecc[3]);			

			weight16[0] =  (ushort16)vmac(psl, v1, bi1_vecc[0], vacc0, (unsigned char)11);
			weight16[1] =  (ushort16)vmac(psl, v3, bi1_vecc[1], vacc1, (unsigned char)11);
			weight16[2] =  (ushort16)vmac(psl, v5, bi1_vecc[2], vacc2, (unsigned char)11);
			weight16[3] =  (ushort16)vmac(psl, v7, bi1_vecc[3], vacc3, (unsigned char)11);


			
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight1 + 0,   v0, 16);
			ret += check_ushort16_vecc_result(weight1 + 16,  v2, 16);
			ret += check_ushort16_vecc_result(weight1 + 32,  v4, 16);
			ret += check_ushort16_vecc_result(weight1 + 48,  v6, 16);
			ret += check_ushort16_vecc_result(weight2 + 0,   v1, 16);
			ret += check_ushort16_vecc_result(weight2 + 16,  v3, 16);
			ret += check_ushort16_vecc_result(weight2 + 32,  v5, 16);
			ret += check_ushort16_vecc_result(weight2 + 48,  v7, 16);
			if (ret){
				PRINT_C_GROUP("w1",weight1,0, 64,stderr);
				PRINT_C_GROUP("w2",weight2,0, 64,stderr);
			}
			assert(ret == 0);
			ret += check_ushort16_vecc_result(weight+  0, weight16[0], 16);
			ret += check_ushort16_vecc_result(weight+ 16, weight16[1], 16);
			ret += check_ushort16_vecc_result(weight+ 32, weight16[2], 16);
			ret += check_ushort16_vecc_result(weight+ 48, weight16[3], 16);
			if (ret){
				PRINT_CEVA_VRF("weight16",weight16[0],stderr);
				PRINT_CEVA_VRF("weight16",weight16[1],stderr);
				PRINT_CEVA_VRF("weight16",weight16[2],stderr);
				PRINT_CEVA_VRF("weight16",weight16[3],stderr);
			}
			assert(ret == 0);

			// x+VECC_ONCE_LEN for one loop 
			for  ( k = 0 ; k < VECC_ONCE_LEN ; k++ )
			{
			#if 0
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
			#else

				if((light[k]-512) > weight[k])
						weight[k] = light[k]-512;
				if((light[k]+512) < weight[k])
						weight[k] = light[k]+512;
				
				
				weight_bak[k] = clip16bit_ceva(weight[k], light[k]-512, light[k]+512);
				assert(weight[k]==weight_bak[k]);

				if((weight[k] - blacklevel*4) > 0)
					weight[k]  = weight[k] - blacklevel*4;
				else
					weight[k]  = 0;
				//weight_bak = clip16bit_ceva(weight_bak - blacklevel*4, 0, abs(weight_bak - blacklevel*4));
				//assert(weight==weight_bak);
			#endif
				
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

			// clip(weight,light-512,light+512)
			weight16[0] 	= vclip(weight16[0],(ushort16)vsub((short16)light16[0],(unsigned short)512),(ushort16)vadd((short16)light16[0] ,(unsigned short)512)); // weight = light+512 or // weight = light+512
			weight16[1]  	= vclip(weight16[1],(ushort16)vsub((short16)light16[1],(unsigned short)512),(ushort16)vadd((short16)light16[1] ,(unsigned short)512)); // weight = light+512 or // weight = light+512
			weight16[2]  	= vclip(weight16[2],(ushort16)vsub((short16)light16[2],(unsigned short)512),(ushort16)vadd((short16)light16[2] ,(unsigned short)512)); // weight = light+512 or // weight = light+512
			weight16[3]  	= vclip(weight16[3],(ushort16)vsub((short16)light16[3],(unsigned short)512),(ushort16)vadd((short16)light16[3] ,(unsigned short)512)); // weight = light+512 or // weight = light+512


			// (weight - blacklevel*4) > 0,
			vpr2[0]			= vcmp(gt,vsub(weight16[0],(ushort16)(blacklevel*4)),	0);  
			vpr2[1] 		= vcmp(gt,vsub(weight16[1],(ushort16)(blacklevel*4)),	0);  
			vpr2[2] 		= vcmp(gt,vsub(weight16[2],(ushort16)(blacklevel*4)),	0);  
			vpr2[3] 		= vcmp(gt,vsub(weight16[3],(ushort16)(blacklevel*4)),	0);		


			// choose the "weight - blacklevel*4" or "0" to weight
			weight16[0] 	= (ushort16)vselect(vsub(weight16[0],(ushort16)(blacklevel*4)),	0 , vpr2[0] );  
			weight16[1]  	= (ushort16)vselect(vsub(weight16[1],(ushort16)(blacklevel*4)),	0 , vpr2[1] );  
			weight16[2]  	= (ushort16)vselect(vsub(weight16[2],(ushort16)(blacklevel*4)),	0 , vpr2[2] );  
			weight16[3]  	= (ushort16)vselect(vsub(weight16[3],(ushort16)(blacklevel*4)),	0 , vpr2[3] );  
		
			// lindex = weight >> 4;
			lindex16[0] 	= vshiftr(weight16[0],  (unsigned char)4);
			lindex16[1] 	= vshiftr(weight16[1],  (unsigned char)4);
			lindex16[2] 	= vshiftr(weight16[2],  (unsigned char)4);
			lindex16[3] 	= vshiftr(weight16[3],  (unsigned char)4);

			vpld((unsigned short*)scale_table , lindex16[0], v0, v1);// v0 is scale_table[lindex[k]], v1 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[1], v2, v3);// v2 is scale_table[lindex[k]], v3 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[2], v4, v5);// v4 is scale_table[lindex[k]], v5 is scale_table[lindex[k]+1]
			vpld((unsigned short*)scale_table , lindex16[3], v6, v7);// v6 is scale_table[lindex[k]], v7 is scale_table[lindex[k]+1]

			//  (weight & 15)
			weightLow16[0] = (uchar32)vand(weight16[0],(unsigned short )15); 
			weightLow16[1] = (uchar32)vand(weight16[1],(unsigned short )15); 
			weightLow16[2] = (uchar32)vand(weight16[2],(unsigned short )15); 
			weightLow16[3] = (uchar32)vand(weight16[3],(unsigned short )15); 

			// (16 - (weight & 15)) || (weight & 15)  
			weightLow16[0] = (uchar32)vperm(weightLow16[0],vsub((uchar32)16,weightLow16[0]),vconfig0);
			weightLow16[1] = (uchar32)vperm(weightLow16[1],vsub((uchar32)16,weightLow16[1]),vconfig0);
			weightLow16[2] = (uchar32)vperm(weightLow16[2],vsub((uchar32)16,weightLow16[2]),vconfig0);
			weightLow16[3] = (uchar32)vperm(weightLow16[3],vsub((uchar32)16,weightLow16[3]),vconfig0);

	
			// weight = (scale_table[lindex] * (16 - (weight & 15)) + scale_table[lindex + 1] * (weight & 15) + 8) >> 4;
			weight16[0] 	= vmac3(splitsrc, psl, v1, v0, weightLow16[0], (uint16)8, (unsigned char)4);
			weight16[1] 	= vmac3(splitsrc, psl, v3, v2, weightLow16[1], (uint16)8, (unsigned char)4);
			weight16[2] 	= vmac3(splitsrc, psl, v5, v4, weightLow16[2], (uint16)8, (unsigned char)4);
			weight16[3] 	= vmac3(splitsrc, psl, v7, v6, weightLow16[3], (uint16)8, (unsigned char)4);

	 		// read pixel_in                       
			v16     = vpld(ptmp1,     offset_in);      
			v17     = vpld((ptmp1+16),offset_in); 
			v18     = vpld((ptmp1+32),offset_in); 
			v19     = vpld((ptmp1+48),offset_in); 

			// *pixel_in -= blacklevel*2, if >0, copy; else 0;
			v16		= vsubsat(v16,	(unsigned short)(blacklevel*2));// & vpr3[0]; // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v17 	= vsubsat(v17,	(unsigned short)(blacklevel*2));// & vpr3[1]; // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v18 	= vsubsat(v18,	(unsigned short)(blacklevel*2));// & vpr3[2]; // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
			v19 	= vsubsat(v19,	(unsigned short)(blacklevel*2));// & vpr3[3]; // v1 > 0, need do ajdust pixel_out. else set to blacklevel/4.
		

			// *(pGainMat + y*w + x) = (RK_U32)weight; 
			vst(weight16[0],(ushort16*)ptmp5,	  vprMask); // vprMask handle with unalign in image board.
			vst(weight16[1],(ushort16*)(ptmp5+16),vprMask); // vprMask handle with unalign in image board.
			vst(weight16[2],(ushort16*)(ptmp5+32),vprMask); // vprMask handle with unalign in image board.
			vst(weight16[3],(ushort16*)(ptmp5+48),vprMask); // vprMask handle with unalign in image board.

			// *pixel_out++ = clip10bit(((*pixel_in++) - blacklevel * 2)*weight / 1024 + blacklevel/4);
			v0 		= vclip((ushort16)vmac(psl, v16, weight16[0], const16mpy256, (unsigned char)10), (ushort16) 0, (ushort16) 1023);// & vpr3[0];     
			v1 		= vclip((ushort16)vmac(psl, v17, weight16[1], const16mpy256, (unsigned char)10), (ushort16) 0, (ushort16) 1023);// & vpr3[1];     
			v2 		= vclip((ushort16)vmac(psl, v18, weight16[2], const16mpy256, (unsigned char)10), (ushort16) 0, (ushort16) 1023);// & vpr3[2];     
			v3 		= vclip((ushort16)vmac(psl, v19, weight16[3], const16mpy256, (unsigned char)10), (ushort16) 0, (ushort16) 1023);// & vpr3[3];     

			vst(v0,(ushort16*)ptmp3     ,	vprMask); // vprMask handle with unalign in image board.
			vst(v1,(ushort16*)(ptmp3+16),	vprMask); // vprMask handle with unalign in image board.
			vst(v2,(ushort16*)(ptmp3+32),	vprMask); // vprMask handle with unalign in image board.
			vst(v3,(ushort16*)(ptmp3+48),	vprMask); // vprMask handle with unalign in image board.

			ptmp1 += VECC_ONCE_LEN;
			ptmp3 += VECC_ONCE_LEN;
			ptmp5 += VECC_ONCE_LEN;
			
		#if DEBUG_VECC
			ret += check_ushort16_vecc_result(weight+0,   weight16[0], 16);
			ret += check_ushort16_vecc_result(weight+16,  weight16[1], 16);
			ret += check_ushort16_vecc_result(weight+32,  weight16[2], 16);
			ret += check_ushort16_vecc_result(weight+48,  weight16[3], 16);
			if (ret){
				PRINT_C_GROUP("weight",weight, 0, 16,stderr);
				PRINT_CEVA_VRF("weight16",weight16[0],stderr);
			}
			
			ret += check_wdr_result(ptmp2 - VECC_ONCE_LEN,  ptmp3 - VECC_ONCE_LEN, 64, 1);	
			if (ret){
				PRINT_CEVA_VRF("out",v1,stderr);
			}	
			assert(ret == 0);
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
