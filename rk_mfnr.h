//
//////////////////////////////////////////////////////////////////////////
// File: rk_mfnr.h
// Desc: MFNR: Multi-Frame Noise Reduction
// 
// Date: Revised by yousf 20160721
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_MFNR_H
#define _RK_MFNR_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"              // Type definition
#include "rk_global.h"               // Global definition
#include "rk_memory.h"               // Memory operation

#include "rk_wdr.h"

//////////////////////////////////////////////////////////////////////////
////-------- Macro Switch Setting
// 
// Compute Grad Switch Setting
#define     USE_MAX_GRAD            1               // 1-use max grad truncation, 0-not use
#define     FEATURE_TH_METHOD       1//1               // 0-USE_MIN2_TH, 1-USE_AVE_TH
// Homography Computation Switch Setting
#define     USE_MV_HIST_FILTRATE    0//1               // 1-use MV Hist Filtrate, 0-not use
#define     USE_EARLY_STOP_H		0//1               // 1-use Early Stop Compute Homography, 0-not use
// Multi-Frame Composition Method Switch Setting
#define     MF_COMPOSE_METHOD       1//2               // 0-only use Homography, 1-USE_H_BM_V1, 2-USE_H_BM_V2
// Block Luma Matching Switch Setting
#define		USE_PIXEL_NOISE_STD_TH	1				// 0-use Region Noise Std Threshold (AVE), 1-use BayerPixel Noise Std Threshold
#define     SEARCH_BRANCH_METHOD    0               // 0-OLD_SEARCH_BRANCH, 1-MODIFY_SEARCH_BRANCH
#define     USE_MOTION_DETECT       1               // 1-use Motion Detect, 0-not use
// Auto White Balance Switch Setting
#define     USE_AWB                 0//1               // 1-use Auto White Balance, 0-not use
// DetailEnhancer Switch Setting
#define     USE_DETAIL_ENHANCER     0//1               // 1-use DetailEnhancer, 0-not use
// LowLightEnhancer Switch Setting
#define     USE_LOW_LIGHT_ENHANCER  0//0               // 1-use LowLightEnhancer, 0-not use


//////////////////////////////////////////////////////////////////////////
////-------- Macro Definition
//
//---- Scaler Params Setting
#define     SCALER_FACTOR_R2R       2               // Raw to Raw
#define     SCALER_FACTOR_R2L       2               // Raw to Luma
#define     SCALER_FACTOR_R2T       8               // Raw to Thumbnail

//---- Compute Grad Params Setting
//#define     USE_MAX_GRAD            0               // 1-use max grad truncation, 0-not use
#if USE_MAX_GRAD == 1
    #define MAX_GRAD                0xFFF           // max grad 0xFFF= 2^12-1
#endif
#define     USE_MIN2_TH             0               // Method-0: average
#define     USE_AVE_TH              1               // Method-1: first min + second min
//#define     FEATURE_TH_METHOD       USE_AVE_TH      // Feature Threshold Method

//---- Coarse Matching Params Setting
#define     DIV_FIXED_WIN_SIZE      32              // Win size
#define     NUM_LINE_DDR2DSP_THUMB  32              // num of line: read thumb to DSP
#define     MAX_SHARP_RATIO         0.0//0.96            // sharp < 0.96*MaxSharp -> InvalidRef
#define     MARK_BASE_FRAME         0               // base frame mark: 0
#define     MARK_VALID_REF          1               // valid ref frame mark: 1
#define     MARK_INVALID_REF        2               // invalid ref frame mark: 2
#define     COARSE_MATCH_WIN_SIZE   16              // Coarse Matching Win size in Thumb
#define     MAX_OFFSET              20//100             // max offset of each 2 frames
#define     COARSE_MATCH_RADIUS    (CEIL(MAX_OFFSET * 1.0 / SCALER_FACTOR_R2T)) // Coarse Matching Radius

//---- Divide Image
#define     NUM_DIVIDE_IMAGE        4               // Divide Image into 4x4 Region

//---- Fine Matching Params Setting
#define     FINE_MATCH_WIN_SIZE     64              // Fine Matching Win size in Raw
#define     FINE_LUMA_RADIUS        5               // search radius in Luma: 8=SCALER_FACTOR_RAW2THUMB, 2=SCALER_FACTOR_RAW2LUMA, Radius=(8/2+1)

//---- Homography Computation Params Setting
//#define     USE_MV_HIST_FILTRATE    1               // 1-use MV Hist Filtrate, 0-not use
#define		MAX_NUM_MATCH_FEATURE   512             // Max Num of Match Feature <-- 32x16 Segments at most in Thumb (Raw:8192x4096)    
#if USE_MV_HIST_FILTRATE == 1
    #define HALF_LEN_MV_HIST        (COARSE_MATCH_RADIUS * SCALER_FACTOR_R2T / SCALER_FACTOR_R2L + FINE_LUMA_RADIUS) // Half Length of MV Hist: 13*8/2+5=57
    #define LEN_MV_HIST             (HALF_LEN_MV_HIST*2+1) // Length of MV Hist: 57*2+1=115
    #define VALID_FEATURE_RATIO     0.1//0.01            // Valid Feature Ratio
#endif
#define     NUM_R4IT_CHOICE         1810            // nchoosek(16,4) - 10(in line)
#define     MARK_EXIST_AGENT        1               // mark of ExistAgent in 4x4 Region
#define     NUM_HOMOGRAPHY          200//100             // Number of Homography Computed
#define     ERR_TH_VALID_H          5               // Error Threshold of Valid Homography
//#define     USE_EARLY_STOP_H		1               // 1-use Early Stop Compute Homography, 0-not use
#if USE_EARLY_STOP_H == 1
	#define	ERR_TH_GOOD_H           0               // Error Threshold of Good Homography
	#define GOOD_CNT_H_RATIO		0.8				// Good Count Homography Ratio
#endif
#define     CRRCNT_TH_VALID_H       4               // Correct Count Threshold of Valid Homography
#define     MAX_PROJECT_ERROR       0x3F            // Max Project error: (64=6bit) + (16x16Division=8bit) + (RowCol=1bit) < 16bit

//---- Multi-Frame Composition Method Selection
#define     ONLY_USE_H              0               // Method-0: only use Homography
#define     USE_H_BM_V1             1               // Method-1: use Homography + BlockLumaMatching version 1
#define     USE_H_BM_V2             2               // Method-2: use Homography + BlockLumaMatching version 2
//#define     MF_COMPOSE_METHOD       USE_H_BM_V2     // Multi-Frame Composition Method Selection: default USE_H_BM_V2

//---- Block Luma Matching Params Setting
#define     LUMA_MATCH_WIN_SIZE     32              // Luma Matching Win size in Raw
#define     LUMA_MATCH_WIN_NUM		1//4				// Num of Block: 32x(32*4) = 32x128 Raw(DDR->DSP)
#define     LUMA_MATCH_RADIUS       2				// Block Matching search radius in Luma
//#define		USE_PIXEL_NOISE_STD_TH	0				// 0-use Region Noise Std Threshold, 1-use BayerPixel Noise Std Threshold
#if USE_PIXEL_NOISE_STD_TH == 0
	#define	NOISE_STD_PARAM_A		0.29	 		// Noise Std Param A      A=0.29, B=17.0
	#define	NOISE_STD_PARAM_B		17.0			// Noise Std Param B      std = A * sqrt(aveLuma - B)
#else
	#define NOISE_STD_TALBE_LEN		1024			// Noise Std Table Length: 10bit
#endif
//#define	    MV_DIFF_THRESHOLD       200//6               // MV Difference Threshold (default 6)
#define     OLD_SEARCH_BRANCH       0               // Method-0: Old Method
#define     MODIFY_SEARCH_BRANCH    1               // Method-1: Modify Method
//#define     SEARCH_BRANCH_METHOD    0//OLD_SEARCH_BRANCH  // 0-Old Method, 1-Modify Method
//#define     USE_MOTION_DETECT       1               // Motion Detect: 1-use Motion Detect, 0-not use
#if USE_MOTION_DETECT == 1
    #define MOTION_DETECT_TH        1024//12              // Motion Detect Threshold
	#define MOTION_DETECT_TALBE_LEN	1024			// Motion Detect Table Length: 10bit
#endif

//---- Auto White Balance Params Setting
//#define     USE_AWB                 1               // 1-use Auto White Balance, 0-not use
#if USE_AWB == 1
    #define AWB_BLACK_LEVEL         64              // Black Level
#endif

//---- DetailEnhancer Params Setting
//#define     USE_DETAIL_ENHANCER     1               // 1-use DetailEnhancer, 0-not use

//---- LowLightEnhancer Params Setting
//#define     USE_LOW_LIGHT_ENHANCER  1               // 1-use LowLightEnhancer, 0-not use
#if USE_LOW_LIGHT_ENHANCER == 1
    #define LOW_LIGHT_TALBE_LEN		1024			// Low Light Table Length: 10bit
    #define BLACK_LEVEL             64              // Black Level
#endif

#define		TEST_PARAMS_NUM			20

//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//---- MAX() & MIN()
#define		MAX(a, b)			    ( (a) > (b) ? (a) : (b) )
#define		MIN(a, b)			    ( (a) < (b) ? (a) : (b) )

//---- ABS()
#define	    ABS_U16(a)			    (RK_U16)( (a) > 0 ? (a) : (-(a)) )

//---- ROUND()
#define	    ROUND_U16(a)		    (RK_U16)( (double) (a) + 0.5 )
#define	    ROUND_I32(a)		    (RK_S32)( (a) > 0 ? ((double) (a) + 0.5) : ((double) (a) - 0.5) )

//---- CEIL() & FLOOR()
#define	    CEIL(a)                 (int)( (double)(a) > (int)(a) ? (int)((a)+1) : (int)(a) )  
#define	    FLOOR(a)                (int)( (a) ) 

//---- FABS()
#define	    FABS(a)                 ( (a) >= 0 ? (a) : (-(a)) )


//////////////////////////////////////////////////////////////////////////
////-------- Class Definition

// class MFNR
class classMFNR
{
public:
    //// constructor & destructor
    classMFNR(void);                                // constructor
    ~classMFNR(void);                               // destructor
    
private: 



public: 
    //////////////////////////////////////////////////////////////////////////

    // RawSrcs
    int			    mRawWid;					        // Raw data width
    int			    mRawHgt;					        // Raw data height
    int			    mRawFileNum;				        // number of Raw data files
    int             mRaw10Stride;                       // Raw10bit data Stride (Bytes, 4ByteAlign)
    int             mRaw10DataSize;                     // Raw10bit data Size (Bytes)
    int             mRaw16Stride;                       // Raw16bit data Stride (Bytes, 4ByteAlign)
    int             mRaw16DataSize;                     // Raw10bit data Size (Bytes)
    RK_U16*         pRawSrcs[RK_MAX_FILE_NUM];          // Raw Srcs data pointers

    // Scaler
    int             mScaleRaw2Raw;                      // scale factor for Raw to Raw (for Preview)
    int             mScaleRaw2Luma;                     // scale factor for Raw to Luma
    int             mScaleRaw2Thumb;                    // scale factor for Raw to Thumbnail

    // ThumbSrcs
    int			    mThumbWid;					        // Thumb data width
    int			    mThumbHgt;					        // Thumb data height
    int             mThumb16Stride;                     // Thumb data Stride (Bytes, 4ByteAlign)
    int             mThumb16DataSize;                   // Thumb data Size (Bytes)
    RK_U16*         pThumbSrcs[RK_MAX_FILE_NUM];        // Thumb Srcs data pointers

    //////////////////////////////////////////////////////////////////////////

    //// DSP Memory
    int             DSP_UsedCount;  // DSP Memory used count

    //// Determine Base Frame
    int             mThumbFeatWinSize;                  // Thumb Feature Win Size
    int             mThumbDivSegCol;                    // number of Seg Col
    int             mThumbDivSegRow;                    // number of Seg Row
    RK_U32*         pSharpOfAllFrm;                     // Sharpness of All Frame -> baseFrame
    RK_U32*         pSharpOfAllSeg[RK_MAX_FILE_NUM];    // Sharpness of All 32x32 Segment in Frames
    RK_U32*         pMaxSharpFrmSeg[RK_MAX_FILE_NUM];   // MaxSharpness in each 32x32 Segment in Frames
    RK_U16*         pThumbDataChunk;                    // Thumb Data Chunk
    int             mBasePicNum;                        // Base Picture Num
    RK_U8*          pValidMark;                         // Frame Mark: 0-Base 1-ValidRef 2-InvalidRef
    int             mCntValidRefFrm;                    // count Valid RefFrame
    int             mNumValidFeature;                   // num of Valid Feature
    RK_U32*         pMaxSharpBaseSeg;                   // MaxSharpness in each 32x32 Segment in BaseFrame

    //// Block Coarse Matching
    RK_U16*         pMatchPoints;   // MatchResult: { [RegMarkRow, RegMarkCol], [matchRow, matchCol, SAD]*RawFileNum } * cntFeature
    RK_U16*         pThumbBaseBlk;  // 16x16Block in 32x32 Segment in BaseFrame
    RK_U16*         pThumbRefBlk;   // (16+2*radius)x(16+2*radius')Block in RefFrame

    //// Block Fine Matching
    RK_U16*         pRawBaseBlk;        // 64x64Block in 256x256 Segment in BaseFrame
    RK_U16*         pRawBaseBlkLuma;    // Luma of 64x64Block in 256x256 Segment in BaseFrame
    RK_U16*         pRawRefBlk;		    // (64+2*radius)x(64+2*radius')Block in RefFrame
    RK_U16*         pRawRefBlkLuma;	    // Luma of (64+2*radius)x(64+2*radius')Block in RefFrame

    //// Compute Homography
    RK_U8*          pRowMvHist;             // RowMV Hist
    RK_U8*          pColMvHist;             // ColMV Hist
    RK_U8*          pMarkMatchFeature;      // Marks of Match Feature in BaseFrame & RefFrame#k
    RK_U16*         pAgentsIn4x4Region;     // Agents in 4x4 Region [RegMark4x4, Sharp/SAD, 2*RawFileNum]*16
    RK_U16*         pRegion4Points;         // 4 points in 4 Regions: [x0,y0,x1,y1] * 4Points * 2Byte
    RK_F32*         pHomographyMatrix;      // Homography: [9*RawFileNum] * 4Byte
    RK_F32*         pMatrixA;               // Coefficient Matrix A for A*X = B: 8*8*4Byte
    RK_F32*         pVectorB;               // Coefficient Vector B for A*X = B: 8*1*4Byte
    RK_F32*         pVectorX;               // Coefficient Vector X for A*X = B: 3*3*4Byte
    RK_S32*         pProjectPoint;          // Perspective Project Point: 2*4Byte

    //// BlockMatching & Multi-Frame MV Compensate
    RK_U16*         pBlkMatchBaseBlks[LUMA_MATCH_WIN_NUM];	    // BlockMatching Base Blocks: 32*32*2Byte * 4
    RK_U16*         pBlkMatchBaseBlksLuma[LUMA_MATCH_WIN_NUM];  // Luma of BlockMatching Base Blocks: 16*16*2Byte * 4
    RK_U16*		    pBlkMatchRefBlks[LUMA_MATCH_WIN_NUM];	    // BlockMatching Ref Block: (32+2*radius)*(32+2*radius')*2Byte * 4
    RK_U16*		    pBlkMatchRefBlksLuma[LUMA_MATCH_WIN_NUM];   // Luma of BlockMatching Ref Block: (32+2*radius)/2*(32+2*radius')/2*2Byte * 4
    RK_U16*		    pBlkMatchDstBlks[LUMA_MATCH_WIN_NUM];       // BlockMatching Dst Block: 32*32*2Byte * 4
    RK_U16*         pBlkCenterPoint;				            // Block Center Point: 2*2Byte
    RK_S16*         pTwoLineBlkMPs[2];                          // Even & Odd Block Match Points Cache: ceil(ceil(RawWid/32)/4)*4 * 2 * 6 * 2Byte * 2
    RK_U16*         pMatchResultRects;                          // Match Result Rects of All RefFrame: 2 * 6 * 2Bte
    RK_U8*          pBlockWeights;                              // Block Weights: 32*32*4*1Byte
    RK_U16*         pBlkMatchBaseBlksFilt[LUMA_MATCH_WIN_NUM];  // Filtered Base Raw data
    RK_U16*         pBlkMatchRefBlksFilt[LUMA_MATCH_WIN_NUM];   // Filtered Ref Raw data

    //// Auto White Balance
    char            mBayerType[8];                              // Bayer Type: RGGB ...
    RK_F32          mAwbGains[4];                               // AWB Gains
	//
	// 
	int				mLumIntensity;
	RK_F32			mSensorGain;
	RK_F32			mIspGain;
	RK_F32			mShutter;
	int				mLuxIndex;
	int				mExpIndex;
	RK_S16			mBlackLevel[4];
	//

	//---- testParams[]
	RK_F32			mTestParams[TEST_PARAMS_NUM];
	// mTestParams[0]	 0-Not use MAX(64,refValue), 1-Use MAX(64,refValue)    
	// mTestParams[1]	 1-Use PreGain-PostBlk   2-Use PreBlk-PostGain
	// mTestParams[2]    n-Gain x n, n=1,2,... (n>=1, float)
	// mTestParams[3]    0-LinearGain, 1-OldNonLinearGain, 2-NewNonLinearGain, 3-LnNonLinearGain_WDR
	// mTestParams[4]    0-FixedMotionDetectThreshold, n-MotionDetectTable[n], n=1,2,...,8
	// mTestParams[5]    0-OldWdrParamPol, 1-NewWdrParamMax
	// mTestParams[6]    0-NotUseRegister, 1-UseRegister
	// mTestParams[7]    0-NotUseOppoBlk, 1-UseOppoBlk
	// mTestParams[8]    n-nFrameCompose, n = 1,2,3,...
	// mTestParams[9]    1-NonOverlap, 2-OverlapStep1/2, 4-OverlapStep1/4, ...
	// mTestParams[10]   0-NotUseSpatialDenoise, 1-UseSpatialDenoise, 2-UseEdgeDenoise

    //// Detail Enhancer


    //// Low Light Enhancer


    //////////////////////////////////////////////////////////////////////////

    // RawDst
    RK_U16*         pRawDst;    // Raw Dst data pointer


	// WDR
	RK_U16*         pRawDstGain;    // for ln-test
	RK_U16*         pRawDstGainWDR; // for ln-test
	RK_U16*         pRawDstGainWDR_CEVA; // for ln-test
	
	// Spatial Denoise
	RK_U32*			pRawDstWeight;  // for zlf-SpaceDenoise
	RK_U16*         pRawDstCpy;     // Raw Dst data pointer
    //
    //////////////////////////////////////////////////////////////////////////

public: 
    //// Process Functions-1: Determine Base Frame
    int ComputeGrad(int numFrame, int rowSeg);  // Compute Grad
    int DetermineBaseFrame();           // Determine Base Frame

    //// Process Functions-2: Block Coarse Matching
    int GetThumbBlock(int numFrame, int rowDDR, int colDDR, int hgtDDR, int widDDR, RK_U16* pThumbDsp); // Get Thumb Block
    int ThumbSparseMatching(RK_U16* pThumbBase, int widBase, int hgtBase, RK_U16* pThumbRef, int widRef, int hgtRef, int colStart, RK_U16& row, RK_U16& col, RK_U16& sad);  // Thumb Sparse Matching
    int BlockCoarseMatching();          // Block Coarse Matching
    
    //// Process Functions-3: Block Fine Matching
    int GetRaw16Block(int numFrame, int rowDDR, int colDDR, int hgtDDR, int widDDR, RK_U16* pRawDsp);   // Get Raw16 Block
    int Scaler_Raw2Luma(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, RK_U16* pLumaData); // Scaler Raw to Luma
    int LumaSparseMatching(RK_U16* pLumaBase, int widBase, int hgtBase, RK_U16* pLumaRef, int widRef, int hgtRef, int colStart, RK_U16& row, RK_U16& col, RK_U32& sad);     // Luma Sparse Matching
    int BlockFineMatching();            // Block Fine Matching

    //// Process Functions-4: Compute Homography
    int GetRegion4Points(RK_U16* pAgents, int wid, RK_U8* pTable, int idx, int numBase, int numRef, RK_U16* pPoints4); // Get 4 points in 4 Regions
    int CreateCoefficient(RK_U16* pPoints4, RK_F32* pMatA, RK_F32* pVecB);          // Create Coefficient MatrixA & VectorB
    int GetPerspectMatrix(RK_F32* pMatA, RK_F32* pVecB, RK_F32* pVecX);             // Get a PersPective Matrix
    int PerspectProject(RK_F32* pVecX, RK_U16* pBasePoint, RK_S32* pProjectPoint);  // Perspective Project: pVecX * pBasePoint = pProjectPoint
    int ComputeHomography();            // Compute Homography

    //// Process Functions-5: BlockMatching & Multi-Frame MV Compensate
    int PullRaw16Block(RK_U16* pRawDsp, int rowDDR, int colDDR, int hgtDDR, int widDDR); // Pull Raw16 Block
    int LumaDenseMatching(RK_U16* pLumaBase, int widBase, int hgtBase, RK_U16* pLumaRef, int widRef, int hgtRef, int colStart, RK_U16& row, RK_U16& col, RK_U16& sad);   // Luma Dense Matching
    int MotionDetectFilter(RK_U16* pRawData, int wid, int hgt, RK_U16* pFiltData); // Motion Detect Filter
    int BlockMatching_MvCompensate();   // BlockMatching & Multi-Frame MV Compensate

	//// Process Functions-6: Spatial Denoise
	int SpatialDenoise();				// SpatialDenoise
	int EdgeDenoise();					// EdgeDenoise

    //// Process Functions-7: Auto White Balance
    int AutoWhiteBalance();

    //// Process Functions-8: Detail Enhancer
    int DetailEnhancer();               // Detail Enhancer

    //// Process Functions-9: Low Light Enhancer
    int LowLightEnhancer();				// Low Light Enhancer

	//// Draw Test Params
	int DrawTestParams(int numTP);		// Draw Test Params


    //// Interface Functions
    int MFNR_Open(int wid, int hgt, int stride, int size, int numPic, 
		RK_U8* bayerType, RK_F32 redGain, RK_F32 blueGain, int lumIntensity,
		RK_F32 sensorGain, RK_F32 ispGain, RK_F32 shutter,
		int luxIndex, int expIndex, RK_S16 blackLevel[],
		void* pRSrcs[], RK_U16* pTSrcs[], 
		/*RK_F32 useGain*/
		RK_F32 testParams[]);						// MFNR Initialization

    int MFNR_Execute(void* pOutData);		// MFNR Execute
    int MFNR_Close();						// MFNR Close


};


//////////////////////////////////////////////////////////////////////////

#endif // _RK_MFNR_H



