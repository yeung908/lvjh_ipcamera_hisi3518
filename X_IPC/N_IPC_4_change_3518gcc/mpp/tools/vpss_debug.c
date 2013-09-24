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

#define MAX_PARAMS	20

#define USAGE_HELP()\
{\
    printf("\n\tusage : %s -g group -c chn -s param1=value1,param2=value2...\n", argv[0]); \
    printf("\t        : %s -g group -c chn -f config_file\n", argv[0]);    	  \
    printf("\n\t param: \n");    \
    printf("\t\t tf             [time_strength, value:0~63, 	default:8 ]\n");   \
    printf("\t\t tf_y_pro       [tf_y_profile,  value:0~4,      default:4 ]\n");   \
    printf("\t\t sf_post_flag   [sf_post_flag,  value:0~1,      default:0 ]\n");   \
    printf("\t\t c_range        [c_range,       value:0~255,  	default:8 ]\n");   \
    printf("\t\t c_mad_thresh   [c_mad_thresh,  value:0~63,  	default:8 ]\n");   \
    printf("\t\t c_mad_slope    [c_mad_slope,   value:0~15,     default:0 ]\n");   \
    printf("\t\t s0             [space_strength,value:0~255, 	default:32]\n");   \
    printf("\t\t s1             [space_strength,value:0~16, 	default:2 ]\n");   \
    printf("\t\t s2             [space_strength,value:0~63, 	default:8 ]\n");   \
    printf("\n\t examples: %s -g 0 -c 0 -s tf=7,c_range=16\n", argv[0]);           \
    printf("\t             %s -g 0 -c 0 -f vpss_night.cfg \n", argv[0]);    	   \
    printf("\n\t append keyword \'off\' to disable image quality debug\n");		   \
    printf("\t   example:  %s -g 0 -c 0 off\n", argv[0]);		\
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

extern HI_S32 HI_MPI_VPSS_GetImageQualityCfg(VPSS_GRP VpssGrp,
				VPSS_CHN VpssChn, VPSS_IMG_QUALITY_CFG_S *pstImageQualityCfg);

extern HI_S32 HI_MPI_VPSS_SetImageQualityCfg(VPSS_GRP VpssGrp,
				VPSS_CHN VpssChn, VPSS_IMG_QUALITY_CFG_S *pstImageQualityCfg);

static int parse_line(char *line, VPSS_IMG_QUALITY_PARAM_S *pImgQualityParam)
{
	char para[255];
	unsigned int value;
	char *comma;

	if (line[0] == '#' || line[0] == '\0')	
		return 0;

	comma = strchr(line, '=');
	if (!comma)
	{
		printf("invalid set %s\n", line);
		return -1;
	}
	
	*comma = '\0';

	snprintf(para, 255, "%s", line);
	value = atoi(++comma);
	
#if 0
	printf("para=%s,value=%d\n", para, value);
#endif

	if (0 == strcmp(para, "tf"))
	{
		pImgQualityParam->u32TfStrength = value;
	} 
	else if (0 == strcmp(para, "tf_y_pro"))
	{
		pImgQualityParam->u32TfyProfile = value;
	}  
	else if (0 == strcmp(para, "c_range"))
	{
		pImgQualityParam->u32CStrength = value;
	}
	else if (0 == strcmp(para, "c_mad_thresh"))    
	{
		pImgQualityParam->u32CMADThresh = value;
	}
	else if (0 == strcmp(para, "c_mad_slope"))
	{
		pImgQualityParam->u32CMADSlope = value;
	}
	else if (0 == strcmp(para, "sf_post_flag"))
	{
		pImgQualityParam->u32SfPostFlag = value;
	} 
	else if (0 == strcmp(para, "s0"))
	{
		pImgQualityParam->u32SfStrength0 = value;
	}    
	else if (0 == strcmp(para, "s1"))
	{
		pImgQualityParam->u32SfStrength1 = value;
	}    
	else if (0 == strcmp(para, "s2"))
	{
		pImgQualityParam->u32SfStrength2 = value;
	}
	else
	{
		printf("unknown param %s\n", para);
		return -1;
	}	

	return 0;
}


static int parse_config_string(char *str, VPSS_IMG_QUALITY_PARAM_S *pImgQualityParam)
{
	char lines[MAX_PARAMS][100];
	int i = 0;
	int j = 0;
	
	char *string = str;

	if (!string) {
		printf("string invalid\n");
		return -1;
	}
	
	for (;;) {
	
		char *comma = strchr(string, ',');
		
		if (comma) {
			*comma = '\0';

			if (i > MAX_PARAMS)
				break;

			snprintf(lines[i], 100, "%s", string);

			i++;
			*comma = ',';
			string = ++comma;
			
		} else {
			if (string != '\0') {
			
				if (i > MAX_PARAMS)
						break;

				snprintf(lines[i], 100, "%s", string);

				i++;
				break;
				
			} else {
			
				printf("invalid param \n");
				return -1;
				
			}
		}
	}
	
	//printf("i=%d\n", i);

	for (j=0;j<i;j++) {
		//printf("%s\n", lines[j]);
		
		if (parse_line(lines[j], pImgQualityParam) < 0)
		{
			printf("%s parse line error\n", __func__);
			return -1;
		}
	}
	
	return 0;
}


static int parse_config_file(const char *file_name, VPSS_IMG_QUALITY_PARAM_S *pImgQualityParam)
{
	FILE *fp;
	int n = 0;
	char line[255];
	
	if (!(fp = fopen(file_name, "r")))
	{
		printf("open %s failed\n", file_name);
		return -1;
	}
	
	while(fgets(line, sizeof(line), fp)) {
		n++;
		line[strlen(line)-1] = '\0';

		//printf("%s\n", line);	
		if (parse_line(line, pImgQualityParam) < 0)
		{
			fclose(fp);
			return -1;
		}
	}
	
	//printf("line number = %d\n", n);

	fclose(fp);
	return 0;
}


int main(int argc, char **argv)
{
    int i = 0;
    int result = -1;
	int enable = 1;

  	VPSS_IMG_QUALITY_CFG_S stImgQualityCfg;
	VPSS_IMG_QUALITY_PARAM_S *pstImgQualityParam;

    VPSS_GRP VpssGrp = 0;
    VPSS_CHN VpssChn = 0;
    
	if (argc < 3)
	{
		printf("no input param\n");
		USAGE_HELP();
		return -1;
	}

	pstImgQualityParam = &stImgQualityCfg.stImageQualityParam;

	printf("%s %d\n", __func__, __LINE__);

#if 1
	//get group 0,channel 0 as default.
	result = HI_MPI_VPSS_GetImageQualityCfg(VpssGrp, VpssChn, &stImgQualityCfg);
	CHECK_RET(result, "HI_MPI_VPSS_GetImgQualityParam");

	for (i = 1; i < argc; i++)
	{
		if (!strcmp("-f", argv[i]))
		{
			i++;
			char *file_name = argv[i];
			result = parse_config_file(file_name, pstImgQualityParam);
		}
		else if (!strcmp("-s", argv[i]))
		{
			i++;
			char *string = argv[i];	
			result = parse_config_string(string, pstImgQualityParam);
		}
		else if (!strcmp("-g", argv[i]))
		{
			i++;			
			VpssGrp = atoi(argv[i]);		
			if (VpssGrp > VPSS_MAX_GRP_NUM || VpssGrp < 0)
			{
				printf("invalid group num %d", VpssGrp);
				return -1;
			}
		}
		else if (!strcmp("-c", argv[i]))
		{
			i++;
			VpssChn = atoi(argv[i]);
			if (VpssChn > VPSS_MAX_GRP_NUM || VpssChn < 0)
			{
				printf("invalid chn num %d", VpssChn);
				return -1;
			}
			
			/*if grp, chnl exist, get new debug param*/
			result = HI_MPI_VPSS_GetImageQualityCfg(VpssGrp, VpssChn, &stImgQualityCfg);
			CHECK_RET(result, "HI_MPI_VPSS_GetImgQualityParam");
		}
		else if (!strcmp("off", argv[i]))
		{
			i++;
			printf("\tdisable Image quality Cfg!!\n");
			enable = 0;
		}	
		else
		{
			printf("invalid options %s\n", argv[i]);
			result = -1;
		}

		if (result < 0)
		{
			return result;
		}		
	}

	if (enable)
	{
		stImgQualityCfg.bEnable = HI_TRUE;
		printf("\t\t tf             %d\n", pstImgQualityParam->u32TfStrength);
		printf("\t\t tf_y_pro       %d\n", pstImgQualityParam->u32TfyProfile);
		printf("\t\t sf_post_flag   %d\n", pstImgQualityParam->u32SfPostFlag);
		printf("\t\t c_range        %d\n", pstImgQualityParam->u32CStrength);
		printf("\t\t c_mad_thresh   %d\n", pstImgQualityParam->u32CMADThresh);
		printf("\t\t c_mad_slope    %d\n", pstImgQualityParam->u32CMADSlope);
		printf("\t\t s0             %d\n", pstImgQualityParam->u32SfStrength0);
		printf("\t\t s1             %d\n", pstImgQualityParam->u32SfStrength1);
		printf("\t\t s2             %d\n", pstImgQualityParam->u32SfStrength2);
	}
	else
	{
		stImgQualityCfg.bEnable = HI_FALSE;
	}

    result = HI_MPI_VPSS_SetImageQualityCfg(VpssGrp, VpssChn, &stImgQualityCfg);
    CHECK_RET(result, "HI_MPI_VPSS_SetImgQualityCfg");
    
#endif
    return 0;
}

