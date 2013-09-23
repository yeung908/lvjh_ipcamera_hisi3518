#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "videoInModule.h"
#include "../videoEnc/videoEncModule.h"


#include "../include/sample_comm.h"

/*AR0130 DC 12bit输入720P@30fps*/
VI_DEV_ATTR_S DEV_ATTR_AR0130_NEW_DC_720P =
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFF00000,    0x0}, 
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,
     
    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_VALID_SINGAL,VI_VSYNC_VALID_NEG_HIGH,
    
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {0,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     0,            720,        0,
    /*vsync1_vhb vsync1_act vsync1_hhb*/ 
     0,            0,            0}
    },    
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB
};



#define VI_DEBUG				1

static int g_Hi3511_vin_init_flag = 0;
static int g_Hi3511_vin_pub_attr_flag = 0;
static int g_Hi3511_vin_setup_flag = 0;
static int g_Hi3511_vin_standard = 0;
static int g_Hi3511_vin_format = 0;

// MASK
static g_hi3511_mask_flag[MAX_VIN_CHANNEL];
	
int Hi3511VinOpen(int nChannel)
{
	HI_S32 s32Ret;
	VB_CONF_S stVbConf = {0};
	MPP_SYS_CONF_S stSysConf = {0};
		
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;	
	}		

	g_Hi3511_vin_init_flag = 0;

	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
	
	memset(&stVbConf, 0, sizeof(VB_CONF_S));
	memset(&stSysConf, 0, sizeof(MPP_SYS_CONF_S));
	
#ifdef HD_CMOS
	printf("Hi3511VinOpen: HD_CMOS\n");

	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 1600*1200*2;
	stVbConf.astCommPool[0].u32BlkCnt = 5;
	
	stVbConf.astCommPool[1].u32BlkSize = 800*624*2;
	stVbConf.astCommPool[1].u32BlkCnt = 5;
	
	stVbConf.astCommPool[2].u32BlkSize = 640*320*2;
    stVbConf.astCommPool[2].u32BlkCnt = 5;
	
#endif

#ifdef CCD
	stVbConf.u32MaxPoolCnt = 64;
	stVbConf.astCommPool[0].u32BlkSize = 768*576*2;
	stVbConf.astCommPool[0].u32BlkCnt = 10;
	stVbConf.astCommPool[1].u32BlkSize = 384*288*2;
	stVbConf.astCommPool[1].u32BlkCnt = 10;
	stVbConf.astCommPool[1].u32BlkSize = 320*176*2;
	stVbConf.astCommPool[1].u32BlkCnt = 10;
#endif
		
	s32Ret = HI_MPI_VB_SetConf(&stVbConf);
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VB_SetConf failed 0x%x!\n", s32Ret);
		return -1;
	}
#ifdef VI_DEBUG
	else
	{
		printf("HI_MPI_VB_SetConf OK 0x%x!\n", s32Ret);
	}
#endif

	s32Ret = HI_MPI_VB_Init();
	if (HI_SUCCESS != s32Ret)
	{
		printf("HI_MPI_VB_Init failed 0x%x!\n", s32Ret);
		return -1;
	}
#ifdef VI_DEBUG
	else
	{
		printf("HI_MPI_VB_Init OK 0x%x!\n", s32Ret);
	}
#endif

	stSysConf.u32AlignWidth = 64;
	s32Ret = HI_MPI_SYS_SetConf(&stSysConf);
	if (HI_SUCCESS != s32Ret)
	{
		HI_MPI_VB_Exit();
		printf("conf : system config failed 0x%x!\n", s32Ret);
		return -1;
	}
#ifdef VI_DEBUG
	else
	{
		printf("HI_MPI_SYS_SetConf OK 0x%x!\n", s32Ret);
	}
#endif

	s32Ret = HI_MPI_SYS_Init();
	if (HI_SUCCESS != s32Ret)
	{
		HI_MPI_VB_Exit();
		printf("HI_MPI_SYS_Init err 0x%x\n", s32Ret);
		return -1;
	}
#ifdef VI_DEBUG
	else
	{
		printf("HI_MPI_SYS_Init OK 0x%x!\n", s32Ret);
	}
#endif


	g_Hi3511_vin_init_flag = 1;
	
	return 0; 
} 

HI_S32 IspSensorInit(void)
{
    HI_S32 s32Ret;

    /* 1. sensor init */
    sensor_init();

    /* 0: linear mode, 1: WDR mode */
    sensor_mode_set(0);

    /* 2. sensor register callback */
    s32Ret = sensor_register_callback();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: sensor_register_callback failed with %#x!\n", \
               __FUNCTION__, s32Ret);
        return s32Ret;
    }
    return HI_SUCCESS;
}


int Hi3511VinSetup(int nChannel, int nStandard)
{
	HI_S32 s32Ret = 0;
	VI_CHN_ATTR_S ViChnAttr;
	VI_CHN_ATTR_S stChnAttr;
	   RECT_S stCapRect;
	/*isp departmeter*/
	ISP_IMAGE_ATTR_S stImageAttr;
    ISP_INPUT_TIMING_S stInputTiming;

	VPSS_GRP VpssGrp;
   VPSS_CHN VpssChn;
   VPSS_GRP_ATTR_S stVpssGrpAttr;
   VPSS_CHN_ATTR_S stVpssChnAttr;
   VPSS_CHN_MODE_S stVpssChnMode;
   VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr;

   static pthread_t gs_IspPid;

	
	
    //ROTATE_E enRotate = ROTATE_NONE;
	int videoFormat = 0; 
	int videoStandard = 0;
	SAMPLE_VI_CHN_SET_E enViChnSet = VI_CHN_SET_NORMAL;
	VI_DEV_ATTR_S    stViDevAttr;
	
		
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;
	}
	IspSensorInit();
	memset(&ViChnAttr, 0, sizeof(VI_CHN_ATTR_S));
	memset(&stViDevAttr, 0, sizeof(VI_DEV_ATTR_S));
	printf("Hi3511VinSetup: %d %x\n", nChannel, nStandard);
	
	videoFormat = ((nStandard>>24) & 0x0F);

#ifdef CCD
	videoStandard = (nStandard & 0x0F);
	if (videoStandard!=0 && videoStandard!=1)
	{
		return -1;
	}
	g_Hi3511_vin_standard = videoStandard;
	printf("__func__ = %s , __LINE__ = %d  %d\n", __func__, __LINE__, g_Hi3511_vin_standard);
	
#endif	
	
	if (g_Hi3511_vin_setup_flag == 1)
	{
		s32Ret = HI_MPI_VI_DisableChn(nChannel);
		if(HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_VI_DisableDevChn failed 0x%x!\n", s32Ret);
			return -1;
		}

		s32Ret = HI_MPI_VI_DisableDev(nChannel);
		if(HI_SUCCESS != s32Ret)
		{
			printf("HI_MPI_VI_UnBindOutput failed 0x%x!\n", s32Ret);
			return -1;
		}
	
		g_Hi3511_vin_setup_flag = 0;
	}


	memcpy(&stViDevAttr,&DEV_ATTR_AR0130_NEW_DC_720P,sizeof(VI_CHN_ATTR_S));
	s32Ret = HI_MPI_VI_SetDevAttr(nChannel, &stViDevAttr);
	  if (s32Ret != HI_SUCCESS)
	  {
		  SAMPLE_PRT("HI_MPI_VI_SetDevAttr failed with %#x!\n", s32Ret);
		  return HI_FAILURE;
	  }


	s32Ret = HI_MPI_VI_EnableDev(nChannel);
	 if (s32Ret != HI_SUCCESS)
	 {
		 SAMPLE_PRT("HI_MPI_VI_EnableDev failed with %#x!\n", s32Ret);
		 return HI_FAILURE;
	 }
 

#ifdef HD_CMOS
	switch (videoFormat)
	{
	case 4:
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
	    ViChnAttr.stCapRect.u32Width  = 352;
	    ViChnAttr.stCapRect.u32Height = 288;
	    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	     ViChnAttr.bChromaResample = HI_FALSE;
	    g_Hi3511_vin_format = 7;
	    printf("VI: 320*240\n");
		break;
		
	case 5:		//  VGA
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
	    ViChnAttr.stCapRect.u32Width  = 320;
		ViChnAttr.stCapRect.u32Height = 288;
	    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	    ViChnAttr.bChromaResample = HI_FALSE;
	    g_Hi3511_vin_format = 7;
	    printf("VI: 320*240\n");
		break;
	case 6:		// UXVGA
	#if 0
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
	    ViChnAttr.stCapRect.u32Width  = 1600;
	    ViChnAttr.stCapRect.u32Height = 1200;
	    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	    ViChnAttr.bChromaResample = HI_FALSE;
		#endif
		
	   g_Hi3511_vin_format = 6;
	    printf("VI: 1600*1200\n");
		break;
		
	case 7:		// 720P
#if 0
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
	    ViChnAttr.stCapRect.u32Width  = 1280;
	    ViChnAttr.stCapRect.u32Height = 720;
	    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	    ViChnAttr.bChromaResample = HI_FALSE;
	    g_Hi3511_vin_format = 7;
#endif	
	    printf("VI: 1280*720\n");
	    break;

	case 8:		// 720P
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
	    ViChnAttr.stCapRect.u32Width  = 640;
	    ViChnAttr.stCapRect.u32Height = 480;
	    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
	    ViChnAttr.bChromaResample = HI_FALSE;
	    g_Hi3511_vin_format = 7;
	    printf("VI: 720*576\n");
	    break;	
	case 9: 	// 720P
		ViChnAttr.stCapRect.s32X = 0;
		ViChnAttr.stCapRect.s32Y = 0;
		ViChnAttr.stCapRect.u32Width  = 640;
		ViChnAttr.stCapRect.u32Height = 480;
		ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
		ViChnAttr.bChromaResample = HI_FALSE;
		g_Hi3511_vin_format = 7;
		printf("VI: 640*480\n");
		break;	
	default:
		return -1;
	}		
#endif
	ViChnAttr.stCapRect.s32X = 0;
	ViChnAttr.stCapRect.s32Y = 0;
    ViChnAttr.stDestSize.u32Width  = 1280;
    ViChnAttr.stDestSize.u32Height = 720;
    ViChnAttr.enCapSel = VI_CAPSEL_BOTH;
    ViChnAttr.bChromaResample = HI_FALSE;
   
    ViChnAttr.bMirror = HI_FALSE;
    ViChnAttr.bFlip = HI_FALSE;
	ViChnAttr.s32SrcFrameRate = 30;
	ViChnAttr.s32FrameRate = 30;
	ViChnAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;   /* sp420 or sp422 */

	
	/* 1. isp init */
	  s32Ret = HI_MPI_ISP_Init();
	  if (s32Ret != HI_SUCCESS)
	  {
		  printf("%s: HI_MPI_ISP_Init failed!\n", __FUNCTION__);
		  return s32Ret;
	  }

    /* 2. isp set image attributes */
    /* note : different sensor, different ISP_IMAGE_ATTR_S define.
              if the sensor you used is different, you can change
              ISP_IMAGE_ATTR_S definition */
           
     stImageAttr.enBayer      = BAYER_GRBG;
     stImageAttr.u16FrameRate = 30;
     stImageAttr.u16Width     = 1280;
     stImageAttr.u16Height    = 720;

	 s32Ret = HI_MPI_ISP_SetImageAttr(&stImageAttr);
     if (s32Ret != HI_SUCCESS)
     {
         printf("%s: HI_MPI_ISP_SetImageAttr failed with %#x!\n", __FUNCTION__, s32Ret);
         return s32Ret;
     }
	  /* 3. isp set timing */

    stInputTiming.enWndMode = ISP_WIND_NONE;
    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
	if (s32Ret != HI_SUCCESS)
	{
		printf("%s: HI_MPI_ISP_SetInputTiming failed with %#x!\n", __FUNCTION__, s32Ret);
		return s32Ret;
	}

	if (0 != pthread_create(&gs_IspPid, 0, (void* (*)(void*))HI_MPI_ISP_Run, NULL))
	{
		printf("%s: create isp running thread failed!\n", __FUNCTION__);
		return HI_FAILURE;
	}

	stCapRect.s32X = 0;
	stCapRect.s32Y = 0;
    stCapRect.u32Width = 1280;
    stCapRect.u32Height = 720;
   
   	memcpy(&stChnAttr.stCapRect, &stCapRect, sizeof(RECT_S));


	stChnAttr.enCapSel = VI_CAPSEL_BOTH;
   /* to show scale. this is a sample only, we want to show dist_size = D1 only */
     
   stChnAttr.stDestSize.u32Width = stCapRect.u32Width;
   stChnAttr.stDestSize.u32Height = stCapRect.u32Height;
   stChnAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;   /* sp420 or sp422 */

   stChnAttr.bMirror = HI_FALSE;
   stChnAttr.bFlip = HI_FALSE;
   
   stChnAttr.bChromaResample = HI_FALSE;
  stChnAttr.s32SrcFrameRate = 30;
  stChnAttr.s32FrameRate = 30;

	

	s32Ret = HI_MPI_VI_SetChnAttr(nChannel, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

	s32Ret = HI_MPI_VI_EnableChn(nChannel);
	 if (s32Ret != HI_SUCCESS)
	 {
		  SAMPLE_PRT("failed with %#x!\n", s32Ret);
		  return HI_FAILURE;
 	 }


	  VpssGrp = 0;
	  stVpssGrpAttr.u32MaxW = 1600;
	  stVpssGrpAttr.u32MaxH = 1200;
	  stVpssGrpAttr.bDrEn = HI_FALSE;
	  stVpssGrpAttr.bDbEn = HI_FALSE;
	  stVpssGrpAttr.bIeEn = HI_TRUE;
	  stVpssGrpAttr.bNrEn = HI_TRUE;
	  stVpssGrpAttr.bHistEn = HI_TRUE;
	  stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
	  stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
	  s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
	  if (HI_SUCCESS != s32Ret)
	  {
		  SAMPLE_PRT("Start Vpss failed!\n");
		  return -1;
		 }
	
	  s32Ret = SAMPLE_COMM_VI_BindVpss(0);
	  if (HI_SUCCESS != s32Ret)
	  {
		  SAMPLE_PRT("Vi bind Vpss failed!\n");
		 return -1;
	  }
	
	  VpssChn = 0;
	  memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
	  stVpssChnAttr.bFrameEn = HI_FALSE;
	  stVpssChnAttr.bSpEn	 = HI_TRUE;    
	  s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, HI_NULL, HI_NULL);
	  if (HI_SUCCESS != s32Ret)
	  {
		  SAMPLE_PRT("Enable vpss chn failed!\n");
		  return -1;
	  }
	
	  VpssChn = 1;
	  stVpssChnMode.enChnMode	  = VPSS_CHN_MODE_USER;
	  stVpssChnMode.bDouble 	  = HI_FALSE;
	  stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
	  stVpssChnMode.u32Width	  = 640;
	  stVpssChnMode.u32Height	  = 480;
	  s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
	  if (HI_SUCCESS != s32Ret)
	  {
		  SAMPLE_PRT("Enable vpss chn failed!\n");
		 return -1;
	  }
	
	  VpssChn = 3;
	  stVpssExtChnAttr.s32BindChn = 1;
	  stVpssExtChnAttr.s32SrcFrameRate = 30;
	  stVpssExtChnAttr.s32DstFrameRate = 30;
	  stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
	  stVpssExtChnAttr.u32Width 	   = 320;
	  stVpssExtChnAttr.u32Height	   = 240;
	  s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
	  if (HI_SUCCESS != s32Ret)
	  {
		  SAMPLE_PRT("Enable vpss chn failed!\n");
		  return -1;
	  }

	 
	 
	printf("__func__ = %s , __LINE__ = %d VI SETUP  isp init HI_MPI_ISP_Run devatrr chnattr success!!!\n", __func__, __LINE__);
	
	g_Hi3511_vin_setup_flag = 1;

	return 0;
}

int Hi3511VinGetSetup(int nChannel, int *standard)
{
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;	
	}
	if (standard ==NULL)
	{
		return -1;	
	}
	
	return 0;
}

int Hi3511VinStart(int nChannel)
{	
	return 0;
}

int Hi3511VinStop(int nChannel)
{
	return 0;
}

int Hi3511VinGetStream(int nChannel, void *stream, int *size)
{	
	if (stream==NULL || size==NULL)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;
	}	
	
	*size = 0;
	
	return 0;
}

int Hi3511VinReleaseStream(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;
	}	
	
	return 0;
}

int Hi3511VinSetMask(int nChannel, VIDEO_MASK mask)
{
	return 0;	
}



int GetVideoNewEncAttr(void)
{


	SAMPLE_COMM_VENC_Start(0,0, PT_H264, VIDEO_ENCODING_MODE_PAL, PIC_HD720, SAMPLE_RC_CBR);
	SAMPLE_COMM_VENC_BindVpss(0, 0, 0);
 
#if 0
	SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
	PIC_SIZE_E enSize = PIC_HD720;
	VIDEO_NORM_E enNorm = VIDEO_ENCODING_MODE_PAL;

	VENC_ATTR_H264_CBR_S    stH264Cbr;
    VENC_ATTR_H264_VBR_S    stH264Vbr;
    VENC_ATTR_H264_FIXQP_S  stH264FixQp;
    VENC_ATTR_MJPEG_S stMjpegAttr;
    VENC_ATTR_MJPEG_FIXQP_S stMjpegeFixQp;
    VENC_ATTR_JPEG_S stJpegAttr;
    SIZE_S stPicSize;

	int s32Ret = 0;
	 s32Ret = HI_MPI_VENC_CreateGroup(0);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VENC_CreateGroup[%d] failed with %#x!\n",\
                 0, s32Ret);
        return HI_FAILURE;
    }
	memset(pstAttr, 0, sizeof(VENC_CHN_ATTR_S));
	{
		VENC_ATTR_H264_S stH264Attr;
		
		memset(&stH264Attr, 0, sizeof(VENC_ATTR_H264_S));
		
		stH264Attr.u32MaxPicWidth = pEncParam->nEncodeWidth;
		stH264Attr.u32MaxPicHeight = pEncParam->nEncodeHeight;
		stH264Attr.u32PicWidth = pEncParam->nEncodeWidth;/*the picture width*/
		stH264Attr.u32PicHeight = pEncParam->nEncodeWidth;/*the picture height*/
		stH264Attr.u32BufSize  = pEncParam->nEncodeWidth * pEncParam->nEncodeWidth* 2;/*stream buffer size*/
		stH264Attr.u32Profile  = 0;/*0: baseline; 1:MP; 2:HP   ? */
		stH264Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
		stH264Attr.bField = HI_FALSE;  /* surpport frame code only for hi3516, bfield = HI_FALSE */
		stH264Attr.bMainStream = HI_TRUE; /* surpport main stream only for hi3516, bMainStream = HI_TRUE */
		stH264Attr.u32Priority = 0; /*channels precedence level. invalidate for hi3516*/
		stH264Attr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame. Invalidate for hi3516*/
		memcpy(&pstAttr->stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

		if(SAMPLE_RC_CBR == enRcMode)
		{
			pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
			stH264Cbr.u32Gop			= (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264Cbr.u32StatTime		= 1; /* stream rate statics time(s) */
			stH264Cbr.u32ViFrmRate		= (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* input (vi) frame rate */
			stH264Cbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* target frame rate */
			switch (enSize)
			{
			  case PIC_QCIF:
				   stH264Cbr.u32BitRate = 256; /* average bit rate */
				   break;
			  case PIC_QVGA:	/* 320 * 240 */
			  case PIC_CIF: 

				   stH264Cbr.u32BitRate = 512;
				   break;

			  case PIC_D1:
			  case PIC_VGA:    /* 640 * 480 */
				   stH264Cbr.u32BitRate = 768;
				   break;
			  case PIC_HD720:	/* 1280 * 720 */
				   stH264Cbr.u32BitRate = 1024*2;
				   break;
			  case PIC_HD1080:	/* 1920 * 1080 */
				   stH264Cbr.u32BitRate = 1024*4;
				   break;
			  default :
				   stH264Cbr.u32BitRate = 1024;
				   break;
			}
			
			stH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
			memcpy(&pstAttr->stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
		}
		else if (SAMPLE_RC_FIXQP == enRcMode) 
		{
			pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
			stH264FixQp.u32Gop = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264FixQp.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264FixQp.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264FixQp.u32IQp = 20;
			stH264FixQp.u32PQp = 23;
			memcpy(&pstAttr->stRcAttr.stAttrH264FixQp, &stH264FixQp,sizeof(VENC_ATTR_H264_FIXQP_S));
		}
		else if (SAMPLE_RC_VBR == enRcMode) 
		{
			pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
			stH264Vbr.u32Gop = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264Vbr.u32StatTime = 1;
			stH264Vbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264Vbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
			stH264Vbr.u32MinQp = 24;
			stH264Vbr.u32MaxQp = 32;
			switch (enSize)
			{
			  case PIC_QCIF:
				   stH264Vbr.u32MaxBitRate= 256*3; /* average bit rate */
				   break;
			  case PIC_QVGA:	/* 320 * 240 */
			  case PIC_CIF:
				   stH264Vbr.u32MaxBitRate = 512*3;
				   break;
			  case PIC_D1:
			  case PIC_VGA:    /* 640 * 480 */
				   stH264Vbr.u32MaxBitRate = 1024*2*3;
				   break;
			  case PIC_HD720:	/* 1280 * 720 */
				   stH264Vbr.u32MaxBitRate = 1024*3*3;
				   break;
			  case PIC_HD1080:	/* 1920 * 1080 */
				   stH264Vbr.u32MaxBitRate = 1024*6*3;
				   break;
			  default :
				   stH264Vbr.u32MaxBitRate = 1024*4*3;
				   break;
			}
			memcpy(&pstAttr->stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
		}
		else
		{
			return HI_FAILURE;
		}
	}
	
	s32Ret = HI_MPI_VENC_CreateChn(0, pstAttr);
	   if (HI_SUCCESS != s32Ret)
	   {
		   SAMPLE_PRT("HI_MPI_VENC_CreateChn [%d] faild with %#x!\n",\
				   0, s32Ret);
		   return s32Ret;
	   }
	
	   /******************************************
		step 3:  Regist Venc Channel to VencGrp 
	   ******************************************/
	   s32Ret = HI_MPI_VENC_RegisterChn(0, 0);
	   if (HI_SUCCESS != s32Ret)
	   {
		   SAMPLE_PRT("HI_MPI_VENC_RegisterChn faild with %#x!\n", s32Ret);
		   return HI_FAILURE;
	   }
	
	   /******************************************
		step 4:  Start Recv Venc Pictures
	   ******************************************/
	   s32Ret = HI_MPI_VENC_StartRecvPic(0);
	   if (HI_SUCCESS != s32Ret)
	   {
		   SAMPLE_PRT("HI_MPI_VENC_StartRecvPic faild with%#x!\n", s32Ret);
		   return HI_FAILURE;
	   }
	   #endif
	   

	return 0;
}



int Hi3511VinClose(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_VIN_CHANNEL)
	{
		return -1;	
	}
	
	HI_MPI_SYS_Exit();
	HI_MPI_VB_Exit();
	
	g_Hi3511_vin_init_flag = 0;

	return 0;
}

#define SENSOR_TYPE  APTINA_AR0130_DC_720P_30FPS
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;


#if 1
HI_S32 SAMPLE_VENC_720P_CLASSIC(HI_VOID)
{
    PAYLOAD_TYPE_E enPayLoad[3]= {PT_H264, PT_H264, PT_H264};
    PIC_SIZE_E enSize[3] = {PIC_HD720, PIC_VGA, PIC_QVGA};

    VB_CONF_S stVbConf;
    SAMPLE_VI_CONFIG_S stViConfig;
    
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;
    VPSS_EXT_CHN_ATTR_S stVpssExtChnAttr;
    
    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    SAMPLE_RC_E enRcMode= SAMPLE_RC_CBR;
    HI_S32 s32ChnNum = 3;
    
    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;

	printf("classic 720P \n");
    /******************************************
     step  1: init sys variable 
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    if (SONY_IMX122_DC_1080P_30FPS == SENSOR_TYPE)
    {
        enSize[0] = PIC_HD1080;
    }

    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/   
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[0], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 10;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[1], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 6;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enSize[2], SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[2].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[2].u32BlkCnt = 6;
    
    /* hist buf*/
    stVbConf.astCommPool[3].u32BlkSize = (196*4);
    stVbConf.astCommPool[3].u32BlkCnt = 6;

    /******************************************
     step 2: mpp system init. 
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_VENC_720P_CLASSIC_0;
    }
	printf("sys init suceess!\n");

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    stViConfig.enViMode   = SENSOR_TYPE;
    stViConfig.enRotate   = ROTATE_NONE;
    stViConfig.enNorm     = VIDEO_ENCODING_MODE_AUTO;
    stViConfig.enViChnSet = VI_CHN_SET_NORMAL;
    s32Ret = SAMPLE_COMM_VI_StartVi(&stViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_VENC_720P_CLASSIC_1;
    }
   /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize[0], &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_VENC_720P_CLASSIC_1;
    }

    VpssGrp = 0;
    stVpssGrpAttr.u32MaxW = stSize.u32Width;
    stVpssGrpAttr.u32MaxH = stSize.u32Height;
    stVpssGrpAttr.bDrEn = HI_FALSE;
    stVpssGrpAttr.bDbEn = HI_FALSE;
    stVpssGrpAttr.bIeEn = HI_TRUE;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.bHistEn = HI_TRUE;
    stVpssGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stVpssGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stVpssGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_VENC_720P_CLASSIC_2;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(stViConfig.enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_VENC_720P_CLASSIC_3;
    }

    VpssChn = 0;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.bFrameEn = HI_FALSE;
    stVpssChnAttr.bSpEn    = HI_TRUE;    
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, HI_NULL, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_720P_CLASSIC_4;
    }
	

    VpssChn = 1;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = 640;
    stVpssChnMode.u32Height     = 480;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_720P_CLASSIC_4;
    }

    VpssChn = 3;
    stVpssExtChnAttr.s32BindChn = 1;
    stVpssExtChnAttr.s32SrcFrameRate = 30;
    stVpssExtChnAttr.s32DstFrameRate = 30;
    stVpssExtChnAttr.enPixelFormat   = SAMPLE_PIXEL_FORMAT;
    stVpssExtChnAttr.u32Width        = 320;
    stVpssExtChnAttr.u32Height       = 240;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, HI_NULL, HI_NULL, &stVpssExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_VENC_720P_CLASSIC_4;
    }

    /******************************************
     step 5: start stream venc
    ******************************************/
    /*** HD720P **/
    VpssGrp = 0;
    VpssChn = 0;
    VencGrp = 0;
    VencChn = 0;
    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[0],\
                                   gs_enNorm, enSize[0], enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    /*** vga **/
    VpssChn = 1;
    VencGrp = 1;
    VencChn = 1;
    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[1], \
                                    gs_enNorm, enSize[1], enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    /*** vga **/
    VpssChn = 3;
    VencGrp = 2;
    VencChn = 2;
    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad[2], \
                                    gs_enNorm, enSize[2], enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencChn, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }

    /******************************************
     step 6: stream venc process -- get stream, then save it to file. 
    ******************************************/
    #if 0
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(s32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_VENC_720P_CLASSIC_5;
    }
	
    printf("please press twice ENTER to exit this sample\n");
	return 0;
    printf("please press twice ENTER to exit this sample\n");
    getchar();
    getchar();

    /******************************************
     step 7: exit process
    ******************************************/
    SAMPLE_COMM_VENC_StopGetStream();
	#endif

	printf("return \n\n");
    return s32Ret;    
END_VENC_720P_CLASSIC_5:
    VpssGrp = 0;
    
    VpssChn = 0;
    VencGrp = 0;   
    VencChn = 0;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);

    VpssChn = 1;
    VencGrp = 1;   
    VencChn = 1;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);

    VpssChn = 3;
    VencGrp = 2;   
    VencChn = 2;
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);

    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_720P_CLASSIC_4:	//vpss stop
    VpssGrp = 0;
    VpssChn = 3;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 0;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    VpssChn = 1;
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
END_VENC_720P_CLASSIC_3:    //vpss stop       
    SAMPLE_COMM_VI_UnBindVpss(stViConfig.enViMode);
END_VENC_720P_CLASSIC_2:    //vpss stop   
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_VENC_720P_CLASSIC_1:	//vi stop
    SAMPLE_COMM_VI_StopVi(&stViConfig);
END_VENC_720P_CLASSIC_0:	//system exit
    SAMPLE_COMM_SYS_Exit();
    
    return s32Ret;    
}

#endif



static struct vinModuleInfo Hi3511VinInfoStruct = 
{
	open:           	Hi3511VinOpen,
	close:          	Hi3511VinClose,
	setup:				Hi3511VinSetup,
	getSetup:			Hi3511VinGetSetup,
	start:         		Hi3511VinStart,
	stop:				Hi3511VinStop,
	getStream:			Hi3511VinGetStream,
	releaseStream:		Hi3511VinReleaseStream,
	setMask:			Hi3511VinSetMask,
};

vinModuleInfo_t Hi3511VinInfo = &Hi3511VinInfoStruct;
