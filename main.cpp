//
/////////////////////////////////////////////////////////////////////////
// File: main.c
// Desc: Implementation of MFNR
//       Version 2.41 for RK3288+VIVO
// 
// Date: Revised by yousf 20160428-20160721
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "rk_precomp.h"              // Precompile

#define     USE_MultiSetsTest       0               // 1-UseMultiSetsTest 0-NotUseUSE_MultiSetsTest

#define     THUMBSIZE 		409600   // Thumb data Stride (Bytes, 4ByteAlign)
RK_U16      g_BaseThumbBuf[THUMBSIZE];                         // Thumb data pointers


#if USE_MultiSetsTest == 0
//////////////////////////////////////////////////////////////////////////
/*/
////-------- Test Info Setting
#define     RAW_WID	                4608            // Raw Src data width
#define     RAW_HGT	                3456            // Raw Src data height
#define     RAW_FILE_NUM            12              // number of Src Raw data files
////-------- Test Folder Setting
//#define     FILENAME_PREFIX         "D:/adb_tools/0604_04_v222so_OPPORaw_HappyCoast/01121054"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0605_02_v223so_OPPORaw_HappyCoast/01121054"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0606_05_v224so_OPPORaw_CoarstalCity/0112135"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0614_04_OPPORaw/0102338"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0616_03_OPPORaw+Jpg/0112128"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0620_02_OPPORaw32/011228"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0620_04_OPPORaw14/0112118"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/0705_01_OPPORaw/010138"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/20160707_02_RK3288OPPORaw/0103455"  // Raw data file name prefix
#define     FILENAME_PREFIX         "D:/adb_tools/20160714_01_OPPOJpg/010163"  // Raw data file name prefix
//*/

//
////-------- Test Info Setting
#define     RAW_WID	                4164            // Raw Src data width
#define     RAW_HGT	                3136            // Raw Src data height
#define     RAW_FILE_NUM            6              // number of Src Raw data files
////-------- Test Folder Setting
//#define     FILENAME_PREFIX         "D:/adb_tools/20160629_03_OPPODng/test01/test01"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/20160630_01_OPPODng/test11/test11"  // Raw data file name prefix
//#define     FILENAME_PREFIX         "D:/adb_tools/20160704_02_OPPODng/test23/test23"  // Raw data file name prefix
#define     FILENAME_PREFIX         "../../data/test317/test317"
//*/


#else // USE_MultiSetsTest == 1

    //#define     MULT_SETS_DIR           "D:\\adb_tools\\20160711_01_OPPODng\\"
    //#define     MULT_SETS_DIR           "I:\\MFNR_TestData\\"
	#define     MULT_SETS_DIR           "D:\\adb_tools\\MFNR_TestData\\"
    #define     MULT_SETS_NUM           35              // num of test-sets
    #define     RAW_WID	                4164            // Raw Src data width
    #define     RAW_HGT	                3136            // Raw Src data height
    #define     RAW_FILE_NUM            6               // number of Src Raw data files

#endif

////-------- Debug Global Pointer
#if XM4_DEBUG == 1 // 1/0 for CEVA_XM4 IDE Debug-View
    classMFNR* pGlobalPointer = NULL;       // for CEVA_XM4 IDE Debug-View
#endif


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: doRkEffect()
// Desc: MFNR Interface
//   In: mImgBuf        - RawSrc data pointers
//       mWidth         - RawSrc data width 
//       mHeight        - RawSrc data height
//       mPitch         - RawSrc data Stride
//       mImgSize       - RawSrc data Size
//       bayerType      - Bayer Type
//       redGain        - AWB Red Gain
//       blueGain       - AWB Blue Gain
//  Out: pOutData       - RawDst data pointer
// 
// Date: Revised by yousf 20160603
// 
/*************************************************************************/
int doRkEffect(void** mImgBuf, int mWidth, int mHeight, int mPitch, int mImgSize, 
    RK_U8* bayerType, RK_F32 redGain, RK_F32 blueGain, int lumIntensity,
	float sensorGain, float ispGain, float shutter,
	int luxIndex, int expIndex, RK_S16 blackLevel[4], 
    void* pOutData, /*float useGain*/RK_F32 testParams[])
{
    // 
    int ret = 0;
    //int nRawFileNum = RAW_IMG_BUF_COUNT;
	int nRawFileNum = testParams[8];
	if (nRawFileNum > RK_MAX_FILE_NUM)
	{
		printf("nRawFileNum > RK_MAX_FILE_NUM !");
		return -1;
	}
	
    //////////////////////////////////////////////////////////////////////////
    ////---- HW Scaler: ThumbBuf
    // 
#if MY_DEBUG_PRINTF == 1
    printf("Scaler Raw2Thumb:\n");
#endif
    int         ThumbWid        = mWidth  / SCALER_FACTOR_R2T;      // Thumb data width  (floor)
    int         ThumbHgt        = mHeight / SCALER_FACTOR_R2T;      // Thumb data height (floor)
    int         Thumb16Stride   = (ThumbWid * 16 + 31) / 32 * 4;    // Thumb data Stride (Bytes, 4ByteAlign)
    int         Thumb16DataSize = ThumbHgt * Thumb16Stride;         // Thumb data Size (Bytes)
    RK_U16*     mThumbBuf[RK_MAX_FILE_NUM];                         // Thumb data pointers
    for (int k=0; k < RK_MAX_FILE_NUM; k++)                         // Thumb data pointers
    {
        if (k < nRawFileNum)
        {
            mThumbBuf[k] = (RK_U16*)malloc(sizeof(RK_U16) * Thumb16DataSize/2);
            if(mThumbBuf[k] == NULL)
            {
                ret = -1;
                return ret;
            }
        }
        else
        {
            mThumbBuf[k] = NULL;
        }
    }
    classScaler*    pClassScaler = (classScaler*)malloc(sizeof(classScaler));
    char            fileName[1024];
    for (int k=0; k < nRawFileNum; k++)
    {
#if MY_DEBUG_PRINTF == 1
        printf("\tmThumbBuf[%d] = Scaler_R2T(ImgBuf[%d]);\n", k, k);
#endif

        pClassScaler->ScaleDown_R2T((RK_U16*)mImgBuf[k], mWidth, mHeight, ThumbWid, ThumbHgt, mThumbBuf[k]);
		if (k==0)
			// add by zxy for get the thumb luma map.
			memcpy(g_BaseThumbBuf, mThumbBuf[0], sizeof(RK_U16) * Thumb16DataSize/2);
		
#if MY_DEBUG_WRT2RAW == 1
        // Write
        sprintf(fileName, "data/pRawSrcs%d_Thumb.raw", k);
        ret = WriteRaw16Data(mThumbBuf[k], Thumb16Stride/2, ThumbHgt, fileName);
        if (ret)
        {
            printf("Failed to write rawfile: %s!\n", fileName);
        }//*/
#endif

        /*/ Read
        sprintf(fileName, "data/pRawSrcs%d_Thumb.raw", k);
        ret = ReadRaw16Data(fileName, Thumb16Stride/2, ThumbHgt, &mThumbBuf[k]);
        if (ret)
        {
            printf("Failed to read rawfile: %s!\n", fileName);
        }//*/
    }


    //////////////////////////////////////////////////////////////////////////
    ////---- MFNR
    //
#if MY_DEBUG_PRINTF == 1
    printf("RK_MFNR Processing ...\n");
#endif
    classMFNR*      pClassMFNR = (classMFNR*)malloc(sizeof(classMFNR));
#if XM4_DEBUG == 1 // 1/0 for CEVA_XM4 IDE Debug-View
    pGlobalPointer = pClassMFNR;    // for CEVA_XM4 IDE Debug-View
#endif
    ret = pClassMFNR->MFNR_Open(mWidth, mHeight, mPitch, mImgSize, nRawFileNum, 
		bayerType, redGain, blueGain, lumIntensity,
		sensorGain, ispGain, shutter, 
		luxIndex, expIndex, blackLevel,
		mImgBuf, mThumbBuf, 
		testParams);
    if (ret)
    {
        return ret;
    }
    ret = pClassMFNR->MFNR_Execute(pOutData);
    if (ret)
    {
        return ret;
    }
    ret = pClassMFNR->MFNR_Close();
    if (ret)
    {
        return ret;
    }


    //////////////////////////////////////////////////////////////////////////
    ////---- HW Free
    for (int k=0; k < RK_MAX_FILE_NUM; k++)         // Thumb Srcs data pointers
    {
        if(mThumbBuf[k] != NULL)
        {
            free(mThumbBuf[k]);
            mThumbBuf[k] = NULL;
        }
    }
    ////---- MFNR Free
    if (pClassMFNR != NULL)
    {
        free(pClassMFNR);
        pClassMFNR = NULL;
    }

    //
    return  ret;

} // doRkEffect()


//////////////////////////////////////////////////////////////////////////
// Defect Pixel by ln_20160525
#if USE_DEFECT_PIXEL == 1 // 1-Use Defect Pixel, 0-Not Use
//
typedef enum
{
    BAYER_RGGB,
    BAYER_BGGR,
    BAYER_GRBG,
    BAYER_GBRG
}BAYER_FORMAT;
//
#define	MAX_IMAGE_WIDTH		8192
#define RAW_BITS_DEEPT		14
//
RK_U16 defect_ram[MAX_IMAGE_WIDTH][4];
//
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
//
void defect_pixel_process(RK_U16 *pixel_in, RK_U16 *pixel_out, RK_U16 image_width, RK_U16 image_height, BAYER_FORMAT format)
{
    // 
    int i, j;

    //
    RK_U16  buf[5][5] = {0};
    RK_U16	rd_pixel;
    RK_U16  wr_pixel;
    bool	GH, GV;

    //
    switch (format)
    {
    case BAYER_RGGB:
    case BAYER_BGGR:
        GH = 0;
        break;

    case BAYER_GRBG:
    case BAYER_GBRG:
        GH = 1;
        break;
    }

    //
    for (j = 0; j < image_height+2; j++)
    {
        GV = GH;
        for (i = 0; i < image_width+2; i++)
        {
            //
            if ((j < image_height) && (i < image_width))
                rd_pixel = *pixel_in++;

            //
            if (j < 2)
            {
                if (i<image_width)
                    defect_ram[i][j] = rd_pixel;
            }
            else if (j>image_height - 1)
            {
                if (i < image_width)
                {
                    wr_pixel = defect_ram[i][(j - 2) & 3];
                    *pixel_out++ = wr_pixel;
                }
            }
            else
            {
                if (i < image_width)
                {
                    buf[0][4] = defect_ram[i][(j - 2) & 3];
                    buf[1][4] = defect_ram[i][(j - 1) & 3];
                    buf[2][4] = defect_ram[i][j & 3];
                    buf[3][4] = defect_ram[i][(j + 1) & 3];
                    buf[4][4] = rd_pixel;
                    defect_ram[i][(j + 2) & 3] = rd_pixel;
                }
                if (i >= 2)
                {
                    if (j < 4)
                    {
                        wr_pixel = buf[2][2];
                    }
                    else
                    {
                        RK_U16 minv0, minv1, minv2, minv3, minv4, minv5, minv;
                        RK_U16 maxv0, maxv1, maxv2, maxv3, maxv4, maxv5, maxv;
                        RK_U16 dis;

                        if (GV)
                        {
                            minv0 = min(buf[0][2], buf[4][2]);
                            minv1 = min(buf[1][1], buf[1][3]);
                            minv2 = min(buf[2][0], buf[2][4]);
                            minv3 = min(buf[3][1], buf[3][3]);
                            minv4 = min(minv0, minv1);
                            minv5 = min(minv2, minv3);
                            minv = min(minv4, minv5);

                            maxv0 = max(buf[0][2], buf[4][2]);
                            maxv1 = max(buf[1][1], buf[1][3]);
                            maxv2 = max(buf[2][0], buf[2][4]);
                            maxv3 = max(buf[3][1], buf[3][3]);
                            maxv4 = max(maxv0, maxv1);
                            maxv5 = max(maxv2, maxv3);
                            maxv = max(maxv4, maxv5);
                        }
                        else
                        {
                            minv0 = min(buf[0][0], buf[0][4]);
                            minv1 = min(buf[2][0], buf[2][4]);
                            minv2 = min(buf[4][0], buf[4][4]);
                            minv3 = min(buf[0][2], buf[4][2]);
                            minv4 = min(minv0, minv1);
                            minv5 = min(minv2, minv3);
                            minv = min(minv4, minv5);

                            maxv0 = max(buf[0][0], buf[0][4]);
                            maxv1 = max(buf[2][0], buf[2][4]);
                            maxv2 = max(buf[4][0], buf[4][4]);
                            maxv3 = max(buf[0][2], buf[4][2]);
                            maxv4 = max(maxv0, maxv1);
                            maxv5 = max(maxv2, maxv3);
                            maxv = max(maxv4, maxv5);
                        }
                        dis = maxv - minv;
                        wr_pixel = buf[2][2];
                        if (buf[2][2] > maxv)
                        {
                            if (buf[2][2] - minv > dis * 2)
                                wr_pixel = maxv;
                        }
                        else if (buf[2][2] < minv)
                        {
                            if (maxv - buf[2][2] > dis * 2)
                                wr_pixel = minv;
                        }
                    }
                    *pixel_out++ = wr_pixel;
                }
                buf[0][0] = buf[0][1];
                buf[0][1] = buf[0][2];
                buf[0][2] = buf[0][3];
                buf[0][3] = buf[0][4];

                buf[1][0] = buf[1][1];
                buf[1][1] = buf[1][2];
                buf[1][2] = buf[1][3];
                buf[1][3] = buf[1][4];

                buf[2][0] = buf[2][1];
                buf[2][1] = buf[2][2];
                buf[2][2] = buf[2][3];
                buf[2][3] = buf[2][4];

                buf[3][0] = buf[3][1];
                buf[3][1] = buf[3][2];
                buf[3][2] = buf[3][3];
                buf[3][3] = buf[3][4];

                buf[4][0] = buf[4][1];
                buf[4][1] = buf[4][2];
                buf[4][2] = buf[4][3];
                buf[4][3] = buf[4][4];
            }
            GV = !GV;
        } // for i

        GH = !GH;
    } // for j

} // defect_pixel_process()

#endif


//////////////////////////////////////////////////////////////////////////
// Multiple Sets Test by yousf_20160718
#if USE_MultiSetsTest == 1
/************************************************************************/
// Func: MultiSetsTest()
// Desc: Multiple Sets Test
//   In: mImgBuf        - RawSrc data pointers
//       mWidth         - RawSrc data width 
//       mHeight        - RawSrc data height
//       mPitch         - RawSrc data Stride
//       mImgSize       - RawSrc data Size
//       bayerType      - Bayer Type
//       redGain        - AWB Red Gain
//       blueGain       - AWB Blue Gain
//  Out: pOutData       - RawDst data pointer
// 
// Date: Revised by yousf 20160717
// 
/*************************************************************************/
int MultiSetsTest()
{
    //
    int     ret = 0; // return value

    RK_F32 ispGainArray[1024] = {4.0, 2.0, 8.0, 3.0, 4.0, 8.0, 2.0, 2.0,
                                 2.0, 8.0, 8.0, 8.0, 8.0, 8.0, 8.0,
                                 8.0, 8.0, 8.0, 8.0, 8.0, 8.0,
                                 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0};
    // 
    int         RawWid     = RAW_WID;                                           // Raw data width
    int         RawHgt     = RAW_HGT;                                           // Raw data height
    int         RawStride  = RawWid * 2;                                        // Raw data stride (16bit)
    int         RawSize    = RawStride * RawHgt;                                // Raw data size (Bytes)
    int         RawFileNum = RAW_FILE_NUM;                                      // Raw file num
    RK_U16*     pRawSrcs[RK_MAX_FILE_NUM];                                      // Raw Srcs data pointers
    RK_U16*     pRawDst = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);   // Raw Dst data pointer
    if (RAW_FILE_NUM > RK_MAX_FILE_NUM)
    {
        printf("RAW_FILE_NUM > RK_MAX_FILE_NUM !");
        return -1;
    }


    //// 
    char        cmdstr[1024];
    char        folderList[256]  = "FolderList.txt";
    char        rawFileList[256] = "RawFileList.txt";
    FILE*       fp0 = NULL;                     // raw folder list file-pointer
    FILE*       fp1 = NULL;                     // raw folder file-pointer
    char        folderName[1024];               // raw folder filename
    char        fileNames[RK_MAX_FILE_NUM][256];// raw data filename
    // Get raw folder list
    sprintf(cmdstr, "dir /b %s > %s", MULT_SETS_DIR, folderList);
    ret = system(cmdstr); // 
    
    fp0 = fopen(folderList, "r");
    if (folderList == NULL)
    {
        ret = -1;
        return ret;
    }
    for (int i=0; i < MULT_SETS_NUM; i++)
    {
        //---- Test Set#i
        fscanf(fp0, "%s", folderName);

        // Get raw file list
        sprintf(cmdstr, "dir /b /s %s%s\\*_*_16.raw > %s", MULT_SETS_DIR, folderName, rawFileList);
        ret = system(cmdstr); // 
        fp1 = fopen(rawFileList, "r");
        if (fp1 == NULL)
        {
            ret = -1;
            return ret;
        }
        else
        {
            //////////////////////////////////////////////////////////////////////////
            ////---- Read RawSrcs
            printf("Read RawSrcs:\n");
            for (int k=0; k < RAW_FILE_NUM; k++)
            {
                // raw data filename
                fscanf(fp1, "%s", fileNames[k]);

                // read raw data
                printf("\t%s\n", fileNames[k]);
                ret = ReadRaw16Data(fileNames[k], RawWid, RawHgt, &pRawSrcs[k]);
                if (ret)
                {
                    printf("Failed to read rawfile: %s!\n", fileNames[k]);
                    return ret;
                }

            }


            //////////////////////////////////////////////////////////////////////////
            ////---- Defect Pixel
#if USE_DEFECT_PIXEL == 1 // 1-Use Defect Pixel, 0-Not Use
            RK_U16*         pixel_in     = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);
            RK_U16*         pixel_out    = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);
            RK_U16          image_width  = RawWid;
            RK_U16          image_height = RawHgt;
            BAYER_FORMAT    format       = BAYER_RGGB;
            for (int k=0; k < RAW_FILE_NUM; k++)
            {
                // Method-1
                memcpy(pixel_in, pRawSrcs[k], sizeof(RK_U16) * RawWid * RawHgt);
                memset(pixel_out, 0, sizeof(RK_U16) * RawWid * RawHgt);
                defect_pixel_process(pixel_in, pixel_out, image_width, image_height, format);
                memcpy(pRawSrcs[k], pixel_out, sizeof(RK_U16) * RawWid * RawHgt);
            }
#endif


            //////////////////////////////////////////////////////////////////////////
            // 
            printf("PC-Android Interface Converting:\n");
            // Android Interface
            void*   mImgBuf[RAW_FILE_NUM];
            for (int k=0; k < RAW_FILE_NUM; k++)
            {
                printf("\tmImgBuf[%d] = (void*)pRawSrcs[%d];\n", k, k);
                mImgBuf[k] = (void*)pRawSrcs[k]; // u16bit -> void
            }
            void*   pOutData = (void*)pRawDst;   // u16bit -> void

            // 
            printf("\n---------------- doRkEffect ----------------\n");
            // RK_MFNR Processing
            RK_U8   bayerType[8]  = "RGGB";  // Bayer Type
            RK_F32  redGain       = 1.4762;  // AWB Red Gain
            RK_F32  blueGain      = 2.3180;  // AWB Blue Gain
            int		lumIntensity = 1;		// 
            RK_F32	sensorGain   = 8.0;		// 
            RK_F32	ispGain		 = 5.7170;//9.6883;	// 
            RK_F32	shutter		 = 7465;	// 
            int		luxIndex	 = 1;	// 
            int		expIndex	 = 1; // 
            RK_S16	blackLevel[4]; // 
            blackLevel[0] = 261; // 
            blackLevel[1] = 262; // 
            blackLevel[2] = 259; // 
            blackLevel[3] = 261; // 

            RK_F32 testParams[TEST_PARAMS_NUM] = {0}; // 
            // testParams[0]	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
            // testParams[1]	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
            // testParams[2]     n-Gain x n, n=1,2,... (n>=1, float)
            // testParams[3]     0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
            // testParams[4]     0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
            // testParams[5]     0-OldWdrParamPol, 1-NewWdrParamMax
            // testParams[6]     0-NotUseRegister, 1-UseRegister
            // testParams[7]     0-NotUseOppoBlk, 1-UseOppoBlk
            // testParams[8]     n-nFrameCompose, n = 1,2,3,...
            // testParams[9]     1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
            // testParams[10]    0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise

            //// RawParams...
            char	strRawParams[1024];
            sprintf(strRawParams, "Params=rGain=%.1f_bGain=%.1f_lumIntensity=%d_sensorGain=%.1f_ispGain=%.1f_shutter=%.1f_luxIndex=%d_expIndex=%d_blkLevel4=%d,%d,%d,%d.txt",
                redGain, blueGain, lumIntensity, sensorGain, ispGain, shutter,
                luxIndex, expIndex, blackLevel[0], blackLevel[1], blackLevel[2], blackLevel[3]);
            FILE *fp = fopen(strRawParams, "w");
            if (fp != NULL) fclose(fp);

            //// String...
            int		numTestParams ;			// num of mTestParams
            char	strVersion[1024];			// str of code version
            char	strTestParams[1024];	// str of mTestParams
            numTestParams = 10;
            sprintf(strVersion, "v241");


            //////////////////////////////////////////////////////////////////////////
            /*/ TestParamsSetting - NotUseRegister
            ispGain = 12.9632;
            testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
            testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
            testParams[2] = ispGain;	//   n-Gain x n, n=1,2,... (n>=1, float)
            testParams[3] = 0;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
            testParams[4] = 0;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
            testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
            testParams[6] = 0;			//	 0-NotUseRegister, 1-UseRegister
            testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
            testParams[8] = 12;			//   n-nFrameCompose, n = 1,2,3,...
            testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
            testParams[10]= 0;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
            //*/


            /*/ TestParamsSetting - Linear
            ispGain = 6;
            testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
            testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
            testParams[2] = ispGain;	//   n-Gain x n, n=1,2,... (n>=1, float)
            testParams[3] = 0;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
            testParams[4] = 0;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
            testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
            testParams[6] = 1;			//	 0-NotUseRegister, 1-UseRegister
            testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
            testParams[8] = 15;			//   n-nFrameCompose, n = 1,2,3,...
            testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
            testParams[10]= 0;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
            //*/

            /*/ TestParamsSetting - WDR
            ispGain = 3;
            testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)
            testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
            testParams[2] = 8;			//   n-Gain x n, n=1,2,... (n>=1, float) 
            testParams[3] = 3;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
            testParams[4] = 7;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
            testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
            testParams[6] = 1;			//	 0-NotUseRegister, 1-UseRegister
            testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
            testParams[8] = 6;			//   n-nFrameCompose, n = 1,2,3,...
            testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
            testParams[10]= 1;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
            //*/

            //
            ispGain = ispGainArray[i];
            testParams[0] = 0;				//	  0-Not use MAX(64,refValue)  , 1-Use MAX(64,refValue)  
            testParams[1] = 1;				//	  1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
            testParams[2] = 8;				//    n-Gain x n, n=1,2,... (n>=1, float) 
            testParams[3] = 3;				//    0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
            testParams[4] = 7;				//    0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
            testParams[5] = 0;				//    0-OldWdrParamPol, 1-NewWdrParamMax
            testParams[6] = 1;				// 	  0-NotUseRegister, 1-UseRegister
            testParams[7] = 0;				//	  0-NotUseOppoBlk, 1-UseOppoBlk`
            testParams[8] = 6;				//    n-nFrameCompose, n = 1,2,3,...
            testParams[9] = 1;				//    1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
            testParams[10]= 2;				//    0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
            //*/

            sprintf(strTestParams, "rkTest");
            for (int m=0; m<=numTestParams; m++)
            {
                if (m == 2)
                {
                    sprintf(strTestParams, "%s%x", strTestParams, (int)ispGain);
                }
                else
                {
                    sprintf(strTestParams, "%s%.0f", strTestParams, testParams[m]);
                }
                
                
            }
            //
            doRkEffect(mImgBuf, RawWid, RawHgt, RawStride, RawSize, 
                bayerType, redGain, blueGain, lumIntensity,
                sensorGain, ispGain, shutter, 
                luxIndex, expIndex, blackLevel,
                pOutData, testParams);
            
            //
            char frkPrefix[1024];
#ifdef _DEBUG
            sprintf(frkPrefix, "%s%s\\%s-%d_%d_%s%s_16-debug.raw", MULT_SETS_DIR, folderName, folderName, RawWid, RawHgt, strVersion, strTestParams);
#else
            sprintf(frkPrefix, "%s%s\\%s-%d_%d_%s%s_16-release.raw", MULT_SETS_DIR, folderName, folderName, RawWid, RawHgt, strVersion, strTestParams);
#endif
            printf("Write Result: %s ...\n", frkPrefix);
            WriteRaw16Data((RK_U16*)pOutData, RawWid, RawHgt, frkPrefix);
            
            printf("\n--------------------------------------------\n");

            //////////////////////////////////////////////////////////////////////////
            /*/ 
            char rawfilename[1024]; // raw data filename
            sprintf(rawfilename, "%s%s-%d_%d_rk_16.raw", MULT_SETS_DIR, folderName, RawWid, RawHgt);
            printf("Write Result: %s ...\n", rawfilename);
            WriteRaw16Data((RK_U16*)pOutData, RawWid, RawHgt, rawfilename);
            //*/


            ////---- Memory Free
            for (int k=0; k < RAW_FILE_NUM; k++)   
            {
                if (pRawSrcs[k] != NULL)
                {
                    free(pRawSrcs[k]);
                    pRawSrcs[k] = NULL;
                }
            }
//             if (pRawDst != NULL)
//             {
//                 free(pRawDst);
//                 pRawDst = NULL;
//             }

        }

    } // for i
    
    //
    return ret;

} // MultiSetsTest()

#endif


//////////////////////////////////////////////////////////////////////////
////---- Main
//
int main(void)//(int argc, char* argv[])
{
#if WDR_USE_CEVA_VECC
	interpolationYaxis();
#endif
    //
    int     ret = 0; // return value
    char    fileNames[RK_MAX_FILE_NUM][256];    // file name


    //////////////////////////////////////////////////////////////////////////
    ////-------- Abstract
    // 
    printf("\n\n");
    printf("================================================================\n");
    printf("  Rockchip Raw-domain Low Light Noise Reduction Algorithm Demo\n\n");
    printf("\tVersion:   2.41\n");
    printf("\tAuthor:    Rockchip Algo-Dep (ysf)\n");
    printf("\tDate:      2016.07.21\n");
    printf("----------------------------------------------------------------\n");


    //////////////////////////////////////////////////////////////////////////
    // Multiple Sets Test by yousf_20160718
#if USE_MultiSetsTest == 1

    MultiSetsTest();

    return ret;
#else
   

    //////////////////////////////////////////////////////////////////////////
    ////-------- Input & Output
    //
    printf("Read RawSrcs:\n");
    // 
    int         RawWid     = RAW_WID;                                           // Raw data width
    int         RawHgt     = RAW_HGT;                                           // Raw data height
    int         RawStride  = RawWid * 2;                                        // Raw data stride (16bit)
    int         RawSize    = RawStride * RawHgt;                                // Raw data size (Bytes)
    int         RawFileNum = RAW_FILE_NUM;                                      // Raw file num
    RK_U16*     pRawSrcs[RK_MAX_FILE_NUM];                                      // Raw Srcs data pointers
    RK_U16*     pRawDst = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);   // Raw Dst data pointer
	
    //---- Read RawSrcs
	if (RAW_FILE_NUM > RK_MAX_FILE_NUM)
	{
		printf("RAW_FILE_NUM > RK_MAX_FILE_NUM !");
		return -1;
	}
    for (int k=0; k < RAW_FILE_NUM; k++)
    {
        // file name
        sprintf(fileNames[k], "%s-%d_%d_%d_16.raw", FILENAME_PREFIX, RawWid, RawHgt, k);
        // read raw data
        printf("\t%s\n", fileNames[k]);
        ret = ReadRaw16Data(fileNames[k], RawWid, RawHgt, &pRawSrcs[k]);
        if (ret)
        {
            printf("Failed to read rawfile: %s!\n", fileNames[k]);
            return ret;
        }

#if MY_DEBUG_WRT2RAW == 1
        // Write
        sprintf(fileNames[k], "data/pRawSrcs%d_VS.raw", k);
        ret = WriteRaw16Data(pRawSrcs[k], RawWid, RawHgt, fileNames[k]);
        if (ret)
        {
            printf("Failed to write rawfile: %s!\n", fileNames[k]);
        }//*/
#endif

    }

    //////////////////////////////////////////////////////////////////////////
    ////---- Defect Pixel
#if USE_DEFECT_PIXEL == 1 // 1-Use Defect Pixel, 0-Not Use
    RK_U16*         pixel_in     = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);
    RK_U16*         pixel_out    = (RK_U16 *)malloc(sizeof(RK_U16) * RawWid * RawHgt);
    RK_U16          image_width  = RawWid;
    RK_U16          image_height = RawHgt;
    BAYER_FORMAT    format       = BAYER_RGGB;
    for (int k=0; k < RAW_FILE_NUM; k++)
    {
        // Method-1
        memcpy(pixel_in, pRawSrcs[k], sizeof(RK_U16) * RawWid * RawHgt);
        memset(pixel_out, 0, sizeof(RK_U16) * RawWid * RawHgt);
        defect_pixel_process(pixel_in, pixel_out, image_width, image_height, format);
        memcpy(pRawSrcs[k], pixel_out, sizeof(RK_U16) * RawWid * RawHgt);
    }
#endif


    //////////////////////////////////////////////////////////////////////////
    // 
    printf("PC-Android Interface Converting:\n");
    // Android Interface
    void*   mImgBuf[RAW_FILE_NUM];
    for (int k=0; k < RAW_FILE_NUM; k++)
    {
        printf("\tmImgBuf[%d] = (void*)pRawSrcs[%d];\n", k, k);
        mImgBuf[k] = (void*)pRawSrcs[k]; // u16bit -> void
    }
    void*   pOutData = (void*)pRawDst;   // u16bit -> void

    // 
    printf("\n---------------- doRkEffect ----------------\n");
    // RK_MFNR Processing
    RK_U8  bayerType[8]  = "RGGB";  // Bayer Type
    RK_F32 redGain       = 1.4762;  // AWB Red Gain
    RK_F32 blueGain      = 2.3180;  // AWB Blue Gain
	int		lumIntensity = 1;		// 
	RK_F32	sensorGain   = 8.0;		// 
	RK_F32	ispGain		 = 5.7170;//9.6883;	// 
	RK_F32	shutter		 = 7465;	// 
	int		luxIndex	 = 1;	// 
	int		expIndex	 = 1; // 
	RK_S16	blackLevel[4]; // 
	blackLevel[0] = 261; // 
	blackLevel[1] = 262; // 
	blackLevel[2] = 259; // 
	blackLevel[3] = 261; // 

	RK_F32 testParams[TEST_PARAMS_NUM] = {0}; // 
	// testParams[0]	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
	// testParams[1]	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	// testParams[2]     n-Gain x n, n=1,2,... (n>=1, float)
	// testParams[3]     0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	// testParams[4]     0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	// testParams[5]     0-OldWdrParamPol, 1-NewWdrParamMax
	// testParams[6]     0-NotUseRegister, 1-UseRegister
	// testParams[7]     0-NotUseOppoBlk, 1-UseOppoBlk
	// testParams[8]     n-nFrameCompose, n = 1,2,3,...
	// testParams[9]     1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	// testParams[10]    0-NotUseSpatialDenoise, 1-NotUseSpatialDenoise  

	//// RawParams...
	char	strRawParams[1024];
	sprintf(strRawParams, "Params=rGain=%.1f_bGain=%.1f_lumIntensity=%d_sensorGain=%.1f_ispGain=%.1f_shutter=%.1f_luxIndex=%d_expIndex=%d_blkLevel4=%d,%d,%d,%d.txt",
		redGain, blueGain, lumIntensity, sensorGain, ispGain, shutter,
		luxIndex, expIndex, blackLevel[0], blackLevel[1], blackLevel[2], blackLevel[3]);
	FILE *fp = fopen(strRawParams, "w");
	if (fp != NULL) fclose(fp);

	//// String...
	int		numTestParams ;			// num of mTestParams
	char	strVersion[1024];			// str of code version
	char	strTestParams[1024];	// str of mTestParams
	numTestParams = 10;
	sprintf(strVersion, "v241");


	//////////////////////////////////////////////////////////////////////////
	// TestParamsSetting - NotUseRegister
	ispGain = 12.9632;
	testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
	testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	testParams[2] = ispGain;	//   n-Gain x n, n=1,2,... (n>=1, float)
	testParams[3] = 0;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	testParams[4] = 0;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
	testParams[6] = 0;			//	 0-NotUseRegister, 1-UseRegister
	testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
	testParams[8] = 12;			//   n-nFrameCompose, n = 1,2,3,...
	testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	testParams[10]= 0;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
	//*/


	/*/ TestParamsSetting - Linear
	ispGain = 8;
	testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
	testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	testParams[2] = ispGain;	//   n-Gain x n, n=1,2,... (n>=1, float)
	testParams[3] = 0;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	testParams[4] = 7;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
	testParams[6] = 1;			//	 0-NotUseRegister, 1-UseRegister
	testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
	testParams[8] = 6;			//   n-nFrameCompose, n = 1,2,3,...
	testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	testParams[10]= 0;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
	//*/

	// TestParamsSetting - WDR
	ispGain = 3;
	testParams[0] = 0;			//	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)
	testParams[1] = 1;			//	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	testParams[2] = 8;			//   n-Gain x n, n=1,2,... (n>=1, float) 
	testParams[3] = 3;			//   0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	testParams[4] = 7;			//	 0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	testParams[5] = 0;			//	 0-OldWdrParamPol, 1-NewWdrParamMax
	testParams[6] = 1;			//	 0-NotUseRegister, 1-UseRegister
	testParams[7] = 0;			//	 0-NotUseOppoBlk, 1-UseOppoBlk
	testParams[8] = 6;			//   n-nFrameCompose, n = 1,2,3,...
	testParams[9] = 1;			//   1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	testParams[10]= 1;			//   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
	//*/

	/*/
	ispGain = 8;
	testParams[0] = 0;				//	  0-Not use MAX(64,refValue)  , 1-Use MAX(64,refValue)  
	testParams[1] = 1;				//	  1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	testParams[2] = 8;				//    n-Gain x n, n=1,2,... (n>=1, float) 
	testParams[3] = 3;				//    0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	testParams[4] = 7;				//    0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	testParams[5] = 0;				//    0-OldWdrParamPol, 1-NewWdrParamMax
	testParams[6] = 1;				// 	  0-NotUseRegister, 1-UseRegister
	testParams[7] = 0;				//	  0-NotUseOppoBlk, 1-UseOppoBlk`
	testParams[8] = 6;				//    n-nFrameCompose, n = 1,2,3,...
	testParams[9] = 1;				//    1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	testParams[10]= 0;				//    0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise
	//*/

	sprintf(strTestParams, "rkTest");
	for (int m=0; m<=numTestParams; m++)
	{
		sprintf(strTestParams, "%s%.0f", strTestParams, testParams[m]);
	}
	//
	doRkEffect(mImgBuf, RawWid, RawHgt, RawStride, RawSize, 
		bayerType, redGain, blueGain, lumIntensity,
		sensorGain, ispGain, shutter, 
		luxIndex, expIndex, blackLevel,
		pOutData, testParams);
	{
		char frkPrefix[1024];
		sprintf(frkPrefix, "%s-%s%s-%d_%d_rkHBlkWgt_16.raw", FILENAME_PREFIX, strTestParams, strVersion, RawWid, RawHgt);
	}
    printf("\n--------------------------------------------\n");

    //////////////////////////////////////////////////////////////////////////
#if MY_DEBUG_WRT_DST == 1
    
    // init vars
    char rawfilename[1024]; // raw data filename
#if MF_COMPOSE_METHOD == ONLY_USE_H
    //sprintf_s(rawfilename, "data/pRawDst_H.raw");
    sprintf(rawfilename, "%s-%d_%d_rkH_16.raw", FILENAME_PREFIX, RawWid, RawHgt);
#elif MF_COMPOSE_METHOD == USE_H_BM_V1
    //sprintf_s(rawfilename, "data/pRawDst_HWgt.raw");
    sprintf(rawfilename, "%s-%d_%d_rkHWgt_16.raw", FILENAME_PREFIX, RawWid, RawHgt);
#elif MF_COMPOSE_METHOD == USE_H_BM_V2
    //sprintf_s(rawfilename, "data/pRawDst_HBlkWgt.raw");
    sprintf(rawfilename, "%s-%d_%d_rkHBlkWgt_16.raw", FILENAME_PREFIX, RawWid, RawHgt);
	//sprintf(rawfilename, "%s-%d_%d_rkHBlkWgt_16-G%.0f_N%.0f.raw", FILENAME_PREFIX, RawWid, RawHgt, ispGain, testParams[10]);
	
#else
#endif // #if MF_COMPOSE_METHOD == ?
    // 

    sprintf(rawfilename, "%s-%d_%d_%s%s_16.raw", FILENAME_PREFIX, RawWid, RawHgt, strVersion, strTestParams);
    printf("Write Result: %s ...\n", rawfilename);
    WriteRaw16Data((RK_U16*)pOutData, RawWid, RawHgt, rawfilename);
#endif


    //////////////////////////////////////////////////////////////////////////
    ////---- Memory Free
    for (int k=0; k < RAW_FILE_NUM; k++)   
    {
        if (pRawSrcs[k] != NULL)
        {
            free(pRawSrcs[k]);
            pRawSrcs[k] = NULL;
        }
    }
    if (pRawDst != NULL)
    {
        free(pRawDst);
        pRawDst = NULL;
    }
    

/*/
    //////////////////////////////////////////////////////////////////////////
    // DDR Memory Free
    ret = pClassMem->DDRFree();
    if (ret)
    {
        printf("Failed to DDRFree() !\n");
        return ret;
    }
//*/

    //
    printf("================================================================\n\n");

    //
    return ret;

#endif

} // main()



//////////////////////////////////////////////////////////////////////////



