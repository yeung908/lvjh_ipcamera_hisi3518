/******************************************************************************
  A simple program of Hisilicon HI3516 vda implementation.
  the flow as follows:
    1) init mpp system.
    2) start vi( internal isp, ViDev 0, vichn0) and vo (HD)                  
    3) vda md & od start & print information
    4) stop vi vo and system.
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

#include "sample_comm.h"
#include "loadbmp.h"

VIDEO_NORM_E gs_enNorm = VIDEO_ENCODING_MODE_PAL;
VO_INTF_TYPE_E  g_enVoIntfType = VO_INTF_CVBS;

SAMPLE_VI_CONFIG_S g_stViChnConfig = 
{
    APTINA_AR0130_DC_720P_30FPS,
    VIDEO_ENCODING_MODE_AUTO,    
    ROTATE_NONE,
    VI_CHN_SET_NORMAL
};
HI_U32    gs_u32ViFrmRate = 0;

/******************************************************************************
* function : to process abnormal case                                        
******************************************************************************/
void SAMPLE_VDA_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}
/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VDA_Usage(char *sPrgNm)
{
    printf("Usage : %s <index>\n", sPrgNm);
    printf("index:\n");
    printf("\t 0) MD.(QVGA)\n");
    printf("\t 1) OD.(QVGA)\n");
    return;
}

/******************************************************************************
* function      : main() 
* Description : Vi/VO + VDA(MD&OD)
*               DC -> VI-PortA ViChn0(1080p) 
*                              ViChn1(VGA)    -> VdaChn0 MD
*                                            	  -> VdaChn1 OD
*								 ->VO (CVBS D1)
******************************************************************************/
HI_S32 SAMPLE_Vda_QVGA(HI_BOOL bVdaMd)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VI_CHN ViChn_Md , ViChn_Od ;
    VDA_CHN VdaChn_Md = 0, VdaChn_Od = 1;
    VB_CONF_S stVbConf ={0};	/* vb config define */
    PIC_SIZE_E enSize_Md = PIC_QVGA, enSize_Od = PIC_QVGA; 	/* vda picture size */
    
    VI_CHN ViExtChn = 1;
    VI_EXT_CHN_ATTR_S stViExtChnAttr;
    VO_DEV VoDev;
    VO_CHN VoChn;
    VO_PUB_ATTR_S stVoPubAttr; 
    SAMPLE_VO_MODE_E enVoMode;    

    HI_U32 u32BlkSize;
    SIZE_S stSize;
    PIC_SIZE_E enPicSize = PIC_HD720;    

    /******************************************
     step  1: init global  variable 
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

    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt  =  10;

    u32BlkSize = SAMPLE_COMM_SYS_CalcPicVbBlkSize(gs_enNorm,\
                PIC_QVGA, SAMPLE_PIXEL_FORMAT, SAMPLE_SYS_ALIGN_WIDTH);
    stVbConf.astCommPool[1].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[1].u32BlkCnt  = 10;

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

    stViExtChnAttr.s32BindChn           = 0;
    stViExtChnAttr.stDestSize.u32Width  = 320;
    stViExtChnAttr.stDestSize.u32Height = 240;
    stViExtChnAttr.s32SrcFrameRate      = 30;
    stViExtChnAttr.s32FrameRate         = 30;
    stViExtChnAttr.enPixFormat          = SAMPLE_PIXEL_FORMAT;

    s32Ret = HI_MPI_VI_SetExtChnAttr(ViExtChn, &stViExtChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set vi  extchn failed!\n");
        goto END_0;
    }        

    s32Ret = HI_MPI_VI_EnableChn(ViExtChn);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("set vi  extchn failed!\n");
        goto END_0;
    } 

    /******************************************
         step 4: start VO to preview
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
        goto END_1;
    }
    
    s32Ret = SAMPLE_COMM_VO_StartChn(VoDev, &stVoPubAttr, enVoMode);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn failed!\n");
        goto END_1;
    }

    s32Ret = SAMPLE_COMM_VO_BindVi(VoDev,VoChn,1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_BindVi failed!\n");
        goto END_1;
    }


    /******************************************
     step  5: VDA process
    ******************************************/
    sleep(2);
    
    if(HI_TRUE == bVdaMd)
    {
	    s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize_Md, &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
	        goto END_2;
	    }
	    ViChn_Md = ViExtChn;
	    s32Ret = SAMPLE_COMM_VDA_MdStart(VdaChn_Md, ViChn_Md, &stSize);
	    if (HI_SUCCESS != s32Ret)
	    {
	        SAMPLE_PRT("VDA Md Start failed!\n");
	        goto END_2;
	    }

        printf("Press any key to stop!\n");
    	getchar();   
        SAMPLE_COMM_VDA_MdStop(VdaChn_Md, ViChn_Md);     
    }
    else
    {
		s32Ret = SAMPLE_COMM_SYS_GetPicSize(gs_enNorm, enSize_Od, &stSize);
		if (HI_SUCCESS != s32Ret)
		{
		    SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
		    goto END_2;
		}
		ViChn_Od = ViExtChn;
		s32Ret = SAMPLE_COMM_VDA_OdStart(VdaChn_Od, ViChn_Od, &stSize);
		if (HI_SUCCESS != s32Ret)
		{
		    SAMPLE_PRT("VDA OD Start failed!\n");
		    goto END_2;
		}
        
       	printf("Press any key to stop!\n");
    	getchar(); 
        SAMPLE_COMM_VDA_OdStop(VdaChn_Od, ViChn_Od);
    } 

    /******************************************
     step 6: exit process
    ******************************************/

END_2:    // vo unbind and stop  
    SAMPLE_COMM_VO_UnBindVi(VoDev,VoChn);
    SAMPLE_COMM_VO_StopChn(VoDev, enVoMode);
    SAMPLE_COMM_VO_StopDevLayer(VoDev);
END_1:    // vi stop
    HI_MPI_VI_DisableChn(ViExtChn);
    SAMPLE_COMM_VI_StopVi(&g_stViChnConfig);
END_0:    // system exit
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
        SAMPLE_VDA_Usage(argv[0]);
        return HI_FAILURE;
    }

    signal(SIGINT, SAMPLE_VDA_HandleSig);
    signal(SIGTERM, SAMPLE_VDA_HandleSig);

    switch (*argv[1])
    {
        case '0': /* QVGA MD  */
            s32Ret = SAMPLE_Vda_QVGA(HI_TRUE);
            break;
        case '1': /* QVGA OD  */
            s32Ret = SAMPLE_Vda_QVGA(HI_FALSE);
            break;    
        default:
            SAMPLE_VDA_Usage(argv[0]);
            return HI_FAILURE;
    }

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
