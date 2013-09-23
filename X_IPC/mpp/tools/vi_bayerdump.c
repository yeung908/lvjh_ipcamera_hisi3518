#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"

#include "hi_comm_isp.h"
#include "mpi_isp.h"
#include "hi_sns_ctrl.h"

#define MAX_FRM_CNT 256

pthread_t isp_pid;
static VO_DEV s_VoDev = 0;
static VO_CHN s_VoChn = 0;

#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))

#define VODEV_SD0 1

//#define SNS_AR0130

/* The enCompMode of VI_DEV_ATTR_S should be VI_COMP_MODE_DOUBLE,
and the au32CompMask array which is relative to the enCompMode should be configd,
and the isp should be disabled when we want to get the bayer RGB data. */


/*AR0130 DC 12bit输入*/
VI_DEV_ATTR_S DEV_ATTR_AR0130_DC_720P_BASE =
/* 典型时序3:7441 BT1120 720P@60fps典型时序 (对接时序: 时序)*/
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

VI_DEV_ATTR_S stBayerAr0130Attr =
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
    VI_PATH_RAW,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB
};

/*OV9712 DC 10bit输入*/
VI_DEV_ATTR_S DEV_ATTR_OV9712_DC_720P_BASE =
/* 典型时序3:7441 BT1120 720P@60fps典型时序 (对接时序: 时序)*/
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFC00000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
    
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {408,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     6,            720,        6,
    /*vsync1_vhb vsync1_act vsync1_hhb*/ 
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_ISP,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB
};

VI_DEV_ATTR_S stBayerOv9712Attr =
{
    /*接口模式*/
    VI_MODE_DIGITAL_CAMERA,
    /*1、2、4路工作模式*/
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFFC00000,    0x0},
    /*逐行or隔行输入*/
    VI_SCAN_PROGRESSIVE,
    /*AdChnId*/
    {-1, -1, -1, -1},
    /*enDataSeq, 仅支持YUV格式*/
    VI_INPUT_DATA_YUYV,

    /*同步信息，对应reg手册的如下配置, --bt1120时序无效*/
    {
    /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
    VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH, VI_HSYNC_VALID_SINGNAL,VI_HSYNC_NEG_HIGH,VI_VSYNC_NORM_PULSE,VI_VSYNC_VALID_NEG_HIGH,
    
    /*timing信息，对应reg手册的如下配置*/
    /*hsync_hfb    hsync_act    hsync_hhb*/
    {408,            1280,        0,
    /*vsync0_vhb vsync0_act vsync0_hhb*/
     6,            720,        6,
    /*vsync1_vhb vsync1_act vsync1_hhb*/ 
     0,            0,            0}
    },
    /*使用内部ISP*/
    VI_PATH_RAW,
    /*输入数据类型*/
    VI_DATA_TYPE_RGB
};

VI_CHN_ATTR_S stBayerChn1080PAttr =
{
    /*crop_x crop_y, crop_w  crop_h*/
    {0,     0, 1920,   1080},
    /*dest_w  dest_h  */
    {1920,   1080},
    /*enCapSel*/
    VI_CAPSEL_BOTH,
    /*   通道格式*/
    PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    /*bMirr  bFlip   bChromaResample*/
    0,      0,      0,
    /*s32SrcFrameRate   s32FrameRate*/
    -1, -1
};

VI_CHN_ATTR_S stBayerChn720PAttr =
{
    /*crop_x crop_y, crop_w  crop_h*/
    {0,     0, 1280,   720},
    /*dest_w  dest_h  */
    {1280,   720 },
    /*enCapSel*/
    VI_CAPSEL_BOTH,
    /*   通道格式*/
    PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    /*bMirr  bFlip   bChromaResample*/
    0,      0,      0,
    /*s32SrcFrameRate   s32FrameRate*/
    -1, -1
};

VI_CHN_ATTR_S stBayerChn720PIMX104Attr =
{
    /*crop_x crop_y, crop_w  crop_h*/
    {68,     20, 1280,   720},
    /*dest_w  dest_h  */
    {1280,   720 },
    /*enCapSel*/
    VI_CAPSEL_BOTH,
    /*   通道格式*/
    PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    /*bMirr  bFlip   bChromaResample*/
    0,      0,      0,
    /*s32SrcFrameRate   s32FrameRate*/
    -1, -1
};


VI_CHN_ATTR_S stBayerChn5MAttr =
{
    {0,     0,     2592,   1944 },
    {2592,  1944},
    /*enCapSel*/
    VI_CAPSEL_BOTH,
    /*   通道格式*/
    PIXEL_FORMAT_YUV_SEMIPLANAR_422,
    /*bMirr  bFlip   bChromaResample*/
    0,      0,      0,
    /*s32SrcFrameRate   s32FrameRate*/
    -1, -1

};


#ifdef SNS_PANSO34041
#endif

#ifdef SNS_AR0130
#define DEV_ATTR           DEV_ATTR_AR0130_DC_720P_BASE
#define DEV_BAYER_ATTR     stBayerAr0130Attr
#define CHN_BAYER_ATTR     stBayerChn720PAttr
#endif

#ifdef SNS_IMX104
#define DEV_ATTR           DEV_ATTR_AR0130_DC_720P_BASE
#define DEV_BAYER_ATTR     stBayerAr0130Attr
#define CHN_BAYER_ATTR     stBayerChn720PIMX104Attr
#endif

#ifdef SNS_OV9712
#define DEV_ATTR           DEV_ATTR_OV9712_DC_720P_BASE
#define DEV_BAYER_ATTR     stBayerOv9712Attr
#define CHN_BAYER_ATTR     stBayerChn720PAttr
#endif

/* default */
#ifndef DEV_BAYER_ATTR
#define DEV_ATTR           DEV_ATTR_AR0130_DC_720P_BASE
#define DEV_BAYER_ATTR     stBayerAr0130Attr
#define CHN_BAYER_ATTR     stBayerChn720PAttr
#endif

void sample_bayer_dump(VIDEO_FRAME_S * pVBuf, HI_U32 u32Nbit, FILE *pfd)
{
    unsigned int w, h;
    HI_U16* pVBufVirt_Y;
    HI_U16* pVBufVirt_C;
    HI_U8  au8Data[2000];
    HI_U16 au16Data[2000];
    HI_U32 phy_addr,size;
    HI_U8* pUserPageAddr[2];

    if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pVBuf->enPixelFormat)
    {
        printf("Err! Bayer data can't be 420\n");
        return;
    }

    size = (pVBuf->u32Stride[0])*(pVBuf->u32Height)*2;

    phy_addr = pVBuf->u32PhyAddr[0];

    pUserPageAddr[0] = (HI_U8 *) HI_MPI_SYS_Mmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        return;
    }

    pVBufVirt_Y = (HI_U16*)pUserPageAddr[0];
    pVBufVirt_C = pVBufVirt_Y + (pVBuf->u32Stride[0])*(pVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......Raw data......u32Stride[0]: %d, width: %d\n", pVBuf->u32Stride[0], pVBuf->u32Width);
    fflush(stderr);
    for(h=0; h<pVBuf->u32Height; h++)
    {
        HI_U16 u16Data;
        for (w=0; w<pVBuf->u32Width; w++)
        {
            if (8 == u32Nbit)
            {
                au8Data[w] = (pVBufVirt_Y[h*pVBuf->u32Width + w] >> 8);
            }
            else if (10 == u32Nbit)
            {
                u16Data = (pVBufVirt_Y[h*pVBuf->u32Width + w] >> 6);
                au16Data[w] = u16Data;
            }
            else if (12 == u32Nbit)         //12bit 在高位
            {
                u16Data = (pVBufVirt_Y[h*pVBuf->u32Width + w] >> 4);
                au16Data[w] = u16Data;
            }
            else
            {
                printf("Err! Bayer data can't support %d bits!eg: 8bits;10bits;12bits.\n", u32Nbit);
                return;
            }
        }

        if (8 == u32Nbit)
        {
            fwrite(au8Data, pVBuf->u32Width, 1, pfd);
        }
        else
        {
            fwrite(au16Data, pVBuf->u32Width, 2, pfd);
        }

    }
    fflush(pfd);

    fprintf(stderr, "done u32TimeRef: %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    HI_MPI_SYS_Munmap(pUserPageAddr[0], size);
}

HI_S32 VI_IspInit(void)
{
    HI_S32 s32Ret;
    ISP_IMAGE_ATTR_S stImageAttr;
    ISP_INPUT_TIMING_S stInputTiming;
    
    printf("sensor_init()\r\n");

    /* 1. sensor init */
    sensor_init();

    /* 2. sensor register callback */
    printf("sensor_register_callback()\r\n");
    s32Ret = sensor_register_callback();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: sensor_register_callback failed with %#x!\n", \
        __FUNCTION__, s32Ret);
        return s32Ret;
    }

    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: Sensor init failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_ISP_Init();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_Init failed!\n", __FUNCTION__);
        return s32Ret;
    }

    stImageAttr.enBayer         = BAYER_GRBG;
    stImageAttr.u16FrameRate    = 30;
    stImageAttr.u16Width        = 1280;
    stImageAttr.u16Height       = 720;
#ifdef SNS_IMX104
    stImageAttr.enBayer         = BAYER_GBRG;
    stImageAttr.u16FrameRate    = 30;
    stImageAttr.u16Width        = 1280;
    stImageAttr.u16Height       = 720;
#endif

#ifdef SNS_OV9712
    stImageAttr.enBayer         = BAYER_BGGR;
#endif

    s32Ret = HI_MPI_ISP_SetImageAttr(&stImageAttr);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_SetImageAttr failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    stInputTiming.enWndMode = ISP_WIND_NONE;
#ifdef SNS_IMX104
        stInputTiming.enWndMode = ISP_WIND_ALL;
        stInputTiming.u16HorWndStart = 0;
        stInputTiming.u16HorWndLength = 1348;
        stInputTiming.u16VerWndStart = 0;
        stInputTiming.u16VerWndLength = 740;
#endif


    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_SetInputTiming failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    if (0 != pthread_create(&isp_pid, 0, (void* (*)(void*))HI_MPI_ISP_Run, NULL))
    {
        printf("%s: create isp running thread failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 VI_StopIsp()
{
    /* 11. isp exit and main programme exit                                    */
    printf("Isp will exit!\n");
    HI_MPI_ISP_Exit();
    pthread_join(isp_pid, 0);

    return 0;
}

HI_S32 VI_DumpBayer(VI_CHN ViChn, HI_U32 u32Nbit, HI_U32 u32Cnt)
{
    int i, j;
    VI_FRAME_INFO_S stFrame;
    VI_FRAME_INFO_S astFrame[MAX_FRM_CNT];
    HI_CHAR szYuvName[128];
    FILE *pfd;

    if (HI_MPI_VI_SetFrameDepth(ViChn, 1))
    {
        printf("HI_MPI_VI_SetFrameDepth err, vi chn %d \n", ViChn);
        return -1;
    }

    usleep(5000);

    if (HI_MPI_VI_GetFrame(ViChn, &stFrame.stViFrmInfo))
    {
        printf("HI_MPI_VI_GetFrame err, vi chn %d \n", ViChn);
        return -1;
    }

    /* make file name */
    sprintf(szYuvName, "./vi_chn_%d_%d_%d_%d_%dbits.raw", ViChn,
        stFrame.stViFrmInfo.stVFrame.u32Width, stFrame.stViFrmInfo.stVFrame.u32Height, u32Cnt, u32Nbit);
    printf("Dump YUV frame of vi chn %d  to file: \"%s\"\n", ViChn, szYuvName);
    HI_MPI_VI_ReleaseFrame(ViChn, &stFrame.stViFrmInfo);

    /* open file */
    pfd = fopen(szYuvName, "wb");
    if (NULL == pfd)
    {
        return -1;
    }

    /* get VI frame  */
    for (i=0; i<u32Cnt; i++)
    {
        if (HI_MPI_VI_GetFrame(ViChn, &astFrame[i].stViFrmInfo)<0)
        {
            printf("get vi chn %d frame err\n", ViChn);
            printf("only get %d frame\n", i);
            break;
        }
    }

    for(j=0; j<i; j++)
    {
        /* save VI frame to file */
        sample_bayer_dump(&astFrame[j].stViFrmInfo.stVFrame, u32Nbit, pfd);

        /* release frame after using */
        HI_MPI_VI_ReleaseFrame(ViChn, &astFrame[j].stViFrmInfo);
    }

    fclose(pfd);    

    return 0;
}

int VI_InitMpp()
{
    MPP_SYS_CONF_S stSysConf = {0};
    VB_CONF_S stVbConf ={0};

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
    stVbConf.astCommPool[0].u32BlkSize = 1280*720*2;
    stVbConf.astCommPool[0].u32BlkCnt =20;
#ifdef SNS_9P031_5M
    stVbConf.astCommPool[0].u32BlkSize = 2624*1944*2;
    stVbConf.astCommPool[0].u32BlkCnt = 10;
#endif
#ifdef SNS_9P031_720P
    stVbConf.astCommPool[0].u32BlkSize = 1280*720*2;
    stVbConf.astCommPool[0].u32BlkCnt = 40;
#endif
#ifdef SNS_9M034
    stVbConf.astCommPool[0].u32BlkSize = 1280*720*2;
    stVbConf.astCommPool[0].u32BlkCnt = 40;
#endif

    if ( HI_MPI_VB_SetConf(&stVbConf))
    {
        printf("HI_MPI_VB_SetConf failed!\n");
        return HI_FAILURE;
    }

    if(HI_MPI_VB_Init())
    {
        printf("VI_InitMpp HI_MPI_VB_Init failed!\n");
        return HI_SUCCESS;
    }

    stSysConf.u32AlignWidth = 16;
    if ( HI_MPI_SYS_SetConf(&stSysConf))
    {
        printf("conf : system config failed!\n");
        return HI_SUCCESS;
    }

    if (HI_MPI_SYS_Init())
    {
        printf("sys init failed!\n");
        return HI_SUCCESS;
    }
    return HI_SUCCESS;
}


HI_S32 VI_GetDefVoAttr(VO_DEV VoDev, VO_INTF_SYNC_E enIntfSync, VO_PUB_ATTR_S *pstPubAttr,
    VO_VIDEO_LAYER_ATTR_S *pstLayerAttr, HI_S32 s32SquareSort, VO_CHN_ATTR_S *astChnAttr)
{
    VO_INTF_TYPE_E enIntfType;
    HI_U32 u32Frmt, u32Width, u32Height, j;

    if(VO_OUTPUT_PAL == enIntfSync || VO_OUTPUT_NTSC == enIntfSync )
    {
        enIntfType = VO_INTF_CVBS;
    }
    else
    {
        enIntfType = VO_INTF_BT1120;
    }

    switch (enIntfSync)
    {
        case VO_OUTPUT_PAL      :  u32Width = 720;  u32Height = 576;  u32Frmt = 25; break;
        case VO_OUTPUT_NTSC     :  u32Width = 720;  u32Height = 480;  u32Frmt = 30; break;
        case VO_OUTPUT_1080P24  :  u32Width = 1920; u32Height = 1080; u32Frmt = 24; break;
        case VO_OUTPUT_1080P25  :  u32Width = 1920; u32Height = 1080; u32Frmt = 25; break;
        case VO_OUTPUT_1080P30  :  u32Width = 1920; u32Height = 1080; u32Frmt = 30; break;
        case VO_OUTPUT_720P50   :  u32Width = 1280; u32Height = 720;  u32Frmt = 50; break;
        case VO_OUTPUT_720P60   :  u32Width = 1280; u32Height = 720;  u32Frmt = 60; break;
        case VO_OUTPUT_1080I50  :  u32Width = 1920; u32Height = 1080; u32Frmt = 50; break;
        case VO_OUTPUT_1080I60  :  u32Width = 1920; u32Height = 1080; u32Frmt = 60; break;
        case VO_OUTPUT_1080P50  :  u32Width = 1920; u32Height = 1080; u32Frmt = 50; break;
        case VO_OUTPUT_1080P60  :  u32Width = 1920; u32Height = 1080; u32Frmt = 60; break;
        case VO_OUTPUT_576P50   :  u32Width = 720;  u32Height = 576;  u32Frmt = 50; break;
        case VO_OUTPUT_480P60   :  u32Width = 720;  u32Height = 480;  u32Frmt = 60; break;
        case VO_OUTPUT_800x600_60: u32Width = 800;  u32Height = 600;  u32Frmt = 60; break;
        case VO_OUTPUT_1024x768_60:u32Width = 1024; u32Height = 768;  u32Frmt = 60; break;
        case VO_OUTPUT_1280x1024_60:u32Width =1280; u32Height = 1024; u32Frmt = 60; break;
        case VO_OUTPUT_1366x768_60:u32Width = 1366; u32Height = 768;  u32Frmt = 60; break;
        case VO_OUTPUT_1440x900_60:u32Width = 1440; u32Height = 900;  u32Frmt = 60; break;
        case VO_OUTPUT_1280x800_60:u32Width = 1280; u32Height = 800;  u32Frmt = 60; break;

        default: return HI_FAILURE;
    }

    if (NULL != pstPubAttr)
    {
        pstPubAttr->enIntfSync = enIntfSync;
        pstPubAttr->u32BgColor = 0xFF; //BLUE
        pstPubAttr->bDoubleFrame = HI_FALSE;
        pstPubAttr->enIntfType = enIntfType;
    }

    if (NULL != pstLayerAttr)
    {
        pstLayerAttr->stDispRect.s32X       = 0;
        pstLayerAttr->stDispRect.s32Y       = 0;
        pstLayerAttr->stDispRect.u32Width   = u32Width;
        pstLayerAttr->stDispRect.u32Height  = u32Height;
        pstLayerAttr->stImageSize.u32Width  = u32Width;
        pstLayerAttr->stImageSize.u32Height = u32Height;
        //pstLayerAttr->u32DispFrmRt          = u32Frmt;
        pstLayerAttr->u32DispFrmRt          = 25;
        pstLayerAttr->enPixFormat           = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
    }

    if (NULL != astChnAttr)
    {
        for (j=0; j<(s32SquareSort * s32SquareSort); j++)
        {
            astChnAttr[j].stRect.s32X       = ALIGN_BACK((u32Width/s32SquareSort) * (j%s32SquareSort), 4);
            astChnAttr[j].stRect.s32Y       = ALIGN_BACK((u32Height/s32SquareSort) * (j/s32SquareSort), 4);
            astChnAttr[j].stRect.u32Width   = ALIGN_BACK(u32Width/s32SquareSort, 4);
            astChnAttr[j].stRect.u32Height  = ALIGN_BACK(u32Height/s32SquareSort, 4);
            astChnAttr[j].u32Priority       = 0;
            astChnAttr[j].bDeflicker        = HI_FALSE;
        }
    }

    return HI_SUCCESS;
}

HI_S32 VI_StartVo1Pic(VO_DEV VoDev)
{
    HI_S32 i, s32ChnNum = 1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    VO_CHN_ATTR_S astChnAttr[VO_MAX_CHN_NUM];

    if (VoDev < VODEV_SD0)
    {
        #ifdef VI_TEST_PAL
        VI_GetDefVoAttr(VoDev, VO_OUTPUT_PAL, &stPubAttr, &stLayerAttr, 1, astChnAttr);
        #else
        VI_GetDefVoAttr(VoDev, VO_OUTPUT_NTSC, &stPubAttr, &stLayerAttr, 1, astChnAttr);
        #endif
    }
    else
    {
        #ifdef VI_TEST_PAL
            VI_GetDefVoAttr(VoDev, VO_OUTPUT_PAL, &stPubAttr, &stLayerAttr, 1, astChnAttr);
        #else
            VI_GetDefVoAttr(VoDev, VO_OUTPUT_NTSC, &stPubAttr, &stLayerAttr, 1, astChnAttr);
        #endif
    }

    HI_MPI_VO_SetPubAttr(VoDev, &stPubAttr);

    HI_MPI_VO_Enable(VoDev);

    HI_MPI_VO_SetVideoLayerAttr(VoDev, &stLayerAttr);

    HI_MPI_VO_EnableVideoLayer(VoDev);

    for (i=0; i<s32ChnNum; i++)
    {
        HI_MPI_VO_SetChnAttr(VoDev, i, &astChnAttr[i]);

        HI_MPI_VO_EnableChn(VoDev, i);
    }
    return 0;
}


HI_S32 VI_BindViVo(VI_CHN ViChn, VO_DEV VoDev, VO_CHN VoChn)
{
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId    = HI_ID_VIU;
    stSrcChn.s32DevId   = 0;
    stSrcChn.s32ChnId   = ViChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32ChnId  = VoChn;
    stDestChn.s32DevId  = VoDev;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

HI_S32 VI_UnBindViVo(VO_DEV VoDev, VO_CHN VoChn)
{
    MPP_CHN_S stDestChn;

    stDestChn.enModId   = HI_ID_VOU;
    stDestChn.s32DevId  = VoDev;
    stDestChn.s32ChnId  = VoChn;

    return HI_MPI_SYS_UnBind(NULL, &stDestChn);
}


HI_S32 VI_PreviewOnVo(VI_CHN ViChn)
{
    VI_CHN_ATTR_S stViChnAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    VO_CHN_ATTR_S stVoChnAttr = {0, {0,0,1280, 720}, HI_FALSE};
    SIZE_S *pstDestSize = NULL;
    PIXEL_FORMAT_E  enPixFormat;
    VO_DEV VoDev = s_VoDev;
    VO_CHN VoChn = s_VoChn;

    HI_MPI_VI_GetChnAttr(ViChn, &stViChnAttr);
    pstDestSize = &stViChnAttr.stDestSize;
    enPixFormat = stViChnAttr.enPixFormat;


    if (pstDestSize->u32Height < 32 || pstDestSize->u32Width < 32)
    {
        printf("vo not support Size, the size must larger (32x32)\n");
        return HI_SUCCESS;
    }
    #if 0
    printf("Chn:%d,W:%d,H:%d,fmt:sp%d\n", ViChn, pstDestSize->u32Width, pstDestSize->u32Height,
        (enPixFormat == PIXEL_FORMAT_YUV_SEMIPLANAR_422) ? 422:420);
    #endif
    HI_MPI_VO_GetVideoLayerAttr(VoDev, &stVoLayerAttr);

    if (stVoLayerAttr.enPixFormat != enPixFormat)
    {
        HI_MPI_VO_DisableChn(VoDev, VoChn);

        HI_MPI_VO_DisableVideoLayer(VoDev);
        stVoLayerAttr.enPixFormat = enPixFormat;
        HI_MPI_VO_SetVideoLayerAttr(VoDev, &stVoLayerAttr);
        HI_MPI_VO_EnableVideoLayer(VoDev);

        stVoChnAttr.stRect.u32Width = pstDestSize->u32Width;
        stVoChnAttr.stRect.u32Height = pstDestSize->u32Height;
        stVoChnAttr.stRect.u32Height = stVoChnAttr.stRect.u32Height / 4 * 4;//VO高度要4对齐
        HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr);
        HI_MPI_VO_EnableChn(VoDev, VoChn);
    }

    stVoChnAttr.stRect.s32X = 0;
    stVoChnAttr.stRect.s32Y = 0;
    stVoChnAttr.stRect.u32Width = pstDestSize->u32Width;
    stVoChnAttr.stRect.u32Height = pstDestSize->u32Height;
    stVoChnAttr.stRect.u32Height = stVoChnAttr.stRect.u32Height / 4 * 4;//VO高度要4对齐
    //HI_MPI_VO_ChnHide(VoDev, VoChn), "HI_MPI_VO_ChnHide");
    HI_MPI_VO_SetChnAttr(VoDev, VoChn, &stVoChnAttr);

    VI_UnBindViVo(VoDev, VoChn);
    VI_BindViVo(ViChn, VoDev, VoChn);

    return HI_SUCCESS;
}

HI_S32 VI_StartBayerData(VI_DEV_ATTR_S *pstDevAttr, VI_DEV_ATTR_S *pstBayerDevAttr,
    VI_CHN_ATTR_S *pstChnAttr)
{
    HI_S32 s32Ret = 0;
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;

    s32Ret = HI_MPI_VI_SetDevAttr(ViDev, pstDevAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetDevAttr failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableDev failed!\n");
        return s32Ret;
    }

    VI_IspInit();

    VI_StartVo1Pic(s_VoDev);
    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, pstChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetChnAttr failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableChn failed!\n");
        return s32Ret;
    }

    VI_PreviewOnVo(s_VoChn);

    printf("Waiting isp auto adjust,press any key to stop adjust!\n");
    getchar();

    //HI_MPI_ISP_Exit();
    //pthread_join(isp_pid, 0);

    s32Ret = HI_MPI_VI_DisableChn(ViChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableChn failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_DisableDev(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableDev failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_SetDevAttr(ViDev, pstBayerDevAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetDevAttr failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableDev(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableDev failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_SetChnAttr(ViChn, pstChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_SetChnAttr failed!\n");
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableChn(ViChn);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableChn failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}


HI_S32 main(int argc, char *argv[])
{
    HI_S32 s32Ret = 0;
    HI_U32 u32Nbit = 8;
    HI_U32 u32FrmCnt = 1;

    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    if(argc < 2)
    {
        printf("usage: ./vi_bayerdump [nbit] [frmcnt]. sample: ./vi_dump 12 1\n");
        printf("[nbit]----------Raw data:8bit;10bit;12bit\r\n");
        printf("[frmcnt]----------the frame number \r\n\n\n");

        return s32Ret;
    }

    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    s32Ret = VI_InitMpp();
    if (HI_SUCCESS != s32Ret)
    {
        printf("Init Mpp failed!\n");
        return s32Ret;
    }

    if (argc > 1)
    {
        u32Nbit = atoi(argv[1]);    /* nbit of Raw data:8bit;10bit;12bit */
    }
    if (argc > 2)
    {
        u32FrmCnt = atoi(argv[2]);/* the frame number */
    }


    s32Ret = VI_StartBayerData(&DEV_ATTR, &DEV_BAYER_ATTR, &CHN_BAYER_ATTR);
    if (HI_SUCCESS != s32Ret)
    {
        printf("VI_StartBayerData failed!\n");
        return s32Ret;
    }

    s32Ret = VI_DumpBayer(0, u32Nbit, u32FrmCnt);
    if (HI_SUCCESS != s32Ret)
    {
        printf("VI_StartBayerData failed!\n");
        return s32Ret;
    }
    sleep(2);
    VI_StopIsp();
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();

    return HI_SUCCESS;
}



