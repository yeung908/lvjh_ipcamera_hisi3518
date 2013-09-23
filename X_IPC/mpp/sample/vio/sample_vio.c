/******************************************************************************
  A simple program of Hisilicon HI3531 video input and output implementation.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-8 Created
******************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"


VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
VO_INTF_TYPE_E g_enVoIntfType = VO_INTF_CVBS;
PIC_SIZE_E g_enPicSize = PIC_HD720;

SAMPLE_VIDEO_LOSS_S gs_stVideoLoss;
HI_U32 gs_u32ViFrmRate = 0;

SAMPLE_VI_CONFIG_S g_stViChnConfig =
{
    APTINA_AR0130_DC_720P_30FPS,
    VIDEO_ENCODING_MODE_AUTO,

    ROTATE_NONE,
    VI_CHN_SET_NORMAL
};

#define VI_PAUSE()  do {\
                         printf("---------------press any key to exit!---------------\n");\
                         getchar();\
                     } while (0)

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VIO_Usage(char *sPrgNm)
{
    printf("Usage : %s <index> <intf>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) VI: DC(720P); VO: SD0(CVBS). Embeded isp, phychn channel preview.\n");
    printf("\t 1) VI: BT656(D1); VO:SD0(CVBS). Isp bypass, phychn channel preview.\n");
    printf("\t 2) VI: DC(720P); VO: SD0(CVBS). Embeded isp, one ExtChn preview\n");
    printf("\t 3) VI: DC(720P); VO: SD0(CVBS). Embeded isp, rotate, phychn channel preview.\n");
    printf("\t 4) VI: DC(720P); VO: SD0(CVBS). Embeded isp, LDC, phychn channel preview.\n");
    printf("intf:\n");
    printf("\t 0) vo cvbs output, default.\n");
    printf("\t 1) vo BT1120 output.\n");

    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VIO_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}


/******************************************************************************
* function :  VI: DC(720P); VO: SD0(CVBS). Embeded isp, phychn channel preview.
******************************************************************************/
HI_S32 SAMPLE_VIO_720P_PreView(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_U32 u32ViChnCnt = 2;
    VB_CONF_S stVbConf;
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;;
    VO_CHN VoChn = 0;
    VI_CHN ViChn = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;
    PIC_SIZE_E enPicSize = g_enPicSize;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    ROTATE_E enRotate = ROTATE_NONE;

    /******************************************
     step  1: init global  variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)? 25: 30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));
    
    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enPicSize,
                            SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;

    /******************************************
     step 2: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_720P_1;
    }

    if(pstViConfig)
    {
        enRotate = pstViConfig->enRotate;
        if(ROTATE_90 == enRotate || ROTATE_270 == enRotate)
        {
            u32BlkSize = (CEILING_2_POWER(stSize.u32Width, SAMPLE_SYS_ALIGN_WIDTH) * \
                                CEILING_2_POWER(stSize.u32Height,SAMPLE_SYS_ALIGN_WIDTH) * \
                                   ((PIXEL_FORMAT_YUV_SEMIPLANAR_422 == SAMPLE_PIXEL_FORMAT) ? 2 : 1.5));
            stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
            stVbConf.astCommPool[1].u32BlkCnt = 8;
        }
    }

    /******************************************
     step 3: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_720P_0;
    }

    /******************************************
     step 4: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_720P_0;
    }

    /******************************************
    step 5: start VO SD0 (bind * vi )
    ******************************************/
    stVoPubAttr.enIntfType = g_enVoIntfType;
    if(VO_INTF_BT1120 == g_enVoIntfType)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
        gs_u32ViFrmRate = 50;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
    }
    stVoPubAttr.u32BgColor = 0x000000ff;

    /* In HD, this item should be set to HI_FALSE */
    stVoPubAttr.bDoubleFrame = HI_FALSE;
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
       goto END_720P_1;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
       goto END_720P_2;
    }

    s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ViChn);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_BindVi(vo:%d)-(vichn:%d) failed with %#x!\n", VoDev, VoChn, s32Ret);
       goto END_720P_3;
    }



    VI_PAUSE();
    /******************************************
     step 6: exit process
    ******************************************/
END_720P_3:
    SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
END_720P_2:
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_720P_1:
    SAMPLE_COMM_VI_StopVi(pstViConfig);
END_720P_0:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}

/******************************************************************************
* function :  VI: DC(720P); VO: SD0(CVBS). Embeded isp, phychn channel preview.
******************************************************************************/
HI_S32 SAMPLE_VIO_D1_PreView(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_U32 u32ViChnCnt = 1;
    VB_CONF_S stVbConf;
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;;
    VO_CHN VoChn = 0;
    VI_CHN ViChn = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;
    PIC_SIZE_E enPicSize = g_enPicSize;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;

    /******************************************
     step  1: init global  variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)? 25: 30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_D1,
                            SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_720P_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_720P_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_720P_1;
    }

    /******************************************
    step 5: start VO SD0 (bind * vi )
    ******************************************/
    stVoPubAttr.enIntfType = g_enVoIntfType;
    if(VO_INTF_BT1120 == g_enVoIntfType)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
    }
    stVoPubAttr.u32BgColor = 0x000000ff;
    /* In HD, this item should be set to HI_FALSE */
    stVoPubAttr.bDoubleFrame = HI_FALSE;
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
       goto END_720P_1;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
       goto END_720P_2;
    }

    s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ViChn);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_BindVi(vo:%d)-(vichn:%d) failed with %#x!\n", VoDev, VoChn, s32Ret);
       goto END_720P_3;
    }

    VI_PAUSE();
    /******************************************
     step 6: exit process
    ******************************************/
END_720P_3:
    SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
END_720P_2:
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_720P_1:
    SAMPLE_COMM_VI_StopVi(pstViConfig);
END_720P_0:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


/******************************************************************************
* function :  VI: DC(720P); VO: SD0(CVBS). Embeded isp, one ExtChn preview.
******************************************************************************/
HI_S32 SAMPLE_VIO_720P_Extchn_Preview(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_U32 u32ViChnCnt = 2;
    VB_CONF_S stVbConf;
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;;
    VO_CHN VoChn = 0;
    VI_CHN ViChn = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    VI_EXT_CHN_ATTR_S stExtChnAttr;
    VI_CHN ExtChn = 1;
    PIC_SIZE_E enPicSize = g_enPicSize;

    /******************************************
     step  1: init global  variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)? 25: 30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enPicSize,
                            SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, PIC_D1,
                            SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 8;


    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_720P_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_720P_0;
    }

    stExtChnAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    stExtChnAttr.s32BindChn = ViChn;
    stExtChnAttr.stDestSize.u32Width = 720;
    stExtChnAttr.stDestSize.u32Height = 576;
    stExtChnAttr.s32FrameRate = -1;
    stExtChnAttr.s32SrcFrameRate = -1;

    s32Ret = HI_MPI_VI_SetExtChnAttr(ExtChn, &stExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetExtChnAttr failed!\n");
        goto END_720P_0;
    }
    s32Ret = HI_MPI_VI_EnableChn(ExtChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_EnableChn failed!\n");
        goto END_720P_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_720P_1;
    }

    /******************************************
    step 5: start VO SD0 (bind * vi )
    ******************************************/
    stVoPubAttr.enIntfType = g_enVoIntfType;
    if(VO_INTF_BT1120 == g_enVoIntfType)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
    }
    stVoPubAttr.u32BgColor = 0x000000ff;
    /* In HD, this item should be set to HI_FALSE */
    stVoPubAttr.bDoubleFrame = HI_FALSE;
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
       goto END_720P_1;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
       goto END_720P_2;
    }

    s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ExtChn);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_BindVi(vo:%d)-(vichn:%d) failed with %#x!\n", VoDev, VoChn, s32Ret);
       goto END_720P_3;
    }

    VI_PAUSE();
    /******************************************
     step 6: exit process
    ******************************************/
END_720P_3:
    SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
END_720P_2:
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_720P_1:
    s32Ret = HI_MPI_VI_DisableChn(ExtChn);
    SAMPLE_COMM_VI_StopVi(pstViConfig);
END_720P_0:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


/******************************************************************************
* function :  VI: DC(720P); VO: SD0(CVBS). Embeded isp, one ExtChn preview.
******************************************************************************/
HI_S32 SAMPLE_VIO_720P_LDC_Preview(SAMPLE_VI_CONFIG_S* pstViConfig)
{
    HI_U32 u32ViChnCnt = 3;
    VB_CONF_S stVbConf;
    VO_DEV VoDev = SAMPLE_VO_DEV_DSD0;;
    VO_CHN VoChn = 0;
    VI_CHN ViChn = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode = VO_MODE_1MUX;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;
    VI_LDC_ATTR_S stLDCAttr;
    PIC_SIZE_E enPicSize = g_enPicSize;

    /******************************************
     step  1: init global  variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)? 25: 30;
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm, enPicSize,
                            SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*ddr0 video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = u32ViChnCnt * 8;

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_720P_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_720P_0;
    }

    stLDCAttr.bEnable = HI_TRUE;
    stLDCAttr.stAttr.enViewType = LDC_VIEW_TYPE_CROP;   //LDC_VIEW_TYPE_CROP; LDC_VIEW_TYPE_ALL
    stLDCAttr.stAttr.s32CenterXOffset = 0;
    stLDCAttr.stAttr.s32CenterYOffset = 0;
    stLDCAttr.stAttr.s32Ratio = 255;

    HI_MPI_VI_SetLDCAttr(ViChn, &stLDCAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VI_SetLDCAttr failed!\n");
        goto END_720P_1;
    }

    /******************************************
     step 4: start vpss and vi bind vpss (subchn needn't bind vpss in this mode)
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_720P_1;
    }

    /******************************************
    step 5: start VO SD0 (bind * vi )
    ******************************************/
    stVoPubAttr.enIntfType = g_enVoIntfType;
    if(VO_INTF_BT1120 == g_enVoIntfType)
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_720P50;
    }
    else
    {
        stVoPubAttr.enIntfSync = VO_OUTPUT_PAL;
    }
    stVoPubAttr.u32BgColor = 0x000000ff;
    /* In HD, this item should be set to HI_FALSE */
    stVoPubAttr.bDoubleFrame = HI_FALSE;
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
       goto END_720P_1;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
       goto END_720P_2;
    }

    s32Ret = SAMPLE_COMM_VO_BindVi(VoDev, VoChn, ViChn);
    if (HI_SUCCESS != s32Ret)
    {
       SAMPLE_PRT("SAMPLE_COMM_VO_BindVi(vo:%d)-(vichn:%d) failed with %#x!\n", VoDev, VoChn, s32Ret);
       goto END_720P_3;
    }

    printf("press 'q' key to exit, other key to disable LDC!");;
    while(1)
    {
        char ch = getchar();
        char ch1 = getchar();

        if('q' == ch || 'q' == ch1)
            break;

        stLDCAttr.bEnable = !stLDCAttr.bEnable;
        HI_MPI_VI_SetLDCAttr(ViChn, &stLDCAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_VI_SetLDCAttr failed!\n");
            break;
        }

        if(stLDCAttr.bEnable)
        {
            printf("press 'q' key to exit, other key to disable LDC!\n");
        }
        else
        {
            printf("press 'q' key to exit, other key to enable LDC!\n");
        }
    }
    /******************************************
     step 6: exit process
    ******************************************/
END_720P_3:
    SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
END_720P_2:
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_720P_1:
    s32Ret = HI_MPI_VI_DisableChn(ViChn);
    SAMPLE_COMM_VI_StopVi(pstViConfig);
END_720P_0:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
}


/******************************************************************************
* function    : main()
* Description : video preview sample
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret;

    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_VIO_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_VIO_HandleSig);
    signal(SIGTERM, SAMPLE_VIO_HandleSig);

    if((argc > 2) && *argv[2] == '1')   /* '1': VO_INTF_CVBS, else: BT1120 */
    {
        g_enVoIntfType = VO_INTF_BT1120;
    }

    g_stViChnConfig.enViMode = SENSOR_TYPE;

    if(SONY_IMX122_DC_1080P_30FPS == SENSOR_TYPE)
    {
        g_enPicSize = PIC_HD1080;
    }

    switch (*argv[1])
    {
        /* VI: DC(720P); VO: SD0(CVBS). Embeded isp, phychn channel preview. */
        case '0':
            s32Ret = SAMPLE_VIO_720P_PreView(&g_stViChnConfig);
            break;

        /* VI: BT656(D1); VO:HD0(CVBS ).  Isp bypass, phychn channel preview. */
        case '1':
            g_stViChnConfig.enViMode = SAMPLE_VI_MODE_1_D1;
            g_stViChnConfig.enNorm = VIDEO_ENCODING_MODE_PAL;
            s32Ret = SAMPLE_VIO_D1_PreView(&g_stViChnConfig);
            break;

        /* VI: DC(720P); VO: SD0(CVBS). Embeded isp, one ExtChn preview */
        case '2':
            s32Ret = SAMPLE_VIO_720P_Extchn_Preview(&g_stViChnConfig);
            break;

        /* VI: DC(720P); VO: SD0(CVBS). Embeded isp, rotate, phychn channel  preview. */
        case '3':
            g_stViChnConfig.enRotate = ROTATE_90;
            s32Ret = SAMPLE_VIO_720P_PreView(&g_stViChnConfig);
            break;

        /* VI: DC(720P); VO: SD0(CVBS). Embeded isp,  LDC, phychn channel  preview. */
        case '4':
            s32Ret = SAMPLE_VIO_720P_LDC_Preview(&g_stViChnConfig);
            break;

        default:
            SAMPLE_PRT("the index is invaild!\n");
            SAMPLE_VIO_Usage(argv[0]);
            return HI_FAILURE;
    }

    if (HI_SUCCESS == s32Ret)
        SAMPLE_PRT("program exit normally!\n");
    else
        SAMPLE_PRT("program exit abnormally!\n");
    exit(s32Ret);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

