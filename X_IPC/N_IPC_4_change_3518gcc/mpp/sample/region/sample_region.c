/******************************************************************************
  A simple program of Hisilicon HI3531 osd implementation.
  the flow as follows:
    1) init mpp system.
    2) start vi ( internal isp, ViDev 0, 2 vichn)
    3) start venc
    4) osd process, you can see video from some H264 streams files. the video will show as follows step:
        4.1) create some cover/osd regions
        4.2) display  cover/osd regions ( One Region -- Multi-VencGroup )
        4.3) change all vencGroups Regions' Layer
        4.4) change all vencGroups Regions' position
        4.5) change all vencGroups Regions' color
        4.6) change all vencGroups Regions' alpha (front and backgroud)
        4.7) load bmp form bmp-file to Region-0
        4.8) change BmpRegion-0
    6) stop venc
    7) stop vi and system.
  Copyright (C), 2010-2011, Hisilicon Tech. Co., Ltd.
 ******************************************************************************
    Modification:  2011-2 Created
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
#include "loadbmp.h"

#include "sample_comm.h"

HI_BOOL g_bToRun = HI_FALSE;

VI_SCAN_MODE_E    gs_enViScanMode = VI_SCAN_INTERLACED;

static HI_U32 gs_s32ChnCnt = 1;        /* vi, venc chn count */
static HI_S32 gs_s32RgnCntCur = 0;
static HI_S32 gs_s32RgnCnt = 5;
VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
VO_INTF_TYPE_E  g_enVoIntfType = VO_INTF_CVBS;


#define SAMPLE_RGN_SLEEP_TIME (200*1000)
#define SAMPLE_RGN_LOOP_COUNT 6
HI_U32    gs_u32ViFrmRate = 0;

SAMPLE_VI_CONFIG_S g_stViChnConfig =
{
    APTINA_AR0130_DC_720P_30FPS,
    VIDEO_ENCODING_MODE_AUTO,

    ROTATE_NONE,
    VI_CHN_SET_NORMAL
};

/******************************************************************************
* function : Set region memory location
******************************************************************************/
HI_S32 SAMPLE_RGN_MemConfig(HI_VOID)
{
    HI_S32 i = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    HI_CHAR * pcMmzName;
    MPP_CHN_S stMppChnRGN;

    /*the max chn of vpss,grp and venc is 64*/
    for(i=0; i<RGN_HANDLE_MAX; i++)
    {
        stMppChnRGN.enModId  = HI_ID_RGN;
        stMppChnRGN.s32DevId = 0;
        stMppChnRGN.s32ChnId = 0;

        if(0 == (i%2))
        {
            pcMmzName = NULL;
        }
        else
        {
            pcMmzName = "ddr1";
        }

        s32Ret = HI_MPI_SYS_SetMemConf(&stMppChnRGN,pcMmzName);
        if (s32Ret)
        {
            SAMPLE_PRT("HI_MPI_SYS_SetMemConf ERR !\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}

/******************************************************************************
* funciton : osd region change position
******************************************************************************/
HI_S32 SAMPLE_RGN_ChgPosition(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, POINT_S *pstPoint)
{
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = 0;
    stChn.s32ChnId = VencGrp;

    if (NULL == pstPoint)
    {
        SAMPLE_PRT("input parameter is null. it is invaild!\n");
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = pstPoint->s32X;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = pstPoint->s32Y;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* funciton : osd region show or hide
******************************************************************************/
HI_S32 SAMPLE_RGN_ShowOrHide(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, HI_BOOL bShow)
{
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = 0;
    stChn.s32ChnId = VencGrp;

    s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle, &stChn, &stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    stChnAttr.bShow = bShow;

    s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* funciton : osd region change color
******************************************************************************/
HI_S32 SAMPLE_RGN_ChgColor(RGN_HANDLE RgnHandle, HI_U32 u32Color)
{
    RGN_ATTR_S stRgnAttr;
    HI_S32 s32Ret;

    s32Ret = HI_MPI_RGN_GetAttr(RgnHandle,&stRgnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_GetAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    stRgnAttr.unAttr.stOverlay.u32BgColor = u32Color;

    s32Ret = HI_MPI_RGN_SetAttr(RgnHandle,&stRgnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}
/******************************************************************************
* funciton : osd region change (bgAlpha, fgAlpha, layer)
******************************************************************************/
HI_S32 SAMPLE_RGN_Change(RGN_HANDLE RgnHandle, VENC_GRP VencGrp, SAMPLE_RGN_CHANGE_TYPE_EN enChangeType, HI_U32 u32Val)
{
    MPP_CHN_S stChn;
    RGN_CHN_ATTR_S stChnAttr;
    HI_S32 s32Ret;

    stChn.enModId = HI_ID_GROUP;
    stChn.s32DevId = 0;
    stChn.s32ChnId = VencGrp;
    s32Ret = HI_MPI_RGN_GetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_GetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    switch (enChangeType)
    {
        case RGN_CHANGE_TYPE_FGALPHA:
            stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = u32Val;
            break;
        case RGN_CHANGE_TYPE_BGALPHA:
            stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = u32Val;
            break;
        case RGN_CHANGE_TYPE_LAYER:
            stChnAttr.unChnAttr.stOverlayChn.u32Layer = u32Val;
            break;
        default:
            SAMPLE_PRT("input paramter invaild!\n");
            return HI_FAILURE;
    }
    s32Ret = HI_MPI_RGN_SetDisplayAttr(RgnHandle,&stChn,&stChnAttr);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetDisplayAttr (%d)) failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

/******************************************************************************
* funciton : load bmp from file
******************************************************************************/
HI_S32 SAMPLE_RGN_LoadBmp(const HI_CHAR *filename, BITMAP_S *pstBitmap)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
        SAMPLE_PRT("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    pstBitmap->pData = malloc(2*(bmpInfo.bmiHeader.biWidth)*(bmpInfo.bmiHeader.biHeight));

    if(NULL == pstBitmap->pData)
    {
        SAMPLE_PRT("malloc osd memroy err!\n");
        return HI_FAILURE;
    }

    CreateSurfaceByBitMap(filename,&Surface,(HI_U8*)(pstBitmap->pData));

    pstBitmap->u32Width = Surface.u16Width;
    pstBitmap->u32Height = Surface.u16Height;
    pstBitmap->enPixelFormat = PIXEL_FORMAT_RGB_1555;
    return HI_SUCCESS;
}

#define START_POINT_X_OFFSET 64
#define START_POINT_Y_OFFSET 64

/******************************************************************************
  function : overlay process
             1) create some overlay regions
             2) display overlay regions ( One Region -- Multi-VencGroup )
             3) change all vencGroups Regions' positio
             4) change all vencGroups Regions' Layern
             5) change all vencGroups Regions' color
             6) load bmp form bmp-file to Region-0
             7) change all vencGroups Regions' front alpha
             8) change all vencGroups Regions' backgroud alpha
             9) update bitmap(not support now)
             10) show or hide overlay regions
             11) Detach overlay regions from chn
             12) Detroy overlay regions
******************************************************************************/
HI_S32 SAMPLE_RGN_OverlayProcess(VENC_GRP VencGrpStart,HI_S32 grpcnt)
{
    HI_S32 i, j;
    HI_S32 s32Ret = HI_FAILURE;
    RGN_HANDLE RgnHandle;
    RGN_ATTR_S stRgnAttr;
    MPP_CHN_S stChn;
    VENC_GRP VencGrp;
    RGN_CHN_ATTR_S stChnAttr;
    HI_U32 u32Layer;
    HI_U32 u32Color;
    HI_U32 u32Alpha;
    POINT_S stPoint;
    BITMAP_S stBitmap;
    SAMPLE_RGN_CHANGE_TYPE_EN enChangeType;
    HI_BOOL bShow = HI_FALSE;

    /****************************************
     step 1: create overlay regions
    ****************************************/
    for (i = 0; i < gs_s32RgnCnt; i++)
    {
        stRgnAttr.enType = OVERLAY_RGN;
        stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
        stRgnAttr.unAttr.stOverlay.stSize.u32Width  = 180;
        stRgnAttr.unAttr.stOverlay.stSize.u32Height = 144;
        stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00*(i%2) + ((i+1)%2)*0x1f;

        RgnHandle = i;

        s32Ret = HI_MPI_RGN_Create(RgnHandle, &stRgnAttr);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Create (%d) failed with %#x!\n", \
                   RgnHandle, s32Ret);
            return HI_FAILURE;
        }
        gs_s32RgnCntCur ++;
        SAMPLE_PRT("the handle:%d,creat success!\n",RgnHandle);
    }

    /*********************************************
     step 2: display overlay regions to venc groups
    *********************************************/
    for (i = 0; i < gs_s32RgnCnt; i++)
    {
        RgnHandle = i;

        for (j = 0; j < gs_s32ChnCnt; j++)
        {
            VencGrp = j + VencGrpStart;
            stChn.enModId = HI_ID_GROUP;
            stChn.s32DevId = 0;
            stChn.s32ChnId = VencGrp;

            memset(&stChnAttr,0,sizeof(stChnAttr));
            stChnAttr.bShow = HI_TRUE;
            stChnAttr.enType = OVERLAY_RGN;
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X =(i%3) * 192 + START_POINT_X_OFFSET;
            stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y =(i/3)*160 + START_POINT_Y_OFFSET;
            stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
            stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
            stChnAttr.unChnAttr.stOverlayChn.u32Layer = i;

            stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
            stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;

            s32Ret = HI_MPI_RGN_AttachToChn(RgnHandle, &stChn, &stChnAttr);
            if(HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_RGN_AttachToChn (%d) failed with %#x!\n",\
                       RgnHandle, s32Ret);
                return HI_FAILURE;
            }
        }
    }

    usleep(SAMPLE_RGN_SLEEP_TIME*5);
    printf("display region to chn success!\n");

    /*********************************************
     step 3: change overlay regions' position
    *********************************************/
    RgnHandle = 1;

    for (i = 0; i < grpcnt; i++)
    {
        VencGrp = i + VencGrpStart;
        stPoint.s32X = 60 + START_POINT_X_OFFSET;
        stPoint.s32Y = 0 + START_POINT_Y_OFFSET;
        s32Ret = SAMPLE_RGN_ChgPosition(RgnHandle, VencGrp, &stPoint);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("change region(%d) position failed with %#x!\n",\
                   RgnHandle, s32Ret);
            return HI_FAILURE;
        }
    }

    printf("handle:%d,change point success,new point(x:%d,y:%d) !\n",
           RgnHandle,stPoint.s32X,stPoint.s32Y);
    usleep(SAMPLE_RGN_SLEEP_TIME*5);

    /*********************************************
     step 4: change layer
    *********************************************/
    RgnHandle = 0;
    enChangeType = RGN_CHANGE_TYPE_LAYER;

    for (i=0; i<grpcnt; i++)
    {
        VencGrp = i+VencGrpStart;
        u32Layer = 2;
        s32Ret = SAMPLE_RGN_Change(RgnHandle, VencGrp, enChangeType, u32Layer);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("change region(%d) layer failed with %#x!\n",\
                   RgnHandle, s32Ret);
            return HI_FAILURE;
        }
    }

    printf("handle:%d,change layer success,new layer(%d) !\n",RgnHandle,u32Layer);

    usleep(SAMPLE_RGN_SLEEP_TIME*5);

    /*********************************************
     step 5: change color
    *********************************************/
    RgnHandle = 2;

    u32Color = 0x7fff;
    s32Ret = SAMPLE_RGN_ChgColor(RgnHandle, u32Color);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("change region(%d) color failed with %#x!\n",\
               RgnHandle, s32Ret);
        return HI_FAILURE;
    }

    printf("handle:%d,change color success,new bg color(0x%x)\n",RgnHandle,u32Color);

    usleep(SAMPLE_RGN_SLEEP_TIME*5);

    /*********************************************
     step 6: show bitmap
    *********************************************/
    RgnHandle = 0;

    s32Ret = SAMPLE_RGN_LoadBmp("mm.bmp", &stBitmap);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("load bmp failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_RGN_SetBitMap(RgnHandle,&stBitmap);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_RGN_SetBitMap failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    if (NULL != stBitmap.pData)
    {
        free(stBitmap.pData);
        stBitmap.pData = NULL;
    }

    usleep(SAMPLE_RGN_SLEEP_TIME*5);
    printf("handle:%d,load bmp success!\n",RgnHandle);

    /*********************************************
     step 7: change front alpha
    *********************************************/
    RgnHandle = 0;
    enChangeType = RGN_CHANGE_TYPE_FGALPHA;

    for (i=0; i<grpcnt; i++)
    {
        VencGrp = i+VencGrpStart;
        u32Alpha = 32;
        s32Ret = SAMPLE_RGN_Change(RgnHandle, VencGrp, enChangeType, u32Alpha);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("change region(%d) f-alpha failed with %#x!\n",\
                   RgnHandle, s32Ret);
            return HI_FAILURE;
        }
    }

    printf("handle:%d,change front alpha success,the new alpha:%d\n", RgnHandle,u32Alpha);

    usleep(SAMPLE_RGN_SLEEP_TIME*5);

    /*********************************************
     step 8: change backgroud alpha
    *********************************************/
    RgnHandle = 0;
    enChangeType = RGN_CHANGE_TYPE_BGALPHA;

    for (i = 0; i < grpcnt; i++)
    {
        VencGrp = i + VencGrpStart;
        u32Alpha = 32;
        s32Ret = SAMPLE_RGN_Change(RgnHandle, VencGrp, enChangeType, u32Alpha);
        if(HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("change region(%d) f-alpha failed with %#x!\n",\
                   RgnHandle, s32Ret);
            return HI_FAILURE;
        }
    }

    printf("handle:%d,change backgroud alpha success,the new alpha:%d\n", RgnHandle,u32Alpha);

    usleep(SAMPLE_RGN_SLEEP_TIME*5);

    /*********************************************
     step 9: update bitmap
    *********************************************/
    /*not support now*/

    /*********************************************
     step 10: show or hide overlay regions
    *********************************************/
    RgnHandle = 4;
    bShow = HI_FALSE;

    for (i = 0; i < SAMPLE_RGN_LOOP_COUNT; i++)
    {
        for (j = 0; j < grpcnt; j++)
        {
            VencGrp = j + VencGrpStart;

            s32Ret = SAMPLE_RGN_ShowOrHide(RgnHandle, VencGrp, bShow);
            if(HI_SUCCESS != s32Ret)
            {
                printf("region(%d) show failed with %#x!\n",\
                       RgnHandle, s32Ret);
                return HI_FAILURE;
            }
        }

        bShow = !bShow;

        usleep(SAMPLE_RGN_SLEEP_TIME*5);
    }

    printf("handle:%d,show or hide osd success\n", RgnHandle);

    /*********************************************
     step 11: Detach osd from chn
    *********************************************/
    for (i = 0; i < gs_s32RgnCnt; i++)
    {
        RgnHandle = i;

        for (j = 0; j < grpcnt; j++)
        {
            VencGrp = j+VencGrpStart;
            stChn.enModId = HI_ID_GROUP;
            stChn.s32DevId = 0;
            stChn.s32ChnId = VencGrp;

            s32Ret = HI_MPI_RGN_DetachFrmChn(RgnHandle, &stChn);
            if(HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_RGN_DetachFrmChn (%d) failed with %#x!\n",\
                       RgnHandle, s32Ret);
                return HI_FAILURE;
            }
        }

         printf("Detach handle:%d from chn success\n", RgnHandle);

         usleep(SAMPLE_RGN_SLEEP_TIME*5);
    }

    /*********************************************
     step 12: destory region
    *********************************************/
    for (i = 0; i < gs_s32RgnCnt; i++)
    {
        RgnHandle = i;
        s32Ret = HI_MPI_RGN_Destroy(RgnHandle);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("HI_MPI_RGN_Destroy [%d] failed with %#x\n",\
                    RgnHandle, s32Ret);
        }
    }
    SAMPLE_PRT("destory all region success!\n");
    return HI_SUCCESS;
}


/******************************************************************************
  function : cover process
             1) enable vpp, but disable IE
             2) create two cover region and attach it to vi chn0
             3) change one cover's position,size, color and layer
             4) change one  layer
             5) hide the cover
             6) release resource
******************************************************************************/
HI_S32 SAMPLE_RGN_CoverProcess(VI_DEV ViDev,VI_CHN ViChn)
{
    HI_S32 s32Ret = HI_FAILURE;

    RGN_HANDLE coverHandle[2];
    RGN_ATTR_S stCoverAttr[2];
    MPP_CHN_S stCoverChn[2];
    RGN_CHN_ATTR_S stCoverChnAttr[2];

    /*******************************************************
     step 2: create two cover region and attach it to vi chn0
    ********************************************************/

    coverHandle[0] = 0;
    stCoverAttr[0].enType = COVER_RGN;
    s32Ret = HI_MPI_RGN_Create(coverHandle[0], &stCoverAttr[0]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    stCoverChn[0].enModId = HI_ID_VIU;
    stCoverChn[0].s32ChnId = ViChn;
    stCoverChn[0].s32DevId = ViDev;

    stCoverChnAttr[0].bShow = HI_TRUE;
    stCoverChnAttr[0].enType = COVER_RGN;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.s32X = 12;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.s32Y = 12;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.u32Width = 160;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.u32Height = 160;
    stCoverChnAttr[0].unChnAttr.stCoverChn.u32Color = 0xff;
    stCoverChnAttr[0].unChnAttr.stCoverChn.u32Layer = 0;
    s32Ret = HI_MPI_RGN_AttachToChn(coverHandle[0], &stCoverChn[0], &stCoverChnAttr[0]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
       goto AttachCover_failed_0;
    }

    coverHandle[1] = 1;
    stCoverAttr[1].enType = COVER_RGN;
    s32Ret = HI_MPI_RGN_Create(coverHandle[1], &stCoverAttr[1]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    stCoverChn[1].enModId = HI_ID_VIU;
    stCoverChn[1].s32ChnId = ViChn;
    stCoverChn[1].s32DevId = ViDev;

    stCoverChnAttr[1].bShow = HI_TRUE;
    stCoverChnAttr[1].enType = COVER_RGN;
    stCoverChnAttr[1].unChnAttr.stCoverChn.stRect.s32X = 64;
    stCoverChnAttr[1].unChnAttr.stCoverChn.stRect.s32Y = 64;
    stCoverChnAttr[1].unChnAttr.stCoverChn.stRect.u32Width = 160;
    stCoverChnAttr[1].unChnAttr.stCoverChn.stRect.u32Height = 160;
    stCoverChnAttr[1].unChnAttr.stCoverChn.u32Color = 0xffff;
    stCoverChnAttr[1].unChnAttr.stCoverChn.u32Layer = 1;
    s32Ret = HI_MPI_RGN_AttachToChn(coverHandle[1], &stCoverChn[1], &stCoverChnAttr[1]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
       goto AttachCover_failed_1;
    }
    //printf("create an cover region and attach it to vi chn0\n");
    if(HI_TRUE != g_bToRun)
    {
        goto exit;
    }
    printf("show two  cover: handle0, handle1 \n");
    usleep(SAMPLE_RGN_SLEEP_TIME*50);

    /**********************************************************
      step 3: change one cover's position, size, color and layer
     **********************************************************/
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.s32X = 128;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.s32Y = 128;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.u32Width = 260;
    stCoverChnAttr[0].unChnAttr.stCoverChn.stRect.u32Height = 260;
    stCoverChnAttr[0].unChnAttr.stCoverChn.u32Color = 0xff0000;
    stCoverChnAttr[0].unChnAttr.stCoverChn.u32Layer = 2;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(coverHandle[0], &stCoverChn[0], &stCoverChnAttr[0]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto exit;
    }

    if(HI_TRUE != g_bToRun)
    {
        goto exit;
    }
    printf("change handle0's position,size,color layer\n");
    usleep(SAMPLE_RGN_SLEEP_TIME*30);

    /**********************************************************
      step 4: change one cover's position, size, color and layer
     **********************************************************/
    stCoverChnAttr[1].unChnAttr.stCoverChn.u32Layer = 3;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(coverHandle[1], &stCoverChn[1], &stCoverChnAttr[1]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto exit;
    }

    if(HI_TRUE != g_bToRun)
    {
        goto exit;
    }
    printf("change handle1's layer\n");
    usleep(SAMPLE_RGN_SLEEP_TIME*30);
   /*********************************************
     step 5: hide the cover and the overlay
    *********************************************/
    stCoverChnAttr[0].bShow = HI_FALSE;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(coverHandle[0], &stCoverChn[0], &stCoverChnAttr[0]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto exit;
    }

    stCoverChnAttr[1].bShow = HI_FALSE;
    s32Ret = HI_MPI_RGN_SetDisplayAttr(coverHandle[1], &stCoverChn[1], &stCoverChnAttr[1]);
    if(HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto exit;
    }

    if(HI_TRUE != g_bToRun)
    {
        goto exit;
    }
    printf("hide handle0 and handle1\n");
    usleep(SAMPLE_RGN_SLEEP_TIME*30);

   /*********************************************
     step 6: release resource
    *********************************************/

exit:
    HI_MPI_RGN_DetachFrmChn(coverHandle[0], &stCoverChn[0]);
    HI_MPI_RGN_DetachFrmChn(coverHandle[1], &stCoverChn[1]);
AttachCover_failed_1:
    HI_MPI_RGN_Destroy(coverHandle[1]);
AttachCover_failed_0:
    HI_MPI_RGN_Destroy(coverHandle[0]);
    return s32Ret;
}

void SAMPLE_RGN_Usage(HI_CHAR *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) Cover  : Vi 720P  \n");
    printf("\t 1) Overlay: Venc(VGA) \n");
    return;
}


/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_RGN_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        HI_MPI_RGN_Destroy(gs_s32RgnCntCur);
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

HI_VOID *SAMPLE_RGN_VENC(HI_VOID)
{
    SAMPLE_VI_MODE_E enViMode = SENSOR_TYPE;

    HI_U32 u32ChnNum = 1;

    VB_CONF_S stVbConf;

    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    SAMPLE_RC_E enRcMode;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;

    VO_DEV VoDev;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;
    PIC_SIZE_E enPicSize = PIC_HD720;


    /******************************************
     step  1: init variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)?25:30;

    if (SONY_IMX122_DC_1080P_30FPS == SENSOR_TYPE)
    {
        enPicSize = PIC_HD1080;
    }

    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enPicSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt = 10;
    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_VGA, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt = 10;
    memset(stVbConf.astCommPool[1].acMmzName,0,
        sizeof(stVbConf.astCommPool[1].acMmzName));

    /* hist buf*/
    stVbConf.astCommPool[2].u32BlkSize = (196*4);
    stVbConf.astCommPool[2].u32BlkCnt = 10;
    memset(stVbConf.astCommPool[2].acMmzName,0,
        sizeof(stVbConf.astCommPool[2].acMmzName));

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(&g_stViChnConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_0;
    }

    VpssGrp = 0;
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_1;
    }

    VpssChn = 1;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.bFrameEn = HI_FALSE;
    stVpssChnAttr.bSpEn    = HI_TRUE;

    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = 640;
    stVpssChnMode.u32Height     = 480;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_2;
    }
    /******************************************
     step 5: start vo
    ******************************************/
    VoDev = SAMPLE_VO_DEV_DSD0;
    VoChn  = 0;
    enVoMode = VO_MODE_1MUX;

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
    stVoPubAttr.bDoubleFrame = HI_FALSE;
    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_3;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_4;
    }

    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
        goto END_4;
    }

    /******************************************
     step 6: start stream venc
    ******************************************/
    enRcMode = SAMPLE_RC_CBR;
    VencGrp = 0;
    VencChn = 0;

    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                gs_enNorm, PIC_VGA, enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_4;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_5;
    }

    /******************************************
     step 7: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_5;
    }

    /*Overlay Region Process*/
    s32Ret = SAMPLE_RGN_OverlayProcess(VencChn, u32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("overlay process failed!\n");
        goto END_5;
    }

    printf("please press any key to exit\n");
    while(HI_TRUE == g_bToRun)
    {
    	sleep(1);
    }

    /******************************************
     step 8: exit process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StopGetStream();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_StopGetStream failed!\n");
        goto END_5;
    }

END_5:    //  venc stop
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
END_4:    // vo stop
    SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VpssChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_3:    //vi unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_2:    //vpss stop
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_1:    //vi stop
    SAMPLE_COMM_VI_StopVi(&g_stViChnConfig);
END_0:    //system exit
    SAMPLE_COMM_SYS_Exit();
    return HI_NULL;

}


HI_VOID *SAMPLE_RGN_720P(HI_VOID*arg)
{
    SAMPLE_VI_MODE_E enViMode = SENSOR_TYPE;
    HI_U32 u32ChnNum = 1;
    VI_DEV ViDev = 0;
    VI_CHN ViChn = 0;

    VB_CONF_S stVbConf;
    VPSS_GRP VpssGrp;
    VPSS_CHN VpssChn;
    VPSS_GRP_ATTR_S stGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_CHN_MODE_S stVpssChnMode;

    VENC_GRP VencGrp;
    VENC_CHN VencChn;
    PAYLOAD_TYPE_E enPayLoad = PT_H264;
    SAMPLE_RC_E enRcMode;

    HI_S32 s32Ret = HI_SUCCESS;
    HI_U32 u32BlkSize;
    SIZE_S stSize;

    VO_DEV VoDev;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr;
    SAMPLE_VO_MODE_E enVoMode;

    PIC_SIZE_E enPicSize = PIC_HD720;

    /******************************************
     step  1: init variable
    ******************************************/
    gs_u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL == gs_enNorm)?25:30;

    memset(&stVbConf,0,sizeof(VB_CONF_S));

    if (SONY_IMX122_DC_1080P_30FPS == SENSOR_TYPE)
    {
        enPicSize = PIC_HD1080;
    }

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                enPicSize, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.u32MaxPoolCnt = 128;

    /*video buffer*/
    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  = 10;

    memset(stVbConf.astCommPool[0].acMmzName,0,
        sizeof(stVbConf.astCommPool[0].acMmzName));

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_VGA, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);

    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt  = 10;

    memset(stVbConf.astCommPool[1].acMmzName,0,
        sizeof(stVbConf.astCommPool[1].acMmzName));


    /* hist buf*/
    stVbConf.astCommPool[2].u32BlkSize = (196*4);
    stVbConf.astCommPool[2].u32BlkCnt  = 10;

    memset(stVbConf.astCommPool[2].acMmzName,0,
        sizeof(stVbConf.astCommPool[2].acMmzName));

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        goto END_0;
    }

    /******************************************
     step 3: start vi dev & chn to capture
    ******************************************/
    s32Ret = SAMPLE_COMM_VI_StartVi(&g_stViChnConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vi failed!\n");
        goto END_0;
    }

    /******************************************
     step 4: start vpss and vi bind vpss
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enPicSize, &stSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        goto END_0;
    }

    VpssGrp = 0;
    stGrpAttr.u32MaxW = stSize.u32Width;
    stGrpAttr.u32MaxH = stSize.u32Height;
    stGrpAttr.bDrEn = HI_FALSE;
    stGrpAttr.bDbEn = HI_FALSE;
    stGrpAttr.bIeEn = HI_TRUE;
    stGrpAttr.bNrEn = HI_TRUE;
    stGrpAttr.bHistEn = HI_TRUE;
    stGrpAttr.enDieMode = VPSS_DIE_MODE_AUTO;
    stGrpAttr.enPixFmt = SAMPLE_PIXEL_FORMAT;

    s32Ret = SAMPLE_COMM_VPSS_StartGroup(VpssGrp, &stGrpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Vpss failed!\n");
        goto END_1;
    }

    VpssChn = 1;
    memset(&stVpssChnAttr, 0, sizeof(stVpssChnAttr));
    stVpssChnAttr.bFrameEn = HI_FALSE;
    stVpssChnAttr.bSpEn    = HI_TRUE;
    stVpssChnMode.enChnMode     = VPSS_CHN_MODE_USER;
    stVpssChnMode.bDouble       = HI_FALSE;
    stVpssChnMode.enPixelFormat = SAMPLE_PIXEL_FORMAT;
    stVpssChnMode.u32Width      = 640;
    stVpssChnMode.u32Height     = 480;
    s32Ret = SAMPLE_COMM_VPSS_EnableChn(VpssGrp, VpssChn, &stVpssChnAttr, &stVpssChnMode, HI_NULL);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Enable vpss chn failed!\n");
        goto END_1;
    }

    s32Ret = SAMPLE_COMM_VI_BindVpss(enViMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Vi bind Vpss failed!\n");
        goto END_2;
    }

   /******************************************
      step 5: start vo
     ******************************************/
    VoDev = SAMPLE_VO_DEV_DSD0;
    VoChn = 0;
    enVoMode = VO_MODE_1MUX;

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
    stVoPubAttr.bDoubleFrame = HI_FALSE;

    s32Ret = SAMPLE_COMM_VO_StartDevLayer(VoDev, &stVoPubAttr, gs_u32ViFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartDevLayer failed!\n");
        goto END_3;
    }

    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_4;
    }


    s32Ret = SAMPLE_COMM_VO_BindVpss(VoDev,VoChn,VpssGrp,VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_BindVpss failed!\n");
        goto END_4;
    }

    /******************************************
     step 6: start stream venc
    ******************************************/
    enRcMode = SAMPLE_RC_CBR;
    VencGrp = 0;
    VencChn = 0;

    s32Ret = SAMPLE_COMM_VENC_Start(VencGrp, VencChn, enPayLoad,\
                                gs_enNorm, PIC_VGA, enRcMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_4;
    }

    s32Ret = SAMPLE_COMM_VENC_BindVpss(VencGrp, VpssGrp, VpssChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_5;
    }


    /******************************************
     step 7: stream venc process -- get stream, then save it to file.
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(u32ChnNum);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto END_5;
    }

    /******************************************
      step 6: start region
     ******************************************/
    printf("please press any key to exit\n");
	while(g_bToRun)
	{
		/*Cover Region Process*/
		s32Ret = SAMPLE_RGN_CoverProcess(ViDev,ViChn);
		if (HI_SUCCESS != s32Ret)
		{
			SAMPLE_PRT("cover process failed!\n");
			goto END_4;
		}
	}

    /******************************************
     step 8: exit process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StopGetStream();
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VENC_StopGetStream failed!\n");
        goto END_5;
    }


END_5:    //  venc stop
    SAMPLE_COMM_VENC_UnBindVpss(VencGrp, VpssGrp, VpssChn);
    SAMPLE_COMM_VENC_Stop(VencGrp,VencChn);
END_4:    // vo unbind vpss and stop
    SAMPLE_COMM_VO_UnBindVpss(VoDev,VoChn,VpssGrp,VpssChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_3:    //vi unbind vpss
    SAMPLE_COMM_VI_UnBindVpss(enViMode);
END_2:    //vpss stop
    SAMPLE_COMM_VPSS_DisableChn(VpssGrp, VpssChn);
    SAMPLE_COMM_VPSS_StopGroup(VpssGrp);
END_1:    //vi stop
    SAMPLE_COMM_VI_StopVi(&g_stViChnConfig);
END_0:    //system exit
    SAMPLE_COMM_SYS_Exit();

    return HI_NULL;

}

/******************************************************************************
* function    : main()
* Description : region
******************************************************************************/
int main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_S32 s32Tm = -1;
    pthread_t SampRgnThread;
    if ( (argc < 2) || (1 != strlen(argv[1])))
    {
        SAMPLE_RGN_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_RGN_HandleSig);
    signal(SIGTERM, SAMPLE_RGN_HandleSig);

    g_bToRun = HI_TRUE;

    switch (*argv[1])
    {
        case '0':/* VI: 720P ; Cover */
            pthread_create(&SampRgnThread, 0, SAMPLE_RGN_720P, HI_NULL);
            break;

        case '1':/* VI: VGA ;  Overlay(venc) */
            pthread_create(&SampRgnThread, 0, SAMPLE_RGN_VENC, HI_NULL);
            break;

        default:
            SAMPLE_RGN_Usage(argv[0]);
            return HI_FAILURE;
    }

    getchar();
    g_bToRun = HI_FALSE;
    printf("waitting  exit...\n");

    pthread_join(SampRgnThread, 0);


    if (HI_SUCCESS == s32Ret)
        printf("program exit normally!\n");
    else
        printf("program exit abnormally!\n");
    exit(s32Ret);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
