#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vpss.h"

#define USAGE_HELP(void)\
{\
    printf("\n\tusage : %s para value group [chn] \n", argv[0]);    \
    printf("\n\t para: \n");    \
    printf("\t\tenIE   [0, disable; 1,enable]\n");   \
    printf("\t\tenNR   [0, disable; 1,enable]\n");    \
    printf("\t\tenDEI  [0, disable; 1,enable]\n");   \
    printf("\t\tenHIST [0, disable; 1,enable]\n");    \
    printf("\t\tie     [IE强度，value:0~255, default:32]\n");   \
    printf("\t\tiesp   [IE锐度，value:0~7,  default:7]\n");   \
    printf("\t\tlum   [亮度，value:0~48, default:32]\n");   \
    printf("\t\tcon   [对比度，value:0~48, default:8]\n");   \
    printf("\t\tde    [暗区增强，value:0~48, default:16]\n");   \
    printf("\t\tbe    [亮区增强，value:0~48, default:16]\n");   \
    printf("\t\tdei   [de-interlace强度，value:0~7, default:0]\n");   \
    printf("\t\tsf    [空域去噪强度，value:0~255, default:3]\n");   \
    printf("\t\ttf    [时域去噪强度，value:0~63, default:1]\n");   \
    printf("\t\tmt    [运动判断阈值，value:0~7, default:1]\n");   \
    printf("\t\tcs    [色度去噪强度，value:0~63, default:8]\n");   \
    printf("\t\tenSP   [0, disable; 1,enable]\n");   \
    printf("\t\tchnsp [sp strength of chn，value:0~255, default:40]\n");   \ 
}

#define CHECK_RET(express,name)\
    do{\
        if (HI_SUCCESS != express)\
        {\
            printf("%s failed at %s: LINE: %d ! errno:%d \n", \
                name, __FUNCTION__, __LINE__, express);\
            return HI_FAILURE;\
        }\
    }while(0)


HI_S32 main(int argc, char *argv[])
{
    HI_S32 s32Ret;
    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    VPSS_GRP_PARAM_S stVpssGrpParam;
    VPSS_CHN_SP_PARAM_S stChnSpParam;
    VPSS_CHN_NR_PARAM_S stChnNrParam;
    
    HI_U8 para[16];
    HI_U32 value = 0;
    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
    
    if (argc < 4)
    {
        USAGE_HELP();
        return -1;
    }
    
    strcpy(para,argv[1]);  
    value = atoi(argv[2]);
    VpssGrp = atoi(argv[3]);
    if (5 == argc)
    {
        VpssChn = atoi(argv[4]);
    }

    s32Ret = HI_MPI_VPSS_GetGrpAttr(VpssGrp, &stVpssGrpAttr);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_GetGrpAttr");

    s32Ret = HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_GetChnAttr");

    s32Ret = HI_MPI_VPSS_GetGrpParam(VpssGrp, &stVpssGrpParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_GetGrpParam");

    s32Ret = HI_MPI_VPSS_GetChnSpParam(VpssGrp, VpssChn, &stChnSpParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_GetChnSpParam");

#ifndef HI_3518
    s32Ret = HI_MPI_VPSS_GetChnNrParam(VpssGrp, VpssChn, &stChnNrParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_GetChnNrParam");
#endif		

    if (0 == strcmp(para, "enIE"))
    {
       stVpssGrpAttr.bIeEn = value;
    }
    else if (0 == strcmp(para, "enNR"))
    {
        stVpssGrpAttr.bNrEn = value;
    }
    else if (0 == strcmp(para, "enDEI"))
    {
        stVpssGrpAttr.enDieMode = (value == 0) ? VPSS_DIE_MODE_NODIE : VPSS_DIE_MODE_DIE;
	}
	else if (0 == strcmp(para,"enHIST"))
	{
		stVpssGrpAttr.bHistEn = value;
	}
    else if (0 == strcmp(para, "ie"))
    {
        stVpssGrpParam.u32IeStrength = value;
        
    }
    else if (0 == strcmp(para, "iesp"))
    {
        stVpssGrpParam.u32IeSharp = value;
        
    }
    else if (0 == strcmp(para, "lum"))
    {
        stVpssGrpParam.u32Luminance = value;
        
    }
    else if (0 == strcmp(para, "con"))
    {
        stVpssGrpParam.u32Contrast = value;
        
    }
    else if (0 == strcmp(para, "de"))
    {
        stVpssGrpParam.u32DarkEnhance = value;
        
    } 
    else if (0 == strcmp(para, "be"))
    {
        stVpssGrpParam.u32BrightEnhance = value;
        
    }   
    else if (0 == strcmp(para, "dei"))
    {
        stVpssGrpParam.u32DiStrength = value;
        
    }
#ifndef HI_3518	
    else if (0 == strcmp(para, "sf"))
    {
        stChnNrParam.u32SfStrength = value;
        
    }    
    else if (0 == strcmp(para, "tf"))
    {
        stChnNrParam.u32TfStrength = value;
        
    }
    else if (0 == strcmp(para, "mt"))
    {
        stChnNrParam.u32MotionThresh = value;
        
    }
    else if (0 == strcmp(para, "cs"))
	{
		stChnNrParam.u32ChromaRange = value;
	}	
#else
	else if (0 == strcmp(para, "sf"))
    {
        stVpssGrpParam.u32SfStrength = value;
    }    
    else if (0 == strcmp(para, "tf"))
    {
        stVpssGrpParam.u32TfStrength = value;
    }
    else if (0 == strcmp(para, "mt"))
    {
        stVpssGrpParam.u32MotionThresh = value;
    }
    else if (0 == strcmp(para, "cs"))
    {
	    stVpssGrpParam.u32ChromaRange = value;
    }	
#endif	
    else if (0 == strcmp(para, "enSP"))
    {
        stVpssChnAttr.bSpEn = value;
    }
    else if (0 == strcmp(para, "chnsp"))    
    {
        stChnSpParam.u32LumaGain = value;
    }
    else
    {
        printf("err para\n");
        USAGE_HELP();
    }

    s32Ret = HI_MPI_VPSS_SetGrpAttr(VpssGrp, &stVpssGrpAttr);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_SetGrpAttr");

    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_SetChnAttr");

    s32Ret = HI_MPI_VPSS_SetGrpParam(VpssGrp, &stVpssGrpParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_SetGrpParam");

    s32Ret = HI_MPI_VPSS_SetChnSpParam(VpssGrp, VpssChn, &stChnSpParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_SetChnSpParam");

#ifndef HI_3518
    s32Ret = HI_MPI_VPSS_SetChnNrParam(VpssGrp, VpssChn, &stChnNrParam);
    CHECK_RET(s32Ret, "HI_MPI_VPSS_SetChnNrParam");
#endif

    printf("\t\tenIE   %d\n", stVpssGrpAttr.bIeEn);
    printf("\t\tenNR   %d\n", stVpssGrpAttr.bNrEn);
    printf("\t\tenDEI  %d\n", (stVpssGrpAttr.enDieMode == VPSS_DIE_MODE_NODIE)? 0:1);
    printf("\t\tenHIST %d\n", stVpssGrpAttr.bHistEn);
    printf("\t\tie     %d\n", stVpssGrpParam.u32IeStrength);
    printf("\t\tiesp   %d\n", stVpssGrpParam.u32IeSharp);
    printf("\t\tlum    %d\n", stVpssGrpParam.u32Luminance);
    printf("\t\tcon    %d\n", stVpssGrpParam.u32Contrast);
    printf("\t\tde     %d\n", stVpssGrpParam.u32DarkEnhance);
    printf("\t\tbe     %d\n", stVpssGrpParam.u32BrightEnhance);
    printf("\t\tdei    %d\n", stVpssGrpParam.u32DiStrength);

#ifndef HI_3518
    printf("\t\tsf     %d\n", stChnNrParam.u32SfStrength);
    printf("\t\ttf     %d\n", stChnNrParam.u32TfStrength);
    printf("\t\tmt     %d\n", stChnNrParam.u32MotionThresh);
#else
   	printf("\t\tsf     %d\n", stVpssGrpParam.u32SfStrength);
    printf("\t\ttf     %d\n", stVpssGrpParam.u32TfStrength);
    printf("\t\tmt     %d\n", stVpssGrpParam.u32MotionThresh);
    printf("\t\tcs     %d\n", stVpssGrpParam.u32ChromaRange);
#endif	

    printf("\t\tenSP   %d\n", stVpssChnAttr.bSpEn);
    printf("\t\tchnsp  %d\n", stChnSpParam.u32LumaGain);

    return 0;
}

