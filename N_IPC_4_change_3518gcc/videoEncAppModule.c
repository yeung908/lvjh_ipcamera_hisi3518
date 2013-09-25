#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "videoEncAppModule.h"
#include "videoIn/videoInModule.h"
#include "videoIn/videoIn.h"
#include "videoEnc/videoEncModule.h"
#include "videoEnc/videoEnc.h"

#include "videoIn/vinHi3511.h"
#include "videoEnc/vencHi3511.h"

#include "vadcDrv.h"
#include "util.h"
#include "param.h"

#define  CHN0_SOUR 		"/mnt/mtd/dvs/mobile/tmpfs/ch0.264"
#define  CHN1_SOUR		"/mnt/mtd/dvs/mobile/tmpfs/ch1.264"
#define  MOBILE_PICTURE "/mnt/mtd/dvs/mobile/tmpfs/mobile.jpg"
#define  NEW_MOBILE_PICTURE "/mnt/mtd/dvs/www/tmpfsmobile.jpg"

#define  MOBILE_LENGHT_CONFIG  "/mnt/mtd/dvs/mobile/tmpfs/mobile_len.conf"

pthread_mutex_t  g_video_mutex;
pthread_mutex_t  g_video_main_mutex;
pthread_mutex_t  g_video_sub_mutex;


// global
int g_video_max_channel = 1;

int g_venc_module_run_flag = 0;
int g_venc_module_pause_flag[4] = {0, 0, 0, 0};
int g_venc_error_flag = 0;

int g_mobile_module_run_flag = 0;
int g_mobile_module_pause_flag = 0;

int g_jpeg_snapshot_channel = -1;
int g_jpeg_upload_type = 0;

int g_bitrate_osd_flag[4] = {0, 0, 0, 0};
int g_time_osd_flag[4] = {0, 0, 0, 0};
int g_nBitRate[4][2];
int g_nFrameRate[4][2];

videoSendFun_t g_bitrate_send_fun = NULL;
videoSendFun_t g_jpeg_send_fun = NULL;

int g_md_status[4] = {0, 0, 0, 0};

// OSD
static int g_osd_module_run_flag = 0;
static int g_osd_module_pause_flag = 0;
OSD_PARAM g_osd_param[MAX_CHANNEL];

static int g_osd_time_color = 0x000000;
static int g_osd_bits_color = 0x000000;
static int g_osd_title_color[4] = {0x000000, 0, 0, 0};
static int fp;

int set_time_osd_color(int color)
{
	g_osd_time_color = color;

	return 0;
}

int set_bits_osd_color(int color)
{
	g_osd_bits_color = color;

	return 0;
}

int set_title_osd_color(int index, int color)
{
	if (index>=0 && index<4)
	{
		g_osd_title_color[index] = color;
	}

	return 0;
}

int getVideoInMotionStatus(int nChannel)
{
	if (nChannel<0 || nChannel>=g_video_max_channel)
	{
		return 0;
	}
	
	if (g_md_status[nChannel])
	{
		g_md_status[nChannel] = 0;
		
		return 1;
	}
	else
	{
		return 0;
	}
}

int putVideoInMotionStatus(int nChannel)
{
	if (nChannel<0 || nChannel>=g_video_max_channel)
	{
		return -1;
	}
	
	g_md_status[nChannel] = 1;
	
	return 0;
}

// 设置视频码流传送回调函数
int videoEncModuleSetvideoSendFunc(videoSendFun_t bitratevideoSendFunc, videoSendFun_t jpegvideoSendFunc)
{
	g_bitrate_send_fun = bitratevideoSendFunc;
	g_jpeg_send_fun = jpegvideoSendFunc;

	return 0;
}

// 初始化模块的参数
int videoEncModuleInit(int nMaxChn)
{
	int i = 0;
	
	if (nMaxChn<=0 || nMaxChn>4)
	{
		return -1;
	}
	else
	{
		g_video_max_channel = nMaxChn;	
	}
		
	return 0;
}

int videoEncModuleOpen()
{
	int ret = -1;
	int i = 0;
	VIDEO_STANDARD_PARAM standard;
	VENC_PARAM encParam;
	VIDEO_ENC_PARAM vencParam;
	
	getVideoInStandardParam(0, &standard);
	
	// 初始化VADC
	vadcDrv_SetStandard(0, standard.nStandard&0x0F);	// 其中最后一位才是视频制式
	
	// 初始化VIN
	ret = videoInInit(Hi3511VinInfo);

	if (ret < 0)
	{
		printf("videoInInit(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
			
	// 打开VIN
	ret = videoInOpen(0);
	if (ret < 0)
	{
		printf("videoInOpen(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}

	// 初始化视频编码器
	ret = videoEncInit(Hi3511VideoEncInfo);
	if (ret)
	{
		printf("videoEncInit(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
		
	// 打开视频编码器
	for (i=0; i<g_video_max_channel; i++)
	{
		ret = videoEncOpen(i, 0);
		if (ret < 0)
		{
			printf("videoEncOpen(%s %d) Failed!\n", __FILE__, __LINE__);
			return -1;
		}
	}
	
	return 0;		
}

// 视频模块参数设置
int videoEncModuleSetup()
{
	int i = 0;
	int ret = -1;
	VIDEO_STANDARD_PARAM standard;
	VENC_PARAM encParam;	
	VIDEO_ENC_PARAM vencParam;
	
	getVideoInStandardParam(0, &standard);
	getVideoEncParam(0, 0, &encParam);
	
	//判断视频格式
	switch (encParam.nEncodeHeight)
	{
	case 576:
	case 480:
		if (encParam.nEncodeWidth == 720)
		{
			standard.nStandard |= 0x00;
		}
		if (encParam.nEncodeWidth == 640)
		{
			standard.nStandard = 4;
			#ifdef HD_CMOS
			standard.nStandard = 9;
			printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
			printf("standard.nStandard = 9\n");
			#endif
		}
		if (encParam.nEncodeWidth == 720)
		{
			standard.nStandard = 8;
		}
		
		break;
		
	case 288:
	case 240:
		printf("Entry primary videoEncModuleSetup function...\n");
		if (encParam.nEncodeWidth==720 || encParam.nEncodeWidth==704)
		{
			standard.nStandard |= 0x01000000;
		}
		else if (encParam.nEncodeWidth == 352)
		{
			standard.nStandard |= 0x02000000;
		}
		else
		{
			standard.nStandard = 5;
		}
		break;
		
	case 144:
	case 120:
	case 112:
	case 128:
		standard.nStandard |= 0x03000000;
		break;

	case 1200:
		standard.nStandard = 6;
		break;

	case 720:
		standard.nStandard = 7;
		break;
				
	default:
		standard.nStandard |= 0x02000000;
		break;
	}
	
	// 设置VIN
	if (g_video_max_channel == 1)
	{
		ret = videoInSetup(0, standard.nStandard);
	}
	else
	{
		standard.nStandard &= 0x00FFFFFF;
		ret = videoInSetup(0, standard.nStandard);
	}
	
	// 设置视频编码器
	for (i=0; i<g_video_max_channel; i++)
	{
		memset(&encParam, 0, sizeof(VENC_PARAM));
		getVideoEncParam(i, 0, &encParam);
		printf("**************************encModule*****************\n");
		printf("encParam.nEncodeHeight = %d, encParam.nEncodeWidth = %d\n", encParam.nEncodeHeight, encParam.nEncodeWidth);
		memcpy(&vencParam.param.encParam, &encParam, sizeof(VENC_PARAM));
		
		ret = videoEncSetup(i, 0, venc_setup_enc_param, &vencParam);
		if (ret < 0)
		{
			printf("videoEncSetup1(%s %d) Failed!\n", __FILE__, __LINE__);
			return -1;
		}
		// Add the code by lvjh, 2008-03-29
		//setVideoFormat(i, 0, encParam.nEncodeWidth, encParam.nEncodeHeight);
		setVideoFormat(i, 0, encParam.nEncodeWidth, encParam.nEncodeHeight, encParam.reserve);
		
		memset(&encParam, 0, sizeof(VENC_PARAM));
		getVideoEncParam(i, 1, &encParam);

		#ifdef YIYUAN_VIDEO
		encParam.nEncodeWidth = 704;
		encParam.nEncodeHeight = 576;
		#endif
		memcpy(&vencParam.param.encParam, &encParam, sizeof(VENC_PARAM));
		
		ret = videoEncSetup(i, 1, venc_setup_enc_param, &vencParam);
		if (ret < 0)
		{
			printf("videoEncSetup2(%s %d) Failed!\n", __FILE__, __LINE__);
			return -1;
		}
		// Add the code by lvjh, 2008-03-29
		//setVideoFormat(i, 1, encParam.nEncodeWidth, encParam.nEncodeHeight);
		

		setVideoFormat(i, 1, encParam.nEncodeWidth, encParam.nEncodeHeight, encParam.reserve);
	}
	
	return 0;
}

int videoEncModuleClose()
{
	int i = 0;
	
	for (i=0; i<g_video_max_channel; i++)
	{
		videoEncStop(i, 0);
		videoEncClose(i, 0);
	}
	
	videoInStop(0);
	videoInClose(0);
					
	g_venc_module_run_flag = 0;
	
	for (i=0; i<g_video_max_channel; i++)
	{
		g_venc_module_pause_flag[i] = 0;
	}
		
	return 0;		
}

int videoEncFunc1()
{
	int i = 0;
	int ret = -1;
	int size = 0;
	MD_STATUS status;
	char *stream = NULL;
	unsigned long ptr = 0;
	
	struct timeval tv;
	unsigned long nStartTimeCount = 0;
	unsigned long nCurTimeCount = 0;
	unsigned long nCount = 0;

	unsigned long bitRate[4] = {0, 0, 0, 0};
	unsigned long frameRate[4] = {0, 0, 0, 0};
	
	stream = (char *)malloc(720*576*3/2);
	if (stream == NULL)
	{
		printf("Can not allocate memory(720*576*3/2)!\n");
		return -1;
	}

	gettimeofday (&tv, NULL);
	nStartTimeCount = tv.tv_sec*1000000+tv.tv_usec;

	#if 1
	if (access(CHN0_SOUR, F_OK) == -1) {
		if ( mkfifo(CHN0_SOUR, 0777) != 0) {
		return -1;
		}
	}

	int fpH264File = open(CHN0_SOUR, O_RDWR|O_NONBLOCK);
	if(fpH264File == -1){
			printf("open fpH264File error\n");
			return -1;
	}
	#endif

	
	ret = pthread_mutex_init(&g_video_mutex, NULL);
	if (ret < 0)
	{
		printf("pthread_mutex_init: Failed!\n");
	}
	printf("%s %d&&&&\n", __func__, __LINE__);
		
	while (g_venc_module_run_flag)
	{
		#if 0
		if (g_video_max_channel == 1)
		{
			if (g_venc_module_pause_flag[0])
			{
				sleep(1);

				continue;
			}
		}
		else
		{
			if (g_venc_module_pause_flag[0] && g_venc_module_pause_flag[1] && g_venc_module_pause_flag[2] && g_venc_module_pause_flag[3])
			{
				sleep(1);
								
				continue;	
			}
		}
		#endif
	

		pthread_mutex_lock(&g_video_mutex);
		
		// 采集VADC的视频数据
		#if 0
		size = 0;
		ret = videoInGetStream(0, stream, &size);
		if (ret < 0)
		{
			printf("%s %d&&&&\n", __func__, __LINE__);
	
			continue;
		}
		#endif

		// Add the code by lvjh, 2009-04-22
		gettimeofday (&tv, NULL);
		nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
			
		for (i=0; i<g_video_max_channel; i++)
		{	
			if (g_venc_module_pause_flag[i])
			{
				continue;
			}
			
			// 将视频数据送入编码器, 并取出码流
			size = 0;
//			ret = videoEncGetStream(i, 0, stream, &size, fpH264File);
			ret = Hi3511EncGetStream(i, 0, stream, &size, fpH264File);
			if (ret < 0)
			{
				printf("videoEncGetStream1: Failed, %d\n", ret);
				
				// Add the code by lvjh, 2009-06-08
				if (ret == -2)
				{
					//RebootSystem();			// Add the code by lvjh, 2009-08-07
					
					g_venc_error_flag = 1;
					sleep(1);
				}
				
				continue;
			}
			// Add the code by lvjh, 2009-04-18
			if (size <= 0)
			{
				continue;
			}
			
			bitRate[i] += size;
			frameRate[i] += 1;
			
			if (nCurTimeCount >= nStartTimeCount+1000000)
			{
				g_nBitRate[i][0] = bitRate[i]*8;
				if (frameRate[i] > 30)
				{
					g_nFrameRate[i][0] =  30;
				}
				else
				{
					g_nFrameRate[i][0] = frameRate[i];
				}
				bitRate[i] = 0;
				frameRate[i] = 0;
				
				nStartTimeCount = nCurTimeCount;
			}
			
			// 传送视频码流
			if (g_bitrate_send_fun)
			{
				g_bitrate_send_fun(i, 0, stream, size, 0);
				//printf("g_bitrate_send_fun1: %d %d!\n", i, size);
			}
		
			// 释放编码器的码流
			videoEncReleaseStream(i, 0);			
			#if 0
			// 将视频数据送入编码器, 并取出码流
			if (g_jpeg_snapshot_channel == i)
			{
				ret = videoEncGetJpeg(i, 0, stream, &size);
				if (ret < 0)
				{
					continue;
				}
				// Add the code by lvjh, 2009-04-18
				if (size <= 0)
				{
					continue;
				}
				
				// 传送视频码流
				if (g_jpeg_send_fun)
				{
//					set_snapshot_buffer_size(stream, size);
					g_jpeg_send_fun(i, 0, stream, size, g_jpeg_upload_type);
					
				}
				
				g_jpeg_snapshot_channel = -1;
				g_jpeg_upload_type = -1;
				
			}
			#endif
			
		}
		
		// 释放VIN的码流
		videoInReleaseStream(0);

		pthread_mutex_unlock(&g_video_mutex);
	}

	if (stream)
	{
		free(stream);
		stream = NULL;	
	}
	close(fpH264File);
	pthread_mutex_destroy(&g_video_mutex);
	pthread_exit(NULL);
	
	return 0;
}

int videoEncFunc2()
{
	int i = 0;
	int ret = -1;
	int size = 0;
	MD_STATUS status;
	char *stream = NULL;
	unsigned long ptr = 0;
	
	struct timeval tv;
	unsigned long nStartTimeCount = 0;
	unsigned long nCurTimeCount = 0;
	unsigned long nCount = 0;

	unsigned long bitRate[4] = {0, 0, 0, 0};
	unsigned long frameRate[4] = {0, 0, 0, 0};
	
	stream = (char *)malloc(720*576*3/2);
	if (stream == NULL)
	{
		printf("Can not allocate memory(720*576*3/2)!\n");
		return -1;
	}

	gettimeofday (&tv, NULL);
	nStartTimeCount = tv.tv_sec*1000000+tv.tv_usec;


	#if 1
	if (access(CHN1_SOUR, F_OK) == -1) {
		if ( mkfifo(CHN1_SOUR, 0777) != 0) {
		return -1;
		}
	}

	int fpH264File = open(CHN1_SOUR, O_RDWR|O_NONBLOCK);
	if(fpH264File == -1){
			printf("open fpH264File error\n");
			return -1;
	}
	
	#endif

	ret = pthread_mutex_init(&g_video_main_mutex, NULL);
	if (ret < 0)
	{
		printf("pthread_mutex_init: Failed!\n");
	}
	
	
	while (g_venc_module_run_flag)
	{
		if (g_video_max_channel == 1)
		{
			if (g_venc_module_pause_flag[0])
			{
				sleep(1);

				continue;
			}
		}
		else
		{
			if (g_venc_module_pause_flag[0] && g_venc_module_pause_flag[1] && g_venc_module_pause_flag[2] && g_venc_module_pause_flag[3])
			{
				sleep(1);
								
				continue;	
			}
		}

		pthread_mutex_lock(&g_video_main_mutex);
		

		// 采集VADC的视频数据
		size = 0;
		ret = videoInGetStream(0, stream, &size);
		if (ret < 0)
		{
			continue;
		}

		// Add the code by lvjh, 2009-04-22
		gettimeofday (&tv, NULL);
		nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
			
		for (i=0; i<g_video_max_channel; i++)
		{	
			if (g_venc_module_pause_flag[i])
			{
				continue;
			}
			
			// 将视频数据送入编码器, 并取出码流
			size = 0;
			ret = videoEncGetStream(i, 1, stream, &size, fpH264File);
			if (ret < 0)
			{
				printf("videoEncGetStream2: Failed, %d\n", ret);
				
				// Add the code by lvjh, 2009-06-08
				if (ret == -2)
				{
					//RebootSystem();			// Add the code by lvjh, 2009-08-07
					
					g_venc_error_flag = 1;
					sleep(1);
				}
				
				continue;
			}
			// Add the code by lvjh, 2009-04-18
			if (size <= 0)
			{
				continue;
			}
			
			bitRate[i] += size;
			frameRate[i] += 1;
			
			if (nCurTimeCount >= nStartTimeCount+1000000)
			{
				g_nBitRate[i][1] = bitRate[i]*8;
				if (frameRate[i] > 30)
				{
					g_nFrameRate[i][1] =  30;
				}
				else
				{
					g_nFrameRate[i][1] = frameRate[i];
				}
				bitRate[i] = 0;
				frameRate[i] = 0;
				
				nStartTimeCount = nCurTimeCount;
			}
			
			// 传送视频码流
			if (g_bitrate_send_fun)
			{
				g_bitrate_send_fun(i, 1, stream, size, 0);
				//printf("g_bitrate_send_fun2: %d %d!\n", 1, size);
			}
		
			// 释放编码器的码流
			videoEncReleaseStream(i, 1);
			pthread_mutex_unlock(&g_video_main_mutex);
		}
		
	}

	if (stream)
	{
		free(stream);
		stream = NULL;	
	}
	close(fpH264File);
	pthread_mutex_destroy(&g_video_main_mutex);
	pthread_exit(NULL);
	
	return 0;
}


int rtspFunc()
{
	int i = 0;
	int ret = -1;
	int size = 0;
	MD_STATUS status;
	char *stream = NULL;
	unsigned long ptr = 0;
	
	struct timeval tv;
	unsigned long nStartTimeCount = 0;
	unsigned long nCurTimeCount = 0;
	unsigned long nCount = 0;

	unsigned long bitRate[4] = {0, 0, 0, 0};
	unsigned long frameRate[4] = {0, 0, 0, 0};
	
	stream = (char *)malloc(720*576*3/2);
	if (stream == NULL)
	{
		printf("Can not allocate memory(720*576*3/2)!\n");
		return -1;
	}

	gettimeofday (&tv, NULL);
	nStartTimeCount = tv.tv_sec*1000000+tv.tv_usec;
	printf("*****************************************!\n");
  #if 0
	
	if (access(FIFO_NAME, F_OK) == -1) {
		if ( mkfifo(FIFO_NAME, 0777) != 0) {
		return -1;
		}
	}

	printf("*****************************************!\n");
    int fpH264File;
	fpH264File = open(FIFO_NAME, O_WRONLY|O_CREAT);
	if(fpH264File == -1){
			printf("open fpH264File error\n");\
			return -1;
	}
	printf("*****************************************!\n");
	printf("mkfifo success!\n");
	#endif


	#if 1
	if (access("/mnt/mtd/dvs/mobile/tmpfs/test.264", F_OK) == -1) {
		if ( mkfifo("/mnt/mtd/dvs/mobile/tmpfs/test.264", 0777) != 0) {
		return -1;
		}
	}

	int fpH264File = open("/mnt/mtd/dvs/mobile/tmpfs/test.264", O_RDWR|O_NONBLOCK);
	if(fpH264File == -1){
			printf("open fpH264File error\n");
			return -1;
	}
	
	#endif
		
	while (g_venc_module_run_flag)
	{
		if (g_video_max_channel == 1)
		{
			if (g_venc_module_pause_flag[0])
			{
				sleep(1);

				continue;
			}
		}
		else
		{
			if (g_venc_module_pause_flag[0] && g_venc_module_pause_flag[1] && g_venc_module_pause_flag[2] && g_venc_module_pause_flag[3])
			{
				sleep(1);
								
				continue;	
			}
		}

		// 采集VADC的视频数据
		size = 0;
		ret = videoInGetStream(0, stream, &size);
		if (ret < 0)
		{
			continue;
		}

		// Add the code by lvjh, 2009-04-22
		gettimeofday (&tv, NULL);
		nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
			
		for (i=0; i<g_video_max_channel; i++)
		{	
			if (g_venc_module_pause_flag[i])
			{
				continue;
			}
			
			// 将视频数据送入编码器, 并取出码流
			size = 0;
			ret = videoEncGetStream(i, 0, stream, &size, fpH264File);
			if (ret < 0)
			{
				printf("videoEncGetStream1: Failed, %d\n", ret);
				
				// Add the code by lvjh, 2009-06-08
				if (ret == -2)
				{
					//RebootSystem();			// Add the code by lvjh, 2009-08-07
					
					g_venc_error_flag = 1;
					sleep(1);
				}
				
				continue;
			}
			// Add the code by lvjh, 2009-04-18
			if (size <= 0)
			{
				continue;
			}
			
			bitRate[i] += size;
			frameRate[i] += 1;
			
			if (nCurTimeCount >= nStartTimeCount+1000000)
			{
				g_nBitRate[i][0] = bitRate[i]*8;
				if (frameRate[i] > 30)
				{
					g_nFrameRate[i][0] =  30;
				}
				else
				{
					g_nFrameRate[i][0] = frameRate[i];
				}
				bitRate[i] = 0;
				frameRate[i] = 0;
				
				nStartTimeCount = nCurTimeCount;
			}
			
			// 传送视频码流
			if (g_bitrate_send_fun)
			{
				g_bitrate_send_fun(i, 0, stream, size, 0);
				//printf("g_bitrate_send_fun1: %d %d!\n", i, size);
			}
		
			// 释放编码器的码流
			videoEncReleaseStream(i, 0);			
			
			// 将视频数据送入编码器, 并取出码流
			if (g_jpeg_snapshot_channel == i)
			{
				ret = videoEncGetJpeg(i, 0, stream, &size);
				if (ret < 0)
				{
					continue;
				}
				// Add the code by lvjh, 2009-04-18
				if (size <= 0)
				{
					continue;
				}
				
				// 传送视频码流
				if (g_jpeg_send_fun)
				{
					g_jpeg_send_fun(i, 0, stream, size, g_jpeg_upload_type);
				}
				
				g_jpeg_snapshot_channel = -1;
				g_jpeg_upload_type = -1;
				
			}
		}
		
		// 释放VIN的码流
		videoInReleaseStream(0);
	}

	if (stream)
	{
		free(stream);
		stream = NULL;	
	}
	close(fpH264File);
	pthread_exit(NULL);
	
	return 0;
}


struct flock* file_lock(short type, short whence)
{
    static struct flock ret;
    ret.l_type = type ;
    ret.l_start = 0;
    ret.l_whence = whence;
    ret.l_len = 0;
    ret.l_pid = getpid();
    return &ret;
}


#ifdef MOBILE_VIEW
int mobile_send_fun(int nChannel, char *stream, int nSize)
{
	int nRet = -1;
	FILE *fp = NULL;
	
	if (stream == NULL)
	{
		return -1;
	}
	if (nSize <= 0)
	{
		return -1;
	} 
	
	//remove("/mnt/mtd/dvs/mobile/tmpfs/mobile.jpg");

	fp = fopen("/mnt/mtd/dvs/mobile/tmpfs/mobile.jpg", "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: mobile.jpg.\n");

		return -1;
	}

	fcntl(fp , F_SETLKW, file_lock(F_WRLCK, SEEK_SET));
	nRet = fwrite(stream, 1, nSize, fp);
	if (nRet != nSize)
	{
		printf("Can not write the file(%d): mobile.jpg.\n", nRet);
		fclose(fp);
		return -1;
	}
	fcntl(fp , F_SETLKW, file_lock(F_UNLCK, SEEK_SET));
	//writeConfigFile(WEBSERVER_MOBILE_CONFIG, 0, 1);	
	
	//write(fpH264File, temp, offset);
	
	fclose(fp);
	return 0;
}

int send_mkfifo_mobile_picture(int nChannel, char *stream, int nSize)
{
	int led_control_pipe;
	int null_writer_fd; // for read endpoint not blocking when control process exit

	double period = 0.5;

	unlink(MOBILE_PICTURE);
	mkfifo(MOBILE_PICTURE, 0666);

	led_control_pipe = open(MOBILE_PICTURE, O_RDONLY | O_NONBLOCK);
	if (led_control_pipe < 0) {
		perror("open control pipe for read");
		exit(1);
	}
	null_writer_fd = open(MOBILE_PICTURE, O_WRONLY | O_NONBLOCK);
	if (null_writer_fd < 0) {
		perror("open control pipe for write");
		exit(1);
	}

	for (;;) {
		fd_set rds;
		struct timeval step;
		int ret;

		FD_ZERO(&rds);
		FD_SET(led_control_pipe, &rds);
		step.tv_sec  = period;
		step.tv_usec = (period - step.tv_sec) * 1000000L;

		ret = select(led_control_pipe + 1, &rds, NULL, NULL, &step);
		if (ret < 0) {
			perror("select");
			exit(1);
		}
		if (ret == 0) {
			;
		} else if (FD_ISSET(led_control_pipe, &rds)) {
			static char buffer[200];
			for (;;) {
				char c;
				int len = strlen(buffer);
				
				write(led_control_pipe, stream, nSize);
				buffer[len] = c;
			}
		}
	}
	return 0;
}



int start_lock_file(int fd, int write_lock, int wait_lock)
{
	int nRet = -1;
    struct flock slock;
    int cmd;
    
    if (write_lock) 
    {
        slock.l_type = F_WRLCK;
    }
    else 
    {
        slock.l_type = F_RDLCK;
    }
    slock.l_whence = SEEK_SET;
    slock.l_start = 0;
    slock.l_len = 0;
    if (wait_lock) 
    {
        cmd = F_SETLKW;
    }
    else 
    {
        cmd = F_SETLK;
    }
    
    nRet = fcntl(fd, cmd, &slock);
    if (nRet < 0)
    {
    	printf("fcntl(%d %s): failed.\n", errno, strerror(errno));
    	return -1;
    }
    else
    {
    	return 0;
    }
}

int stop_lock_file(int fd, int write_lock, int wait_lock)
{
    struct flock slock;
    int cmd;
    
    slock.l_type = F_UNLCK;
	slock.l_whence = SEEK_SET;
    slock.l_start = 0;
    slock.l_len = 0;
    if (wait_lock) 
    {
        cmd = F_SETLKW;
    }
    else 
    {
        cmd = F_SETLK;
    }
    
    return fcntl(fd, cmd, &slock);
}

int mobile_send_fun_ext(int nChannel, char *stream, int nSize)
{
	int nRet = -1;
	int fd = -1;
//	char buffer[40] = {0};
	
	if (stream == NULL)
	{
		return -1;
	}
	if (nSize <= 0)
	{
		return -1;
	}
	
	fd = open(MOBILE_PICTURE, O_CREAT | O_WRONLY);
	if (fd < 0)
	{
		printf("Can not open the file: mobile.jpg.\n");

		return -1;
	}
	//fcntl(fd , F_SETLKW, file_lock(F_WRLCK, SEEK_SET));
	//nRet = flock(fd, LOCK_SH);
	//nRet = start_lock_file(fd, 0, 0);
	//if (nRet < 0)
	//{
	//	printf("Can not lock the file(%d %s): mobile.jpg.\n", errno, strerror(errno));
	//	close(fd);
	//	return -1;
	//}
	
	nRet = write(fd, stream, nSize);
	if (nRet != nSize)
	{
		printf("Can not write the file(%d %s): mobile.jpg.\n", errno, strerror(errno), nRet);
		stop_lock_file(fd, 1, 0);
		close(fd);
		return -1;
	}
	//writeConfigFile(WEBSERVER_MOBILE_CONFIG, 0, nRet);	
	//fcntl(fd , F_SETLKW, file_lock(F_UNLCK, SEEK_SET));
	
	//flock(fd, LOCK_UN);
	//stop_lock_file(fd, 0, 0);
	
	close(fd);
	//sprintf(buffer, "cp  /mnt/mtd/dvs/mobile/tmpfs/mobile.jpg  /mnt/mtd/dvs/mobile.jpg");
	//system(buffer);
	
	return 0;
}

int mobileVideoEncFunc()
{
	int i = 0;
	int ret = -1;
	int size = 0;
	char *stream = NULL;
	unsigned long ptr = 0;
		
	stream = (char *)malloc(128000);
	if (stream == NULL)
	{
		printf("Can not allocate memory(128000)!\n");
		return -1;
	}
	
	#if 1
	printf("MOBILE_PICTURE create .....\n");
	if (access(MOBILE_PICTURE, F_OK) == -1) {
		if ( mkfifo(MOBILE_PICTURE, 0777) != 0) {
		return -1;
		}
	}
	
	system("mkdir /mnt/mtd/dvs/www/tmpfs");
	system("ln -s /mnt/mtd/dvs/mobile/tmpfs/mobile.jpg /mnt/mtd/dvs/www/tmpfs/mobile.jpg");
	#endif


	while (g_mobile_module_run_flag)
	{
		//printf("mobileVideoEncFunc ...\n");
		
		if (g_video_max_channel == 1)
		{
			if (g_mobile_module_pause_flag)
			{
				sleep(1);

				continue;
			}
		}
		else
		{
			if (g_venc_module_pause_flag[0] && g_venc_module_pause_flag[1] && g_venc_module_pause_flag[2] && g_venc_module_pause_flag[3])
			{
				sleep(1);
								
				continue;	
			}
		}

		// 采集VADC的视频数据
		size = 0;
		ret = videoInGetStream(0, stream, &size);
		if (ret < 0)
		{
			continue;
		}
			
		for (i=0; i<g_video_max_channel; i++)
		{	
			if (g_venc_module_pause_flag[i])
			{
				continue;
			}	
			
			// 将视频数据送入编码器, 并取出码流
			ret = videoEncGetJpeg(i, 3, stream, &size);	// 3: Mobile
			if (ret < 0)
			{
				continue;
			}
			if (size <= 0)
			{
				continue;
			}
			// 传送视频码流
			//send_mkfifo_mobile_picture(i, stream, size);
  			//printf("readConfigFile(WEBSERVER_MOBILE_CONFIG) = %d\n", readConfigFile(WEBSERVER_MOBILE_CONFIG));
			//if(readConfigFile(WEBSERVER_MOBILE_CONFIG))
			//ret = mobile_send_fun(i, stream, size);
			ret = mobile_send_fun_ext(i, stream, size);
			//ret = write(fp, stream, size);
			if (ret < 0)
			{
				usleep(10*1000);	// 0.1S 
			}
			//usleep(100*1000);
		}
		
		// 释放VIN的码流
		videoInReleaseStream(0);
		//usleep(10*1000);	// 0.1S 
	}

	if (stream)
	{
		free(stream);
		stream = NULL;	
	}
	close(fp);
	pthread_exit(NULL);
	return 0;
}
#endif

int videoMDFunc()
{
	int i = 0;
	int ret = -1;
	int size = 0;
	MD_STATUS status;

	while (g_venc_module_run_flag)
	{
		if (g_video_max_channel == 1)
		{
			if (g_venc_module_pause_flag[0])
			{
				sleep(1);

				continue;
			}
		}
		else
		{
			if (g_venc_module_pause_flag[0] && g_venc_module_pause_flag[1] && g_venc_module_pause_flag[2] && g_venc_module_pause_flag[3])
			{
				sleep(1);
				
				continue;	
			}
		}

		for (i=0; i<g_video_max_channel; i++)
		{	
			if (g_venc_module_pause_flag[i])
			{
				printf("videoMDFunc ....\n");
				usleep(1000*100);
				continue;
			}
		
			// 获取视频移动
			ret = videoEncGetMDStatus(i, 0, &status);
			if (ret == 1)
			{
				putVideoInMotionStatus(i);
			}
		}
		usleep(40*1000);
	}

	pthread_exit(NULL);
	
	return 0;	
}

int videoEncModuleStart()
{
	int ret = -1;
	pthread_t threadID;	
	
	g_venc_module_run_flag = 1;
	printf("%s %d&&&&\n", __func__, __LINE__);
	#if 1
	ret = pthread_create(&threadID, NULL, (void *)videoEncFunc1, NULL);
	if (ret < 0)
	{
		g_venc_module_run_flag = 0;
		return -1;
	}
	usleep(10);
	#endif

	#if 0
	ret = pthread_create(&threadID, NULL, (void *)videoEncFunc2, NULL);
	if (ret < 0)
	{
		g_venc_module_run_flag = 0;
		return -1;
	}
	usleep(10);
	#endif

#if 0
	ret = pthread_create(&threadID, NULL, (void *)videoMDFunc, NULL);
	if (ret < 0)
	{
		g_venc_module_run_flag = 0;
		return -1;
	}
	
	usleep(10);
#endif

 
#if  0
	g_mobile_module_run_flag = 1;
	ret = pthread_create(&threadID, NULL, (void *)mobileVideoEncFunc, NULL);
	if (ret < 0)
	{
		g_mobile_module_run_flag = 0;
		return -1;
	}
	usleep(10);
#endif

	return 0;
}

#ifdef MOBILE_VIEW
int	videoMobileModuleStop()
{
	g_mobile_module_run_flag = 0;
	
	return 0;
}

int	videoMobileModulePause()
{
	g_mobile_module_pause_flag = 1;
	
	return 0;
}

int	videoMobileModuleResume()
{
	g_mobile_module_pause_flag = 0;
	
	return 0;
}
	
#endif

int videoEncModuleStop()
{
	g_venc_module_run_flag = 0;
	
	return 0;
}

int videoEncModulePause(int channel)
{
	if (channel<0 || channel>=g_video_max_channel)
	{
		return -1;
	}
	if (g_venc_module_pause_flag[channel] == 1)
	{
		return -1;
	}
	else
	{
		g_venc_module_pause_flag[channel] = 1;		
		return 0;
	}
}

int videoEncModuleResume(int channel)
{
	if (channel<0 || channel>=g_video_max_channel)
	{
		return -1;
	}
	
	g_venc_module_pause_flag[channel] = 0;
	
	return 0;
}

int videoEncModulePauseAll()
{
	int i = 0;
	
	for (i=0; i<g_video_max_channel; i++)
	{
		g_venc_module_pause_flag[i] = 1;
	}
	
	return 0;
}

int videoEncModuleResumeAll()
{
	int i = 0;
	
	for (i=0; i<g_video_max_channel; i++)
	{
		g_venc_module_pause_flag[i] = 0;
	}
	
	return 0;
}

int videoJpegSnapShot(int nChannel, int type)
{
	if (nChannel<0 || nChannel>=g_video_max_channel)
	{
		return -1;
	}
	
	g_jpeg_snapshot_channel = nChannel;
	g_jpeg_upload_type = type;
	
	//printf("videoJpegSnapShot: %d %d\n", nChannel, type);

	return 0;
}

int getTimeString(char *buffer, int mode)
{
	int ret = -1;
	unsigned long year = 0;
	unsigned long month = 0;
	unsigned long day = 0;
	unsigned long hour = 0;
	unsigned long minute = 0;
	unsigned long second = 0;

	ret = getSystemTime(&year, &month, &day, &hour, &minute, &second);
	if (!ret)
	{
		switch (mode)
		{
		case 0:
			sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
			break;
			
		case 1:
			sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", month, day, year, hour, minute, second);
			break;
			
		case 2:
			sprintf(buffer, "%02d/%02d/%04d %02d:%02d:%02d", day, month, year, hour, minute, second);
			break;
			
		default:
			sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
			break;
		}
		
	}
	else
	{
		return -1;
	}

	return 0;
}

int videoOsdUpdateFun()
{
	int i = 0;
	int ret = -1;	

	unsigned long year = 0;
	unsigned long month = 0;
	unsigned long day = 0;
	unsigned long hour = 0;
	unsigned long minute = 0;
	unsigned long second = 0;
	char buffer[64];
	
	VIDEO_ENC_PARAM encParam;
	
	memset(buffer, 0, 64);
				
	while (g_osd_module_run_flag)
	{
		if (g_osd_module_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		memset(buffer, 0, 64);

		/*
		ret = getSystemTime(&year, &month, &day, &hour, &minute, &second);
		if (!ret)
		{
			sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);			
		}
		*/
		
		for (i=0; i<g_video_max_channel; i++)
		{
		//	printf("g_osd_param[i].TimeOSD.x = %d,g_osd_param[i].TimeOSD.y = %d\n\n", g_osd_param[i].TimeOSD.x, g_osd_param[i].TimeOSD.y);
			encParam.param.encOSD.nChannel = i;
			//encParam.param.encOSD.nShow = g_time_osd_flag[i];
			encParam.param.encOSD.nShow = g_osd_param[i].TimeOSD.bShow;
			encParam.param.encOSD.nIndex = 0;
			encParam.param.encOSD.data.nxPos = g_osd_param[i].TimeOSD.x;
			encParam.param.encOSD.data.nyPos = g_osd_param[i].TimeOSD.y;
			encParam.param.encOSD.data.color = g_osd_param[i].TimeOSD.nColor;
			getTimeString(buffer, g_osd_param[i].TimeOSD.nFormat);
			memcpy(encParam.param.encOSD.data.data, buffer, 32);

			videoEncSetup(i, 0, venc_setup_osd, &encParam);
			videoEncSetup(i, 1, venc_setup_osd, &encParam);
		}
		

		// BitRate
		for (i=0; i<g_video_max_channel; i++)
		{
			memset(buffer, 0, 64);

			encParam.param.encOSD.nChannel = i;
			//encParam.param.encOSD.nShow = g_bitrate_osd_flag[i];
			encParam.param.encOSD.nShow = g_osd_param[i].BitsOSD.bShow;
			encParam.param.encOSD.nIndex = 1;
			encParam.param.encOSD.data.nxPos = g_osd_param[i].BitsOSD.x;
			encParam.param.encOSD.data.nyPos = g_osd_param[i].BitsOSD.y;	// 32
			//encParam.param.encOSD.data.color = g_osd_bits_color;
			//encParam.param.encOSD.data.color = 0xFFFFFF;
			encParam.param.encOSD.data.color = g_osd_param[i].BitsOSD.nColor;
			sprintf(buffer, "BR:%4d kps, FR:%2d fps", g_nBitRate[i][0]/1000, g_nFrameRate[i][0]);
			memcpy(encParam.param.encOSD.data.data, buffer, 32);
					
			videoEncSetup(i, 0, venc_setup_osd, &encParam);
			
			memset(buffer, 0, 64);

			encParam.param.encOSD.nChannel = i;
			encParam.param.encOSD.nShow = g_osd_param[i].BitsOSD.bShow;
			encParam.param.encOSD.nIndex = 1;
			encParam.param.encOSD.data.nxPos = g_osd_param[i].BitsOSD.x;
			encParam.param.encOSD.data.nyPos = g_osd_param[i].BitsOSD.y;	// 32
			encParam.param.encOSD.data.color = g_osd_param[i].BitsOSD.nColor;
			sprintf(buffer, "BR:%4d kps, FR:%2d fps", g_nBitRate[i][1]/1000, g_nFrameRate[i][1]);
			memcpy(encParam.param.encOSD.data.data, buffer, 32);

			videoEncSetup(i, 1, venc_setup_osd, &encParam);
		}	
	
#ifdef DEMO
		for (i=0; i<g_video_max_channel; i++)
		{
			encParam.param.encOSD.nChannel = i;
			encParam.param.encOSD.nShow = 1;
			encParam.param.encOSD.nIndex = 2;
			encParam.param.encOSD.data.nxPos = 100;
			encParam.param.encOSD.data.nyPos = 100;
			//encParam.param.encOSD.data.color = g_osd_title_color[i];
			encParam.param.encOSD.data.color = 0xFFFFFF;
			strcpy(encParam.param.encOSD.data.data, "lvjh");
					
			videoEncSetup(i, 0, venc_setup_osd, &encParam);
		}
#endif	
		
		usleep(100);
	}
	
	pthread_exit(NULL);
	
	return 0;
}

int videoOsdModuleStart()
{
	int i = 0;
	int j = 0;
	int ret = -1;
	OSD_PARAM osd;
	VIDEO_ENC_PARAM vencParam;
	pthread_t threadID;	
	
	for (i=0; i<g_video_max_channel; i++)
	{
		if (g_venc_module_run_flag)
		{						
			g_osd_module_run_flag = 1;

			#if 0
    		getOsdParam(i, &osd);

    		// DATE/TIME
    		vencParam.param.encOSD.nChannel = i;
    		vencParam.param.encOSD.nIndex = 0;

			vencParam.param.encOSD.data.nxPos = 16;
    		vencParam.param.encOSD.data.nyPos = 16;
			//vencParam.param.encOSD.data.color = g_osd_time_color;
			vencParam.param.encOSD.data.color = osd.TimeOSD.nColor;
			getTimeString(vencParam.param.encOSD.data.data, osd.TimeOSD.nFormat);

    		ret = videoEncSetup(i, 0, venc_setup_osd, &vencParam);
			#endif
		}
		
		ret = pthread_create(&threadID, NULL, (void *)videoOsdUpdateFun, NULL);
		if (ret < 0)
		{
			g_osd_module_run_flag = 0;
			
			return -1;
		}
			
		usleep(100);
	}
	
	return 0;	
}

int videoOsdModuleStop()
{
	if (g_osd_module_run_flag)
	{
		g_osd_module_run_flag = 0;	

		return 0;
	}
	else
	{
		return -1;
	}
}

int videoOsdModulePause()
{
	if (g_osd_module_run_flag)
	{
		return -1;
	}
	
	if (g_osd_module_pause_flag == 0)
	{
		return -1;
	}
	else
	{
		g_osd_module_pause_flag = 1;		
		return 0;
	}
}

int videoOsdModuleResume()
{
	g_osd_module_pause_flag = 0;
	
	return 0;
}

int hi3511VideoStart()
{
	int i = 0;
	int ret = -1;
	VIDEO_STANDARD_PARAM standard;
	VENC_PARAM encParam;
	VIDEO_ENC_PARAM vencParam;
	int videoStd = 0;

	getVideoInStandardParam(0, &standard);
	// 初始化VADC
	vadcDrv_SetStandard(0, standard.nStandard&0x0F);	// 其中最后一位才是视频制式
	videoStd = standard.nStandard&0x0F;

	ret = videoInInit(Hi3511VinInfo);
	if (ret < 0)
	{
		//printf("videoInInit(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
			
	// 打开VIN
	ret = videoInOpen(0);
	if (ret < 0)
	{
		//printf("videoInOpen(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}

	memset(&encParam, 0, sizeof(VENC_PARAM));
	getVideoEncParam(0, 0, &encParam);
	//判断视频格式
	switch (encParam.nEncodeHeight)
	{
	case 300:
#ifdef HD_CMOS
			printf("ok!!!\n");
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(0, XXVGA);
			getVideoFlipParam(0, &flipParam);
			vadcDrv_SetImageFlip(0, flipParam.nFlip);
			getVideoMirrorParam(0, &mirrorParam);
			vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
			getVideoHzParam(0, &hzParam);
			vadcDrv_SetImageHz(0, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(0, &vinAttr);
			vadcDrv_SetBrightness(0, vinAttr.nBrightness);
			vadcDrv_SetHue(0, vinAttr.nHue);
			vadcDrv_SetContrast(0, vinAttr.nContrast);
			vadcDrv_SetSaturation(0, vinAttr.nSaturation);
			standard.nStandard = 0x09000000;
			printf("standard.nStandard = 0x09000000\n");
#endif
			break;
	case 576:
	case 480:
		if (encParam.nEncodeWidth == 640)
		{
			//printf("****************640*480>>>>>>>>>>>\n");
			standard.nStandard = 0x04000000;
			
			#ifdef HD_CMOS
				printf("ok!!!\n");
				VIDEO_FLIP_PARAM flipParam;
				VIDEO_MIRROR_PARAM mirrorParam;
				VIDEO_HZ_PARAM hzParam;
				VIDEO_IN_ATTR vinAttr;
				
				vadcDrv_SetImageFormat(0, XXVGA);
				getVideoFlipParam(0, &flipParam);
				vadcDrv_SetImageFlip(0, flipParam.nFlip);
				getVideoMirrorParam(0, &mirrorParam);
				vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
				getVideoHzParam(0, &hzParam);
				vadcDrv_SetImageHz(0, hzParam.nHz);
				
				// Add the code by lvjh, 2009-05-27
				getVideoInAttrParam(0, &vinAttr);
				vadcDrv_SetBrightness(0, vinAttr.nBrightness);
				vadcDrv_SetHue(0, vinAttr.nHue);
				vadcDrv_SetContrast(0, vinAttr.nContrast);
				vadcDrv_SetSaturation(0, vinAttr.nSaturation);
				standard.nStandard = 0x09000000;
				printf("standard.nStandard = 0x09000000\n");
			#endif
		}
		#ifdef HD_CMOS
			else if(encParam.nEncodeWidth == 720)
			{
				printf("ok!!!\n");
				VIDEO_FLIP_PARAM flipParam;
				VIDEO_MIRROR_PARAM mirrorParam;
				VIDEO_HZ_PARAM hzParam;
				VIDEO_IN_ATTR vinAttr;
				
				vadcDrv_SetImageFormat(0, XXVGA);
				getVideoFlipParam(0, &flipParam);
				vadcDrv_SetImageFlip(0, flipParam.nFlip);
				getVideoMirrorParam(0, &mirrorParam);
				vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
				getVideoHzParam(0, &hzParam);
				vadcDrv_SetImageHz(0, hzParam.nHz);
				
				// Add the code by lvjh, 2009-05-27
				getVideoInAttrParam(0, &vinAttr);
				vadcDrv_SetBrightness(0, vinAttr.nBrightness);
				vadcDrv_SetHue(0, vinAttr.nHue);
				vadcDrv_SetContrast(0, vinAttr.nContrast);
				vadcDrv_SetSaturation(0, vinAttr.nSaturation);
				standard.nStandard = 0x08000000;		
			
			}
		#endif
		
		else
		{
			standard.nStandard |= 0x00;
			if (videoStd)
			{
				encParam.nEncodeHeight = 480;
			}
			else
			{
				encParam.nEncodeHeight = 576;
			}
		}
		break;
		
	case 288:
	case 240:
		if (encParam.nEncodeWidth == 320)
		{
			standard.nStandard = 0x05000000;
			
			#ifdef HD_CMOS
				//printf("****************320*240>>>>>>>>>>>\n");
				VIDEO_FLIP_PARAM flipParam;
				VIDEO_MIRROR_PARAM mirrorParam;
				VIDEO_HZ_PARAM hzParam;
				VIDEO_IN_ATTR vinAttr;
				
				vadcDrv_SetImageFormat(0, XXVGA);
				getVideoFlipParam(0, &flipParam);
				vadcDrv_SetImageFlip(0, flipParam.nFlip);
				getVideoMirrorParam(0, &mirrorParam);
				vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
				getVideoHzParam(0, &hzParam);
				vadcDrv_SetImageHz(0, hzParam.nHz);
				
				// Add the code by lvjh, 2009-05-27
				getVideoInAttrParam(0, &vinAttr);
				vadcDrv_SetBrightness(0, vinAttr.nBrightness);
				vadcDrv_SetHue(0, vinAttr.nHue);
				vadcDrv_SetContrast(0, vinAttr.nContrast);
				vadcDrv_SetSaturation(0, vinAttr.nSaturation);
				standard.nStandard = 0x05000000;
				printf("standard.nStandard = 0x05000000\n");
			#endif
		}
		else
		{
			if (encParam.nEncodeWidth==720 || encParam.nEncodeWidth==704)
			{
				standard.nStandard |= 0x01000000;
			}
			else
			{
				standard.nStandard |= 0x02000000;
			}
			if (videoStd)
			{
				encParam.nEncodeHeight = 240;
			}
			else
			{
				encParam.nEncodeHeight = 288;
			}
		}
		break;
		
	case 144:
	case 120:
	case 112:
	case 128:
		standard.nStandard |= 0x03000000;
		if (videoStd)
		{
			encParam.nEncodeHeight = 128;
		}
		else
		{
			encParam.nEncodeHeight = 144;
		}
		break;

	case 1200:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(0, UXVGA);
			getVideoFlipParam(0, &flipParam);
			vadcDrv_SetImageFlip(0, flipParam.nFlip);
			getVideoMirrorParam(0, &mirrorParam);
			vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
			getVideoHzParam(0, &hzParam);
			vadcDrv_SetImageHz(0, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(0, &vinAttr);
			vadcDrv_SetBrightness(0, vinAttr.nBrightness);
			vadcDrv_SetHue(0, vinAttr.nHue);
			vadcDrv_SetContrast(0, vinAttr.nContrast);
			vadcDrv_SetSaturation(0, vinAttr.nSaturation);
		}
			
		standard.nStandard = 0x06000000;
		break;

	case 720:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(0, XXVGA);
			getVideoFlipParam(0, &flipParam);
			vadcDrv_SetImageFlip(0, flipParam.nFlip);
			getVideoMirrorParam(0, &mirrorParam);
			vadcDrv_SetImageMirror(0, mirrorParam.nMirror);
			getVideoHzParam(0, &hzParam);
			vadcDrv_SetImageHz(0, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(0, &vinAttr);
			vadcDrv_SetBrightness(0, vinAttr.nBrightness);
			vadcDrv_SetHue(0, vinAttr.nHue);
			vadcDrv_SetContrast(0, vinAttr.nContrast);
			vadcDrv_SetSaturation(0, vinAttr.nSaturation);
		}
		
		standard.nStandard = 0x07000000;
		break;
				
	default:
		standard.nStandard |= 0x02000000;
		if (videoStd)
		{
			encParam.nEncodeHeight = 240;
		}
		else
		{
			encParam.nEncodeHeight = 288;
		}
		break;
	}
	
	// 设置VIN
	if (g_video_max_channel == 1)
	{
		ret = videoInSetup(0, standard.nStandard);
	}
	else
	{
		standard.nStandard |= 0x00;
		ret = videoInSetup(0, standard.nStandard);
	}
	if (ret < 0)
	{
		return -1;
	}

	// 启动VIN
	ret = videoInStart(0);
	if (ret)
	{
		return -1;
	}

	// 初始化视频编码器
	ret = videoEncInit(Hi3511VideoEncInfo);
	if (ret)
	{
		return -1;
	}
		
	// 打开视频编码器
	ret = videoEncOpen(0, 0);
	if (ret)
	{
		return -1;
	}

	// 设置视频编码器1
	//#if 0
	memcpy(&vencParam.param.encParam, &encParam, sizeof(VENC_PARAM));
	
	GetVideoNewEncAttr();
	#if 0
	ret = videoEncSetup(0, 0, venc_setup_enc_param, &vencParam);
	if (ret < 0)
	{
		return -1;
	}
	#endif
	setVideoFormat(0, 0, encParam.nEncodeWidth, encParam.nEncodeHeight, encParam.reserve);
	//#endif

#if 0
	// 设置视频编码器2
	memset(&encParam, 0, sizeof(VENC_PARAM));
	getVideoEncParam(0, 1, &encParam);
	
	#ifdef YIYUAN_VIDEO
	encParam.nEncodeWidth = 704;
	encParam.nEncodeHeight = 576;
	#endif
	
	memcpy(&vencParam.param.encParam, &encParam, sizeof(VENC_PARAM));
	ret = videoEncSetup(0, 1, venc_setup_enc_param, &vencParam);
	if (ret < 0)
	{
		return -1;
	}
	
	setVideoFormat(0, 1, encParam.nEncodeWidth, encParam.nEncodeHeight, encParam.reserve);
#ifdef MOBILE_VIEW
	// 设置视频编码器3
	memset(&encParam, 0, sizeof(VENC_PARAM));
	getVideoEncParam(0, 1, &encParam);
	memcpy(&vencParam.param.encParam, &encParam, sizeof(VENC_PARAM));
	ret = videoEncSetup(0, 2, venc_setup_enc_param, &vencParam);
	if (ret < 0)
	{
		return -1;
	}
#endif	

#endif


	// 启动视频编码器
	//#if 0	
	ret = videoEncStart(0, 0);
	if (ret)
	{
		return -1;
	}

	#if 1
	ret = videoEncModuleStart();
	if (ret < 0)
	{
		return -1;
	}
	#endif
	
	//#endif
	return 0;
}

// API
int videoEncModuleStartup(videoSendFun_t bitratevideoSendFunc, videoSendFun_t jpegvideoSendFunc)
{
	int i = 0;
	int j = 0;
	int ret = -1;

	VIDEO_ENC_PARAM param;
	VENC_PARAM vencparam;
	LOGO_PARAM logo;
	MASK_PARAM mask;
	VIDEO_MOTION_PARAM motion;
	char logoData[MAX_LOGO_DATA];


	if (bitratevideoSendFunc==NULL || jpegvideoSendFunc==NULL)
	{
		return -1;
	}
		
	// 初始化模块参数

	ret = videoEncModuleInit(1);

	if (ret < 0)
	{
		printf("videoEncModuleInit(%s %d): Failed!\n", __FILE__, __LINE__);
		return -1;
	}	

	// 设置视频模块的回调函数
	ret = videoEncModuleSetvideoSendFunc(bitratevideoSendFunc, jpegvideoSendFunc);
	if (ret < 0)	
	{		
		return -1;	
	}

	#if 0
//	ret = hi3511VideoStart();
	//videoInInit(Hi3511VinInfo);
	//videoEncInit(Hi3511VideoEncInfo);


	SAMPLE_VENC_720P_CLASSIC();
	videoEncModuleStart();
	#endif
	//ret = hi3511VideoStart();
	SAMPLE_VENC_720P_CLASSIC();
	//GetVideoStream(0);
	//videoEncModuleStart();
	printf("%s %d&&&&\n", __func__, __LINE__);
	
	#if 0
	//设置OSD\LOGO\MASK
	for (i=0; i<g_video_max_channel; i++)	
	{
		char string[64];
		//OSD
		getOsdParam(i, &g_osd_param[i]);
		if((abs((g_osd_param[i].TimeOSD.y - g_osd_param[i].BitsOSD.y)) < 20 )||(abs((g_osd_param[i].TimeOSD.y - g_osd_param[i].TitleOSD[0].y)) < 20 )||(abs((g_osd_param[i].BitsOSD.y - g_osd_param[i].TitleOSD[0].y)) < 20 )){
				g_osd_param[i].TimeOSD.x = 16;
				g_osd_param[i].TimeOSD.y = 16;
		
				g_osd_param[i].BitsOSD.x = 16;
				g_osd_param[i].BitsOSD.y =	36;
		
				getVideoEncParam(i, 0, &vencparam);
				g_osd_param[i].TitleOSD[0].x = vencparam.nEncodeWidth-16-strlen(g_osd_param[i].TitleOSD[j].sTitle)*8;
				g_osd_param[i].TitleOSD[0].y = vencparam.nEncodeHeight-32;
				getVideoEncParam(i, 1, &vencparam);
				g_osd_param[i].TitleOSD[1].x = vencparam.nEncodeWidth-16-strlen(g_osd_param[i].TitleOSD[j].sTitle)*8;
				g_osd_param[i].TitleOSD[1].y = vencparam.nEncodeHeight-32;
				setOsdParam(i, &g_osd_param[i]);
			}


		// DATE/TIME
		param.param.encOSD.nChannel = i;
		param.param.encOSD.nIndex = 0;
		param.param.encOSD.nShow = g_osd_param[i].TimeOSD.bShow;
		videoTimeOSD(i, param.param.encOSD.nShow);

		param.param.encOSD.data.nxPos = 16;
		param.param.encOSD.data.nyPos = 16;
		param.param.encOSD.data.color = g_osd_param[i].TimeOSD.nColor;
		getTimeString(param.param.encOSD.data.data, g_osd_param[i].TimeOSD.nFormat);

		ret = videoEncSetup(i, 0, venc_setup_osd, &param);
		ret = videoEncSetup(i, 1, venc_setup_osd, &param);

		
		// BITRATES
		getVideoEncParam(i, 0, &vencparam);
		
		param.param.encOSD.nChannel = i;
		param.param.encOSD.nIndex = 1;
		param.param.encOSD.nShow = g_osd_param[i].BitsOSD.bShow;

		videoBitRateOSD(i, param.param.encOSD.nShow);

		param.param.encOSD.data.nxPos = 16;
		param.param.encOSD.data.nyPos = 36;	// old: 32
		param.param.encOSD.data.color = g_osd_param[i].BitsOSD.nColor;
		sprintf(string, "BR: %4d kps, FR: %2d fps", vencparam.nBitRate/1100, vencparam.nFramerate);
		strcpy(param.param.encOSD.data.data, string);

		ret = videoEncSetup(i, 0, venc_setup_osd, &param);

		// 
		getVideoEncParam(i, 1, &vencparam);
		
		param.param.encOSD.nChannel = i;
		param.param.encOSD.nIndex = 1;
		param.param.encOSD.nShow = g_osd_param[i].BitsOSD.bShow;

		videoBitRateOSD(i, param.param.encOSD.nShow);

		param.param.encOSD.data.nxPos = 16;
		param.param.encOSD.data.nyPos = 36;	// old: 32
		param.param.encOSD.data.color = g_osd_param[i].BitsOSD.nColor;
		sprintf(string, "BR: %4d kps, FR: %2d fps", vencparam.nBitRate/1000, vencparam.nFramerate-500);
		strcpy(param.param.encOSD.data.data, string);
		
		ret = videoEncSetup(i, 1, venc_setup_osd, &param);
		
		// TITLE
		for (j=0; j<1; j++)
		{
			getVideoEncParam(i, 0, &vencparam);
			param.param.encOSD.nChannel = i;
			param.param.encOSD.nIndex = 2+j;
			param.param.encOSD.nShow = g_osd_param[i].TitleOSD[j].bShow;
			param.param.encOSD.data.nxPos = g_osd_param[i].TitleOSD[j].x;
			param.param.encOSD.data.nyPos = g_osd_param[i].TitleOSD[j].y;
			memcpy(param.param.encOSD.data.data, g_osd_param[i].TitleOSD[j].sTitle, strlen(g_osd_param[i].TitleOSD[j].sTitle));
			param.param.encOSD.data.color = g_osd_param[i].TitleOSD[j].nColor;
			param.param.encOSD.data.data[strlen(g_osd_param[i].TitleOSD[j].sTitle)] = '\0';
	
			ret = videoEncSetup(i, 0, venc_setup_osd, &param);
			
			getVideoEncParam(i, 1, &vencparam);
			param.param.encOSD.nChannel = i;
			param.param.encOSD.nIndex = 2+j;
			param.param.encOSD.nShow = g_osd_param[i].TitleOSD[j].bShow;
			param.param.encOSD.data.nxPos = g_osd_param[i].TitleOSD[j].x;
			param.param.encOSD.data.nyPos = g_osd_param[i].TitleOSD[j].y;
			memcpy(param.param.encOSD.data.data, g_osd_param[i].TitleOSD[j].sTitle, strlen(g_osd_param[i].TitleOSD[j].sTitle));
			param.param.encOSD.data.color = g_osd_param[i].TitleOSD[j].nColor;
			param.param.encOSD.data.data[strlen(g_osd_param[i].TitleOSD[j].sTitle)] = '\0';

			ret = videoEncSetup(i, 1, venc_setup_osd, &param);
	
		}
		
		// LOGO
		getLogoParam(i, &logo, logoData);

		param.param.encLogo.nChannel = i;
		param.param.encLogo.nIndex = 0;
		param.param.encLogo.nShow = logo.bShow;
		param.param.encLogo.data.nxPos = (logo.x/16)*16;
		param.param.encLogo.data.nyPos = (logo.y/16)*16;
		param.param.encLogo.data.nWidth = logo.nWidth;
		param.param.encLogo.data.nHeight = logo.nHeight;
		if (logo.nDataLen<MAX_LOGO_DATA && logo.nDataLen>0)
		{
			param.param.encLogo.data.data = logoData;
		}
		else
		{
			param.param.encLogo.data.data = NULL;
		}

		ret = videoEncSetup(i, 0, venc_setup_logo, &param);

		// MASK
		getMaskParam(i, &mask);

		for (j=0; j<1; j++)
		{
			ret = videoInSetMask(i, mask.VideoMask[0]);
		}
		
		// MOTION
		getVideoMotionParam(i, &motion);
		
		memcpy(&param.param.mdParam, &motion, sizeof(VIDEO_MOTION_PARAM));
		
		videoEncSetup(i, 0, venc_setup_md_area, &param);
		param.param.nMD = 1;
		videoEncSetup(i, 0, venc_setup_md, &param);
	}
	#endif
	
	
	// Time OSD Thread
#ifdef BOGUS
	ret = videoOsdModuleStart();
	if (ret < 0)
	{
		return -1;
	}
#endif /* BOGUS */

	return 0;
}

int videoBitRateOSD(int nChannel, int nOnFlag)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nOnFlag)
	{
		g_bitrate_osd_flag[nChannel] = 1;
	}
	else
	{
		g_bitrate_osd_flag[nChannel] = 0;
	}

	return 0;
}

int videoTimeOSD(int nChannel, int nOnFlag)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nOnFlag)
	{
		g_time_osd_flag[nChannel] = 1;
	}
	else
	{
		g_time_osd_flag[nChannel] = 0;
	}

	return 0;
}

int videoTimeOSDExt(int nChannel, OSD_PARAM osd)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	memcpy(&g_osd_param[nChannel], &osd, sizeof(OSD_PARAM));

	return 0;
}

#if 0
int get_snapshot_buffer_size(char *snapshot_buffer, int *snapshot_size)
{
	if(snapshot_buffer == NULL)
	return -1;
	//sem_wait(&g_snapshot_sem);
	memcpy(snapshot_buffer, g_snapshot_buffer, g_snapshot_buffer_size);
	*snapshot_size = g_snapshot_buffer_size;
	//sem_post(&g_snapshot_sem);
	
	//printf("##################################\n g_snapshot_buffer_size = %d snapshot_size  = %d\n", g_snapshot_buffer_size, *snapshot_size);
	return 0;
}
#endif

