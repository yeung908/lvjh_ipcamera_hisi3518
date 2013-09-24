#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include "vadcDrv.h"

#ifdef HI3518


#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpp.h"
#include "hi_comm_venc.h"
#include "hi_comm_md.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vpp.h"
#include "mpi_venc.h"

#include "mpi_sys.h"
#include "mpi_isp.h"
#include "hi_sns_ctrl.h"
#endif 

static int vadc_fd = 0;
static int g_video_status = 1;
pthread_mutex_t g_vadc_dev_mutex;

int vadcDrv_SetRegister(int addr, int val)
{
	TMVADC tmvadc;
	int ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_register;
		tmvadc.vadc.singleregister.address = addr;
		tmvadc.vadc.singleregister.value = val;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_Open(char *devName)
{
	char name[64];
	
	if (devName == NULL)
	{
		return -1;
	}


	sprintf(name, "/dev/misc/%s", devName);

    vadc_fd = open(name, O_RDWR);
    if (vadc_fd < 0)
	{
		printf("Can not open %s!!!\n", name);
		return -1;
	}
	else
	{
		printf("Open %s: OK!!!\n", name);
	}
	
    return 0;
}

int vadcDrv_Close()
{
	close(vadc_fd);
	vadc_fd = -1;
	
	return 0;	
}

int vadcDrv_GetStandard(int channel, int *val)
{
#ifdef CCD	
	TMVADC tmvadc;
	int ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_get_standard;
		tmvadc.vadc.standard.channel = channel;
		tmvadc.vadc.standard.standard = 0;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			//printf("vadcDrv_SetStandard(%d) Failed!\n", val);
			return -1;
		}
		else
		{
			*val = tmvadc.vadc.standard.standard;
			//printf("vadcDrv_SetStandard(%d) OK!\n", val);
			return 0;
		}
	}
	else
	{
		//printf("vadcDrv_SetStandard(%d) Failed!\n", val);
		return -1;
	}
#else
	return 0;
#endif
}

int vadcDrv_SetStandard(int channel, int val)
{
#ifdef CCD
	TMVADC tmvadc;
	int ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_standard;
		tmvadc.vadc.standard.channel = channel;
		tmvadc.vadc.standard.standard = val;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			//printf("vadcDrv_SetStandard(%d) Failed!\n", val);
			return -1;
		}
		else
		{
			//printf("vadcDrv_SetStandard(%d) OK!\n", val);
			return 0;
		}
	}
	else
	{
		//printf("vadcDrv_SetStandard(%d) Failed!\n", val);
		return -1;
	}
#else
	return 0;
#endif
}

int vadcDrv_SetBrightness(int channel, int val)
{
	TMVADC tmvadc;
	int ret = -1;	
		
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_brightness;
		tmvadc.vadc.brightness.channel = channel;
		tmvadc.vadc.brightness.brightness = val;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_SetContrast(int channel, int val)
{
	TMVADC tmvadc;
	int ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_contrast;
		tmvadc.vadc.contrast.channel = channel;
		tmvadc.vadc.contrast.contrast = val;
	
		pthread_mutex_unlock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_SetSaturation(int channel, int val)
{
	TMVADC 	tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_saturation;
		tmvadc.vadc.saturation.channel = channel;
		tmvadc.vadc.saturation.saturation = val;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_SetHue(int channel, int val)
{
	TMVADC tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_hz;
		tmvadc.vadc.hue.channel = channel;
		tmvadc.vadc.hue.hue = val;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_SetVGA(int val)
{
	TMVADC tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = ioctl(vadc_fd, DC_SET_IMAGESIZE, val);

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}


int vadcDrv_SetBack(void)
{
	TMVADC tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_back;
		
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_SetColor(void)
{
	TMVADC tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_color;
		
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}


int vadcDrv_GetStatus(int channel)
{
	int ret = -1;
	TMVADC tmvadc;		

#ifdef CCD	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_get_videostatus;
		tmvadc.vadc.status.channel = channel;
		tmvadc.vadc.status.status = 0;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{	
			return tmvadc.vadc.status.status;
		}
	}
	else
	{
		return -1;
	}
#else
	return 1;
#endif
}

int vadcDrv_SetImageFlip(int channel, int flag)
{
#ifdef CCD
	return -1;
#else
	TMVADC tmvadc;
	int	ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_flip;
		tmvadc.vadc.flip.channel = channel;
		tmvadc.vadc.flip.flip = flag;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
#endif
}

int vadcDrv_SetImageMirror(int channel, int flag)
{
#ifdef CCD
	return -1;
#else
	TMVADC tmvadc;
	int	ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_mirror;
		tmvadc.vadc.mirror.channel = channel;
		tmvadc.vadc.mirror.mirror = flag;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
#endif
}

int vadcDrv_SetImageHz(int channel, int hz)
{
#ifdef CCD
	return -1;
#else
	TMVADC tmvadc;
	int	ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_hz;
		tmvadc.vadc.hz.channel = channel;
		tmvadc.vadc.hz.hz = hz;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		printf("vadcDrv_SetImageHz  %d..... ..\n", tmvadc.opt);
		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
#endif
}

int vadcDrv_SetImageFormat(int channel, int format)
{
#ifdef CCD
	return -1;
#else
	TMVADC tmvadc;
	int	ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_format;
		tmvadc.vadc.format.channel = channel;
		tmvadc.vadc.format.format = format;
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);
		
		printf("vadcDrv_SetImageFormat: %d\n", ret);
		
		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
#endif	
}

int vadcDrv_Get_Register(tmVadc_Register *pregister)
{
	TMVADC 	tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_get_register;
		memcpy(&tmvadc.vadc.singleregister, pregister, sizeof(tmVadc_Register));
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			memcpy(pregister, &tmvadc.vadc.singleregister, sizeof(tmVadc_Register));
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

int vadcDrv_Set_Register(tmVadc_Register *pregister)
{
	TMVADC 	tmvadc;
	int		ret = -1;	
	
	if (vadc_fd > 0)
	{
		tmvadc.opt = vadc_set_register;
		memcpy(&tmvadc.vadc.singleregister, pregister, sizeof(tmVadc_Register));
	
		pthread_mutex_lock(&g_vadc_dev_mutex);

		ret = write(vadc_fd, (void *)&tmvadc, sizeof(TMVADC));

		pthread_mutex_unlock(&g_vadc_dev_mutex);

		if (ret == -1)
		{
			return -1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

// API
int g_videoLostDetect_run_flag = 0;
int g_videoLostDetect_pause_flag = 0;

int videoLostDetectFun()
{
	int i = 0;
	int ret = -1;
	TMVADC vadc;
	int nChannelNum = 0;
	
	nChannelNum = get_channel_num();
	
	while (g_videoLostDetect_run_flag)
	{
		if (g_videoLostDetect_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		for (i=0; i<nChannelNum; i++)
		{
			ret = vadcDrv_GetStatus(i);
			if (ret == 1)
			{
				g_video_status = ret;
				//printf("vadcDrv_GetStatus(%d 1) OK!\n", i); 
			}
			if (ret == 0)
			{
				g_video_status = ret;
				//printf("vadcDrv_GetStatus(%d 0) OK!\n", i); 
			}
			if (ret == -1)
			{
				//printf("vadcDrv_GetStatus(%d) Failed!\n", i); 
			}
		}
		
		sleep(1);
	}
	
	pthread_exit(NULL);
	
	return 0;
}

int videoLostDetectStart()
{
	int ret = -1;
	pthread_t threadID;
	
	g_videoLostDetect_run_flag = 1;

#ifdef HD_CMOS
	#ifdef CCD_CMOS
	ret = vadcDrv_Open("mt9v111");
	#else
	ret = vadcDrv_Open("mt9d131");
	#endif
#endif

#ifdef CCD
	printf("open tvp5150 ok!!!!\n");
	ret = vadcDrv_Open("tvp5150");
#endif

	if (ret < 0)
	{
		printf("vadcDrv_Open() Failed!\n");
		return -1;
	}

	ret = pthread_mutex_init(&g_vadc_dev_mutex, NULL);
	if (ret < 0)
	{
		return -1;
	}

#ifdef CCD	
	ret = pthread_create(&threadID, NULL, (void *)videoLostDetectFun, NULL);
	if (ret)
	{
		g_videoLostDetect_run_flag = 0;		
		return -1;
	}
#endif
	
	//printf("videoLostDetectStart() OK!\n");

	return 0;
}

int videoLostDetectStop()
{
	g_videoLostDetect_run_flag = 0;
	
	return 0;
}

int videoLostDetectPause()
{
	g_videoLostDetect_pause_flag = 1;
	
	return 0;
}

int videoLostDetectResume()
{
	g_videoLostDetect_pause_flag = 0;
	
	return 0;
}

int getVideoStatus(int channel)
{
	return g_video_status;
}

HI_S32 ISP_SensorInit(void)
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


int startIsp(void)
{
    HI_S32 s32Ret;
    ISP_IMAGE_ATTR_S stImageAttr;
    ISP_INPUT_TIMING_S stInputTiming;

    /* 1. isp init */
    s32Ret = ISP_SensorInit();
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_Init failed!\n", __FUNCTION__);
        return s32Ret;
    }

    /* 2. isp set image attributes */
    /* note : different sensor, different ISP_IMAGE_ATTR_S define.
              if the sensor you used is different, you can change
              ISP_IMAGE_ATTR_S definition */
//        case OMNI_OV9712_DC_720P_30FPS:
            stImageAttr.enBayer      = 1;//BAYER_BGGR;
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
    stInputTiming.enWndMode = 0;
    s32Ret = HI_MPI_ISP_SetInputTiming(&stInputTiming);
    if (s32Ret != HI_SUCCESS)
    {
        printf("%s: HI_MPI_ISP_SetInputTiming failed with %#x!\n", __FUNCTION__, s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

/******************************************************************************
* funciton : ISP Run
******************************************************************************/
HI_S32 ISP_Run()
{
    HI_S32 s32Ret;
	static pthread_t gs_IspPid;

    s32Ret = startIsp();
    if (HI_SUCCESS != s32Ret)
    {
        printf("%s: ISP init failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }

    if (0 != pthread_create(&gs_IspPid, 0, (void* (*)(void*))HI_MPI_ISP_Run, NULL))
    {
        printf("%s: create isp running thread failed!\n", __FUNCTION__);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}