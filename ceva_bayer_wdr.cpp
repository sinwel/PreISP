
#include "rk_wdr.h"
#include <assert.h>

unsigned short cure_table[24][961] = 
{
	#include "data/wdr_cure_tab.dat"
};


int check_wdr_result(RK_U16* data1, RK_U16* data2,int Wid ,int  Hgt)
{
	for ( int i = 0 ; i < Hgt ; i++ )
	{
	    for ( int j = 0 ; j < Wid ; j++ )
	    {
	        if(data1[i*Wid + j] != data2[i*Wid + j] )
			{	
				return -1;
	        }
	    }
	}
	return 0;
}

void cul_wdr_cure2(unsigned short *table, float exp_times)
{
	
	int	idx;
	int	i;
	
	if (exp_times>24)
		{
			exp_times = 24;
		}
		exp_times = exp_times-1;
		
		idx = exp_times;
		if(exp_times == idx)
		{
			for(i=0;i<961;i++)
			{
				table[i] = cure_table[idx][i];
			}
		}
		else
		{
			float  s1,s2;
			
			s1 = idx+1-exp_times;
			s2 = exp_times-idx;
			for(i=0;i<961;i++)
			{
				table[i] = (float)cure_table[idx][i]*s1+(float)cure_table[idx+1][i]*s2;
			}
		}
}

void cul_wdr_cure(unsigned short *table, unsigned short exp_times)
{
	double i, tmp, scale, g;




	g = 1 + log(double(exp_times)) / log(1.0/16);

	for (i = 1; i < 1024-64; i++)
	{
		tmp = i / (1023-64);

		//scale = 4*tmp-8*tmp*tmp+7*tmp*tmp*tmp-2*tmp*tmp*tmp*tmp;
		scale = 2.5*tmp-5*tmp*tmp+5.5*tmp*tmp*tmp-2*tmp*tmp*tmp*tmp; // 0606
		

		scale = scale / tmp;
		if (scale>exp_times)
			scale = exp_times;
		table[(unsigned short)(i)] = scale * 128;
	}
	table[0] = 0;
}

inline unsigned short clip16bit(unsigned long x)
{
	if (x > 65535)
		return 65535;
	else
		return x;
}

inline unsigned short clip10bit(unsigned short x)
{
	if (x > 1023)
		return 1023;
	else
		return x;
}

//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, int max_scale)
//void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_F32 testParams_5)
void bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5) // 20160701
{
	int		i;
	int		x, y;
	unsigned long		*pcount[9], **pcount_mat;
	unsigned long		*pweight[9], **pweight_mat;
	unsigned short		sw, sh;
	unsigned short		light;

	unsigned short		*p1, *p2, *p3;
	unsigned short		*plight;
	unsigned short		scale_table[1025];

	int lindex; //
	int blacklevel=256;

	cul_wdr_cure2(scale_table, max_scale);

#if WDR_USE_THUMB_LUMA

	
    int wThumb        = w  / SCALER_FACTOR_R2T;      // Thumb data width  (floor)
    int hThumb        = h  / SCALER_FACTOR_R2T;      // Thumb data height (floor)
	int Thumb16Stride   = (wThumb*16 + 31) / 32 * 4; 

  #if 0
  	sw = w/SPLIT_SIZE;
	sh = h/SPLIT_SIZE;
	for (i = 0; i < 9; i++)
	{
		pcount[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		pweight[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		memset(pcount[i], 0, sw*sh*sizeof(unsigned long));
		memset(pweight[i], 0, sw*sh*sizeof(unsigned long));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
	plight = (unsigned short*)malloc(w*h*sizeof(unsigned short));


	unsigned short*   pTmp0 = g_BaseThumbBuf;
	for (y = 0; y < sh ; y++)
	{
		for (x = 0; x < sw ; x++)
		{
			int idx, idy; 
			int ScaleDownlight = 0;

			for ( int ii = 0 ; ii < (1<<SHIFT_BIT_SCALE)  ; ii++ )
				for ( int jj = 0 ; jj < (1<<SHIFT_BIT_SCALE)  ; jj++ )
					ScaleDownlight += pTmp0[ii*wThumb + jj];

			ScaleDownlight  >>= (2*SHIFT_BIT - 4 );
			
			lindex = ((RK_U32)ScaleDownlight + 1024) >> 11;

			//idx = (x + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			//idy = (y + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			idx = x;
			idy = y;
			pcount_mat [lindex][idy*sw + idx] = pcount_mat [lindex][idy*sw + idx] + 1;
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + ScaleDownlight;

		}
		pTmp0 += wThumb;
	}

	/* SPLIT_SIZE*SPLIT_SIZE 的点设置为 平均值 */
	for (y = 0; y < sh ; y++)
	{
		for (x = 0; x < sw ; x++)
		{
			unsigned long sumlight = 0;
			unsigned short*   pTmp1 = pixel_in + (y*w + x)*SPLIT_SIZE ;
			for ( int ii = 0 ; ii < SPLIT_SIZE ; ii++ )
				for ( int jj = 0 ; jj < SPLIT_SIZE ; jj++ )
					sumlight += pTmp1[ii*w + jj];
			sumlight >>= 11; // 14 bit, linear gain is 8

			for ( int ii = 0 ; ii < SPLIT_SIZE ; ii++ )
				for ( int jj = 0 ; jj < SPLIT_SIZE; jj++ )
					plight[(y*w + x)*SPLIT_SIZE + ii*w + jj] = sumlight;
		}
	}
  #else
  	sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	for (i = 0; i < 9; i++)
	{
		pcount[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		pweight[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		memset(pcount[i], 0, sw*sh*sizeof(unsigned long));
		memset(pweight[i], 0, sw*sh*sizeof(unsigned long));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
	plight = (unsigned short*)malloc(w*h*sizeof(unsigned short));


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

			
			lindex = ((RK_U32)ScaleDownlight + 1024) >> 11;

			idx = (x + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			idy = (y + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			assert(idy < sh);
			assert(idx < sw);
			pcount_mat [lindex][idy*sw + idx] = pcount_mat [lindex][idy*sw + idx] + 1;
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + ScaleDownlight;

		}
		p1 += wThumb;
		p2 += wThumb;
		p3 += wThumb;
	}


  	p1 = pixel_in; // Gain is 8
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;

	for (y = 1; y < h - 1 ; y++)
	{
		for (x = 1; x < w-1 ; x++)
		{
			int sumlight = 0;
			sumlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			sumlight >>= 3; // 10 + 3 + 4 - 3
			plight[y*w + x] = sumlight;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}

  #endif
	
#else
	sw = (w + 127) >> 7;
	sh = (h + 127) >> 7;
	//sw = (w + 7) >> 3;
	//sh = (h + 7) >> 3;
	sw = sw + 1;
	sh = sh + 1;
	for (i = 0; i < 9; i++)
	{
		pcount[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		pweight[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		memset(pcount[i], 0, sw*sh*sizeof(unsigned long));
		memset(pweight[i], 0, sw*sh*sizeof(unsigned long));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
	plight = (unsigned short*)malloc(w*h*sizeof(unsigned short));

	p1 = pixel_in;
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;


	for (y = 1; y < h - 1; y++)
	{
		for (x = 1; x < w - 1; x++)
		{
			int idx, idy;
			#if 1
 			//if (1/*testParams_5 == 0*/)
 			{
 				light = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
 				light = light / 8;
 			}
			#else // use the max for 9 pixel.
 			//else //if (testParams_5 == 1)
 			{
				light = p1[x - 1];
				light = MAX(light, p1[x]);
				light = MAX(light, p1[x+1]);
				light = MAX(light, p2[x-1]);
				light = MAX(light, p2[x]);
				light = MAX(light, p2[x+1]);
				light = MAX(light, p3[x-1]);
				light = MAX(light, p3[x]);
				light = MAX(light, p3[x+1]);
				light = light*2; // Gain is 8, ao the 10bit +3 bit + 1 bit >> 11  = 3bit [ 0-8 ]
			}
			#endif
			

			plight[y*w + x] = light;
			lindex = ((RK_U32)light + 1024) >> 11;

			idx = (x + 64) >> 7;
			idy = (y + 64) >> 7;
			//idx = (x + 4) >> 3;
			//idy = (y + 4) >> 3;
			pcount_mat[lindex][idy*sw + idx] = pcount_mat[lindex][idy*sw + idx] + 1;
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + light;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}

/*
	for (y = 0; y < sh; y++)
	{
		for (x = 1; x < sw; x++)
		{
			for (int line=0;line<9;line++){
				fprintf(stderr,"count  = %d ",pcount_mat[line][y*sw + x] );
				fprintf(stderr,"weight = %d \n",pweight_mat[line][y*sw + x]);
			}
		}
	}
	*/
#endif

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




	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < sh; y++)
		{
			for (x = 0; x < sw; x++)
			{
				if (pcount_mat[i][y*sw + x])
					pweight_mat[i][y*sw + x] = ((double)64.0*(double)pweight_mat[i][y*sw + x]) / (double)pcount_mat[i][y*sw + x];
				else
					pweight_mat[i][y*sw + x] = 0;

				if (pweight_mat[i][y*sw + x]>16383)
					pweight_mat[i][y*sw + x] = 16383;
			}
		}
	}


	unsigned long  left[9], right[9];
	unsigned long weight1;
	unsigned long weight2;
	unsigned long weight;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{

		#if WDR_USE_THUMB_LUMA
			
			if ((x & MAX_BIT_V_MINUS1) == 0)
			{
				for (i = 0; i < 9; i++)
				{
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
			weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

//			*pixel_out++ = weight/16;


			// 0614-1604
			//if (testParams_5 == 1)
			//{
			//	weight = light;
			//}
			//else
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
			//fprintf(stderr,"weight  = %d \n ", weight );
			// Gain = weight/128
			*(pGainMat + y*w + x) = (RK_U32)weight; // for zlf-SpaceDenoise

		#else
			 
			if ((x & 127) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 7)*sw + (x >> 7)] * (128 - (y & 127)) + pweight_mat[i][(y >> 7)*sw + (x >> 7) + sw] * (y & 127)) / 128;
					right[i] = (pweight_mat[i][(y >> 7)*sw + (x >> 7) + 1] * (128 - (y & 127)) + pweight_mat[i][(y >> 7)*sw + (x >> 7) + sw + 1] * (y & 127)) / 128;
				}
			}
			/*
			if ((x & 255) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 8)*sw + (x >> 8)] * (256 - (y & 255)) + pweight_mat[i][(y >> 8)*sw + (x >> 8) + sw] * (y & 255)) / 256;
					right[i] = (pweight_mat[i][(y >> 8)*sw + (x >> 8) + 1] * (256 - (y & 255)) + pweight_mat[i][(y >> 8)*sw + (x >> 8) + sw + 1] * (y & 255)) / 256;
				}
			}
			
			if ((x & 7) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 3)*sw + (x >> 3)] * (8 - (y & 7)) + pweight_mat[i][(y >> 3)*sw + (x >> 3) + sw] * (y & 7)) / 8;
					right[i] = (pweight_mat[i][(y >> 3)*sw + (x >> 3) + 1] * (8 - (y & 7)) + pweight_mat[i][(y >> 3)*sw + (x >> 3) + sw + 1] * (y & 7)) / 8;
				}
			}
			*/
			light = plight[y*w + x];
			if (light>16*1023)
			{
				light=16*1023;
			}

			lindex = light >> 11;
// 			unsigned long weight1 = (left[lindex] * (128 - (x & 127)) + right[lindex] * (x & 127)) / 128;
// 			unsigned long weight2 = (left[lindex + 1] * (128 - (x & 127)) + right[lindex + 1] * (x & 127)) / 128;
// 			unsigned long weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

			weight1 = (left[lindex] * (128 - (x & 127)) + right[lindex] * (x & 127)) / 128;
			weight2 = (left[lindex + 1] * (128 - (x & 127)) + right[lindex + 1] * (x & 127)) / 128;

			//weight1 = (left[lindex] * (256 - (x & 255)) + right[lindex] * (x & 255)) / 256;
			//weight2 = (left[lindex + 1] * (256 - (x & 255)) + right[lindex + 1] * (x & 255)) / 256;

			//weight1 = (left[lindex] * (8 - (x & 7)) + right[lindex] * (x & 7)) / 8;
			//weight2 = (left[lindex + 1] * (8 - (x & 7)) + right[lindex + 1] * (x & 7)) / 8;
			
			weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

//			*pixel_out++ = weight/16;


			// 0614-1604
			if (testParams_5 == 1)
			{
				weight = light;
			}
			else
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
			//fprintf(stderr,"weight  = %d \n ", weight );
			// Gain = weight/128
			*(pGainMat + y*w + x) = (RK_U32)weight; // for zlf-SpaceDenoise


		#endif

			//*pixel_out++ = clip10bit(((unsigned long)(*pixel_in++) - 64 * 4)*weight / 512 + 64);
			if(*pixel_in>blacklevel*2)
				*pixel_out++ = clip10bit(((unsigned long)(*pixel_in++) - blacklevel * 2)*weight / 1024 + blacklevel/4);
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



void ceva_bayer_wdr(unsigned short *pixel_in, unsigned short *pixel_out, int w, int h, float max_scale, RK_U32* pGainMat, RK_F32 testParams_5) // 20160701
{
	int		i;
	int		x, y;
	unsigned long		*pcount[9], **pcount_mat;
	unsigned long		*pweight[9], **pweight_mat;
	unsigned short		sw, sh;
	unsigned short		light;

	unsigned short		*p1, *p2, *p3;
	unsigned short		*plight;
	unsigned short		scale_table[1025];

	int lindex; //
	int blacklevel=256;

	cul_wdr_cure2(scale_table, max_scale);

#if WDR_USE_THUMB_LUMA

    int wThumb        = w  / SCALER_FACTOR_R2T;      // Thumb data width  (floor)
    int hThumb        = h  / SCALER_FACTOR_R2T;      // Thumb data height (floor)
	int Thumb16Stride   = (wThumb*16 + 31) / 32 * 4; 

  	sw = (w + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	sh = (h + (SPLIT_SIZE>>1))/SPLIT_SIZE + 1;
	for (i = 0; i < 9; i++)
	{
		pcount[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		pweight[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		memset(pcount[i], 0, sw*sh*sizeof(unsigned long));
		memset(pweight[i], 0, sw*sh*sizeof(unsigned long));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
	plight = (unsigned short*)malloc(w*h*sizeof(unsigned short));


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

			
			lindex = ((RK_U32)ScaleDownlight + 1024) >> 11;

			idx = (x + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			idy = (y + ((1<<SHIFT_BIT_SCALE)/2)) >> SHIFT_BIT_SCALE;
			assert(idy < sh);
			assert(idx < sw);
			pcount_mat [lindex][idy*sw + idx] = pcount_mat [lindex][idy*sw + idx] + 1;
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + ScaleDownlight;

		}
		p1 += wThumb;
		p2 += wThumb;
		p3 += wThumb;
	}


  	p1 = pixel_in; // Gain is 8
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;

	for (y = 1; y < h - 1 ; y++)
	{
		for (x = 1; x < w-1 ; x++)
		{
			int sumlight = 0;
			sumlight = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
			sumlight >>= 3; // 10 + 3 + 4 - 3
			plight[y*w + x] = sumlight;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}

	
#else
	sw = (w + 127) >> 7;
	sh = (h + 127) >> 7;
	//sw = (w + 7) >> 3;
	//sh = (h + 7) >> 3;
	sw = sw + 1;
	sh = sh + 1;
	for (i = 0; i < 9; i++)
	{
		pcount[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		pweight[i] = (unsigned long*)malloc(sw*sh*sizeof(unsigned long));
		memset(pcount[i], 0, sw*sh*sizeof(unsigned long));
		memset(pweight[i], 0, sw*sh*sizeof(unsigned long));
	}
	pcount_mat = pcount;
	pweight_mat = pweight;
	plight = (unsigned short*)malloc(w*h*sizeof(unsigned short));

	p1 = pixel_in;
	p2 = pixel_in + w;
	p3 = pixel_in + 2 * w;


	for (y = 1; y < h - 1; y++)
	{
		for (x = 1; x < w - 1; x++)
		{
			int idx, idy;
			#if 1
 			//if (1/*testParams_5 == 0*/)
 			{
 				light = p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];
 				light = light / 8;
 			}
			#else // use the max for 9 pixel.
 			//else //if (testParams_5 == 1)
 			{
				light = p1[x - 1];
				light = MAX(light, p1[x]);
				light = MAX(light, p1[x+1]);
				light = MAX(light, p2[x-1]);
				light = MAX(light, p2[x]);
				light = MAX(light, p2[x+1]);
				light = MAX(light, p3[x-1]);
				light = MAX(light, p3[x]);
				light = MAX(light, p3[x+1]);
				light = light*2; // Gain is 8, ao the 10bit +3 bit + 1 bit >> 11  = 3bit [ 0-8 ]
			}
			#endif
			

			plight[y*w + x] = light;
			lindex = ((RK_U32)light + 1024) >> 11;

			idx = (x + 64) >> 7;
			idy = (y + 64) >> 7;
			//idx = (x + 4) >> 3;
			//idy = (y + 4) >> 3;
			pcount_mat[lindex][idy*sw + idx] = pcount_mat[lindex][idy*sw + idx] + 1;
			pweight_mat[lindex][idy*sw + idx] = pweight_mat[lindex][idy*sw + idx] + light;
		}
		p1 += w;
		p2 += w;
		p3 += w;
	}


#endif

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

	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < sh; y++)
		{
			for (x = 0; x < sw; x++)
			{
				if (pcount_mat[i][y*sw + x])
					pweight_mat[i][y*sw + x] = ((double)64.0*(double)pweight_mat[i][y*sw + x]) / (double)pcount_mat[i][y*sw + x];
				else
					pweight_mat[i][y*sw + x] = 0;

				if (pweight_mat[i][y*sw + x]>16383)
					pweight_mat[i][y*sw + x] = 16383;
			}
		}
	}


	unsigned long  left[9], right[9];
	unsigned long weight1;
	unsigned long weight2;
	unsigned long weight;

	
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{

		#if WDR_USE_THUMB_LUMA
			
			if ((x & MAX_BIT_V_MINUS1) == 0)
			{
				for (i = 0; i < 9; i++)
				{
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
			weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

//			*pixel_out++ = weight/16;


			// 0614-1604
			//if (testParams_5 == 1)
			//{
			//	weight = light;
			//}
			//else
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
			//fprintf(stderr,"weight  = %d \n ", weight );
			// Gain = weight/128
			*(pGainMat + y*w + x) = (RK_U32)weight; // for zlf-SpaceDenoise

		#else
			 
			if ((x & 127) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 7)*sw + (x >> 7)] * (128 - (y & 127)) + pweight_mat[i][(y >> 7)*sw + (x >> 7) + sw] * (y & 127)) / 128;
					right[i] = (pweight_mat[i][(y >> 7)*sw + (x >> 7) + 1] * (128 - (y & 127)) + pweight_mat[i][(y >> 7)*sw + (x >> 7) + sw + 1] * (y & 127)) / 128;
				}
			}
			/*
			if ((x & 255) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 8)*sw + (x >> 8)] * (256 - (y & 255)) + pweight_mat[i][(y >> 8)*sw + (x >> 8) + sw] * (y & 255)) / 256;
					right[i] = (pweight_mat[i][(y >> 8)*sw + (x >> 8) + 1] * (256 - (y & 255)) + pweight_mat[i][(y >> 8)*sw + (x >> 8) + sw + 1] * (y & 255)) / 256;
				}
			}
			
			if ((x & 7) == 0)
			{
				for (i = 0; i < 9; i++)
				{
					left[i] = (pweight_mat[i][(y >> 3)*sw + (x >> 3)] * (8 - (y & 7)) + pweight_mat[i][(y >> 3)*sw + (x >> 3) + sw] * (y & 7)) / 8;
					right[i] = (pweight_mat[i][(y >> 3)*sw + (x >> 3) + 1] * (8 - (y & 7)) + pweight_mat[i][(y >> 3)*sw + (x >> 3) + sw + 1] * (y & 7)) / 8;
				}
			}
			*/
			light = plight[y*w + x];
			if (light>16*1023)
			{
				light=16*1023;
			}

			lindex = light >> 11;
// 			unsigned long weight1 = (left[lindex] * (128 - (x & 127)) + right[lindex] * (x & 127)) / 128;
// 			unsigned long weight2 = (left[lindex + 1] * (128 - (x & 127)) + right[lindex + 1] * (x & 127)) / 128;
// 			unsigned long weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

			weight1 = (left[lindex] * (128 - (x & 127)) + right[lindex] * (x & 127)) / 128;
			weight2 = (left[lindex + 1] * (128 - (x & 127)) + right[lindex + 1] * (x & 127)) / 128;

			//weight1 = (left[lindex] * (256 - (x & 255)) + right[lindex] * (x & 255)) / 256;
			//weight2 = (left[lindex + 1] * (256 - (x & 255)) + right[lindex + 1] * (x & 255)) / 256;

			//weight1 = (left[lindex] * (8 - (x & 7)) + right[lindex] * (x & 7)) / 8;
			//weight2 = (left[lindex + 1] * (8 - (x & 7)) + right[lindex + 1] * (x & 7)) / 8;
			
			weight = (weight1*(2048 - (light & 2047)) + weight2*(light & 2047)) / 2048;

//			*pixel_out++ = weight/16;


			// 0614-1604
			if (testParams_5 == 1)
			{
				weight = light;
			}
			else
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
			//fprintf(stderr,"weight  = %d \n ", weight );
			// Gain = weight/128
			*(pGainMat + y*w + x) = (RK_U32)weight; // for zlf-SpaceDenoise


		#endif

			//*pixel_out++ = clip10bit(((unsigned long)(*pixel_in++) - 64 * 4)*weight / 512 + 64);
			if(*pixel_in>blacklevel*2)
				*pixel_out++ = clip10bit(((unsigned long)(*pixel_in++) - blacklevel * 2)*weight / 1024 + blacklevel/4);
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
