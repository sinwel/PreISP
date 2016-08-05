
#include "rk_wdr.h"
#include <assert.h>
#include <vec-c.h>


extern unsigned short cure_table[24][961];

void ceva_bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5) // 20160701
{
	int		i;
	int		x, y;
	RK_U16		*pcount[9], **pcount_mat;
	RK_U32		*pweight[9], **pweight_mat;
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
		pcount[i] = (RK_U16*)malloc(sw*sh*sizeof(RK_U16));
		pweight[i] = (RK_U32*)malloc(sw*sh*sizeof(RK_U32));
		memset(pcount[i], 0, sw*sh*sizeof(RK_U16));
		memset(pweight[i], 0, sw*sh*sizeof(RK_U32));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
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
				{
					#if WDR_USE_CEVA_VECC
					// actully is 17x13 = 221
					pweight_vecc[i][y*sw + x] = (RK_U16)(4*pweight_mat[i][y*sw + x] / (pcount_mat[i][y*sw + x])); 
					#endif	
					pweight_mat[i][y*sw + x] = (RK_U16)(4*pweight_mat[i][y*sw + x] / (pcount_mat[i][y*sw + x]));
				}
				else
				{	
					#if WDR_USE_CEVA_VECC
					pweight_vecc[i][y*sw + x] = 0; 
					#endif		
					pweight_mat[i][y*sw + x] = 0;
				}
				if (pweight_mat[i][y*sw + x]>16383)
				{
					#if WDR_USE_CEVA_VECC
					pweight_vecc[i][y*sw + x] = 16383; 
					#endif
					pweight_mat[i][y*sw + x] = 16383;

				}
			}
		}
	}

	RK_S16  left[9], right[9];
	RK_S16 weight1;
	RK_S16 weight2;
	RK_S16 weight;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++) // input/output 16 pixel result.
		{
			
			if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
				for (i = 0; i < 9; i++)
				{
					// (14BIT * 8 + 14BIT *8) / 8BIT
					left[i] =  (pweight_mat[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT)]     * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
					
					right[i] = (pweight_mat[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + 1] * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_mat[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw + 1] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
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

			light >>= 2;
			weight = (weight1*(512 - (light & 511)) + weight2*(light & 511)) / 512;
			light <<= 2;

		#if 0//WDR_USE_CEVA_VECC	
			plight16 = (ushort16 *)&plight[y*w + x];
			light16  = *plight16; 
			ushort16 vmaxValue = (ushort16)16*1023;
			light16  = (ushort16)vcmpmov(lt, light16, vmaxValue);
			lindex16 = (ushort16)vshiftr(light16, (unsigned char) 11);

			weight1 = (left[lindex]     * (MAX_BIT_VALUE - (x & MAX_BIT_V_MINUS1)) + right[lindex]     * (x & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
			weight2 = (left[lindex + 1] * (MAX_BIT_VALUE - (x & MAX_BIT_V_MINUS1)) + right[lindex + 1] * (x & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
			weight = (weight1*(512 - (light & 511)) + weight2*(light & 511)) / 512;
			
		#endif
			{
				if (abs(weight-light*1.0)>512)
				{
					if(light>weight)
						weight = light-512;
					else
						weight = light+512;
				}
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


void interpolationYaxis()
{
	
	static RK_U16	pweight_vecc[9][256] = {0};// actully is 17x13 = 221
	ushort16* 			plight16;
	ushort16 			light16;
	ushort16 			lindex16;
	short16 inN	;
	ushort16 v0,v1,v2,v3,v4,v5;
	const unsigned short* inM;
	short chunkStrideBytes = 256, ptrChunks[16] = {0};
	short offset;
	unsigned short bi0,bi1;

	uint16 vacc0,vacc1,vacc2,vacc3;
	unsigned short left[9],right[9];
	int x, y, w = 4164, h=3136;
  	int sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	int i,j,ret = 0;
	//init pweight_vecc
	for ( i = 0 ; i < 9 ; i++ )
	    for ( j = 0 ; j < sw*sh ; j++ )
			pweight_vecc[i][j] = i*1024+j;	
	

	
	vacc0 = 0;
	for (y = 1; y < h; y++)
	{
		for (x = 0; x < w; x+=MAX_BIT_VALUE) // input/output 16 pixel result.
		{
			
			//if ((x & MAX_BIT_V_MINUS1) == 0)// the first 2D block (x,y) do once.
			{
				for (i = 0; i < 9; i++)
				{
					// (14BIT * 8 + 14BIT *8) / 8BIT
					left[i] =  (pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT)]     * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
					
					right[i] = (pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + 1] * (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1)) 
						+ pweight_vecc[i][(y >> SHIFT_BIT)*sw + (x >> SHIFT_BIT) + sw + 1] * (y & MAX_BIT_V_MINUS1)) / MAX_BIT_VALUE;
				}

				inM = pweight_vecc[0];				
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
				inN = *(short16*)ptrChunks;

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
				inN = *(short16*)ptrChunks;
				vpld(inM, inN, v1, v3);
				//PRINT_CEVA_VRF("v1",v1,stderr);
				//PRINT_CEVA_VRF("v3",v3,stderr);

				/* DO interpolation, left by v0/v1, right by v2/v3, when y is zero, do copy v0/v2 only rather than vmac3 */
				if (y==0)
				{
					v0 = v0;
					v2 = v2;
				}
				else 
				{
					bi0 = (MAX_BIT_VALUE - (y & MAX_BIT_V_MINUS1));
					bi1 = (y & MAX_BIT_V_MINUS1);

					v0 = vmac3(psl, v0, bi0, v1, bi1, vacc0, (unsigned char)SHIFT_BIT);
					v2 = vmac3(psl, v2, bi0, v3, bi1, vacc0, (unsigned char)SHIFT_BIT);

				}

				ret = check_ushort16_vecc_result(left,  v0, 9);
				ret += check_ushort16_vecc_result(right, v2, 9);
				if (ret){
					PRINT_CEVA_VRF("v0",v0,stderr);
					PRINT_CEVA_VRF("v2",v2,stderr);
				}
				assert(ret == 0);
			}
		}
	}

}

void interpolationXaxis()
{}
