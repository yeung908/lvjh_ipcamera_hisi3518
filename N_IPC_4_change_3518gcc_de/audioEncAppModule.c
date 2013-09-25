#include <stdlib.h>

#include <stdio.h>

#include <pthread.h>

#include <semaphore.h>



#include "audioEncAppModule.h"

#include "audioIn/audioIn.h"

#include "audioEnc/audioEnc.h"

#include "audioEnc/audioEnc.h"

#include "audioStream.h"

#include "audioIn/ainHi3511.h"
#include "audioEnc/aencHi3511.h"

typedef struct
{
	unsigned int len;
	char *buffer;
	pthread_mutex_t mutex;
}audioBitrateBuffer;


// 全局变量

int g_aenc_max_channel = 1;

int g_aenc_module_flag = 0;

int g_aenc_module_run_flag = 0;

int g_aenc_module_pause_flag[4];

audioSendFun_t g_send_fun = NULL;
audioBitrateBuffer g_audioBuffer[4][2]; 

static int g_audio_enc_type = 0;

static int audioBitRateBufferInit(int nChannel, int nStreamType)
{
	int nRet = -1;

	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nStreamType<0 || nStreamType>=2)

	{

		return -1;

	}
	
	g_audioBuffer[nChannel][nStreamType].len = 0;
	g_audioBuffer[nChannel][nStreamType].buffer = NULL;
	g_audioBuffer[nChannel][nStreamType].buffer = (char *)malloc(AUDIO_ENC_BUFFER_SIZE);
	if (g_audioBuffer[nChannel][nStreamType].buffer == NULL)
	{
		return -1;
	}

	nRet = pthread_mutex_init(&g_audioBuffer[nChannel][nStreamType].mutex, NULL);
	if (nRet < 0)
	{
		printf("pthread_mutex_init: Failed!\n");
	}

	return 0;


}


static int audioBitRateBufferDeInit(int nChannel, int nStreamType)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nStreamType<0 || nStreamType>=2)

	{

		return -1;

	}
	
	g_audioBuffer[nChannel][nStreamType].len = 0;
		
	if (g_audioBuffer[nChannel][nStreamType].buffer)
	{
		free(g_audioBuffer[nChannel][nStreamType].buffer);
		g_audioBuffer[nChannel][nStreamType].buffer = NULL;
	}
		
	pthread_mutex_destroy(&g_audioBuffer[nChannel][nStreamType].mutex);

	

	return 0;
}

static int putAudioBitRateBuffer(int nChannel, int nStreamType, char *buffer, int size)
{
	int ret = -1;
	
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nStreamType<0 || nStreamType>=2)

	{

		return -1;

	}
	if (buffer == NULL)
	{
		return -1;
	}
	if (size<=0 || size>AUDIO_ENC_BUFFER_SIZE)
	{
		return -1;
	}	
	
	pthread_mutex_lock(&g_audioBuffer[nChannel][nStreamType].mutex);
	
	if (g_audioBuffer[nChannel][nStreamType].len+size > AUDIO_ENC_BUFFER_SIZE)
	{
		g_audioBuffer[nChannel][nStreamType].len = 0;
	}
	
	// 发送到PC端，不需要包头，包头尺寸：16字节
	memcpy(g_audioBuffer[nChannel][nStreamType].buffer+g_audioBuffer[nChannel][nStreamType].len, (char *)buffer+16, size-16);
	g_audioBuffer[nChannel][nStreamType].len += (size-16);

	//printf("putAudioBitRateBuffer: %d\n", g_audioBuffer[nChannel].len);

	pthread_mutex_unlock(&g_audioBuffer[nChannel][nStreamType].mutex);

	return 0;
}

int getAudioBitRateBuffer(int nChannel, int nStreamType, char *buffer, int *size)
{
	int ret = -1;

	if (size == NULL)
	{
		return -1;
	}	
	if (nChannel<0 || nChannel>=4)
	{
		*size = -1;
		return -1;
	}

	if (nStreamType<0 || nStreamType>=2)

	{

		*size = -1;

		return -1;

	}
	if (buffer == NULL)
	{
		*size = -1;
		return -1;
	}
	if (g_aenc_module_run_flag==0 || g_aenc_module_pause_flag[nChannel]==1)
	{
		*size = -1;
		return -1;
	}
	
	pthread_mutex_lock(&g_audioBuffer[nChannel][nStreamType].mutex);
	
	//printf("getAudioBitRateBuffer: %d\n", g_audioBuffer[nChannel][nStreamType].len);

	if (g_audioBuffer[nChannel][nStreamType].len > 0)
	{
		memcpy(buffer, g_audioBuffer[nChannel][nStreamType].buffer, g_audioBuffer[nChannel][nStreamType].len);
		*size = g_audioBuffer[nChannel][nStreamType].len;

		
		g_audioBuffer[nChannel][nStreamType].len = 0; 	// Delete the code by lvjh, 2009-06-25
	}
	else
	{
		*size = 0;
	}
	
	pthread_mutex_unlock(&g_audioBuffer[nChannel][nStreamType].mutex);

	return 0;
}


int audioEncModuleSetaudioSendFunc(audioSendFun_t senFunc)
{

	if (senFunc == NULL)

	{
		g_send_fun = NULL;
		return 0;

	}

	else

	{

		g_send_fun = senFunc;

		return 0;

	}

}


int audioEncModuleOpen(int maxChn, int encType)

{

	int ret = -1;

	int i = 0;

	int j = 0;

	AIN_PARAM ainParam;
	AENC_PARAM aencParam;

	

	if (maxChn<=0 || maxChn>4)

	{

		g_aenc_max_channel = 0;

		return -1;

	}

	else

	{

		g_aenc_max_channel = maxChn;

	}



	if (encType<0 || encType>=5)
	{

		g_audio_enc_type = 0;	

	}

	else
	{

		g_audio_enc_type = encType;

	}

	

	// 初始化AIN


	ret = audioInInit(Hi3511AinInfo);

	if (ret < 0)

	{

		printf("audioInInit(%s %d) Failed!\n", __FILE__, __LINE__);

		return -1;

	}

			

	// 打开VIN

	ret = audioInOpen(0);

	if (ret < 0)

	{

		printf("audioInOpen(%s %d) Failed!\n", __FILE__, __LINE__);

		return -1;

	}

	

	// 设置AIN

	ret = audioInSetup(0, &ainParam);

	if (ret < 0)

	{

		printf("audioInSetup(%s %d) Failed!\n", __FILE__, __LINE__);

		return -1;

	}

		

	// 启动AIN

	ret = audioInStart(0);

	if (ret < 0)

	{

		printf("audioInStart(%s %d) Failed!\n", __FILE__, __LINE__);

		return -1;

	}

		// 初始化音频编码器
	ret = audioEncInit(Hi3511AudioEncInfo);

	if (ret < 0)

	{

		printf("audioEncInit(%s %d) Failed!\n", __FILE__, __LINE__);

		goto exit;

	}

		

	for (i=0; i<g_aenc_max_channel; i++)

	{

		for (j=0; j<2; j++)

		{
			//audioBitRateBufferInit(i);

			audioBitRateBufferInit(i, j);

		}

		

		// 打开音频编码器

		ret = audioEncOpen(i, g_audio_enc_type);
		if (ret < 0)

		{

			printf("audioEncOpen(%s %d) Failed!\n", __FILE__, __LINE__);

			goto exit;

		}

		

		// 设置音频编码器

		ret = audioEncSetup(i, NULL);
		if (ret < 0)

		{

			printf("audioEncSetup(%s %d) Failed!\n", __FILE__, __LINE__);

			goto exit;

		}

		

		// 启动音频编码器

		ret = audioEncStart(i);
		if (ret < 0)

		{

			printf("audioEncStart(%s %d) Failed!\n", __FILE__, __LINE__);

			goto exit;

		}

	}

	

	for (i=0; i<g_aenc_max_channel; i++)

	{

		g_aenc_module_flag = 1;

		g_aenc_module_run_flag = 0;

		getAudioEncParam(i, &aencParam);
		if (aencParam.nOnFlag)
		{

			g_aenc_module_pause_flag[i] = 0;
		}
		else
		{
			g_aenc_module_pause_flag[i] = 1;
		}

	}



	return 0;

	

exit:

	audioInStop(0);

	audioInClose(0);

	

	for (i=0; i<g_aenc_max_channel; i++)

	{

		audioEncStop(i);

		audioEncClose(i);

		g_aenc_module_pause_flag[i] = 0;

	}

	

	g_aenc_module_flag = 0;

	g_aenc_module_run_flag = 0;

	

	return -1;	

}



int audioEncModuleClose()

{

	int i = 0;

	int j = 0;

	

	if (g_aenc_module_flag)

	{

		audioInStop(0);

		audioInClose(0);

			

		for (i=0; i<g_aenc_max_channel; i++)

		{			

			audioEncStop(i);

			audioEncClose(i);

			

			g_aenc_module_flag = 0;

			g_aenc_module_run_flag = 0;

			g_aenc_module_pause_flag[i] = 0;
			

			//audioBitRateBufferInit(i);

			for (j=0; j<2; j++)

			{

				audioBitRateBufferInit(i, j);

			}

		}

	}

		

	return 0;		

}



int audioEncFunc()

{

	int i = 0;

	int j = 0;

	int ret = -1;

	int size = 0;

	char *stream = NULL;

	unsigned long ptr = 0;
	unsigned long nStartTime = 0;

	

	stream = (char *)malloc(AUDIO_BUFFER_SIZE);

	if (stream == NULL)

	{

		return -1;

	}

	if (g_aenc_module_flag != 1)

	{

		return -1;

	}


	while (g_aenc_module_run_flag)

	{
		size = 0;
		usleep(1000);

		/*

		if (g_aenc_module_pause_flag)

		{

			sleep(1);

			continue;

		}
		*/
		if (g_aenc_max_channel == 1)

		{
#if 1
			if (g_aenc_module_pause_flag[0])

			{
				printf("g_aenc_module_pause_flag[0] %d \n ",g_aenc_module_pause_flag[0]);
				sleep(1);
				continue;

			}
#endif			

		}

		else

		{

			if (g_aenc_module_pause_flag[0] && g_aenc_module_pause_flag[1] && g_aenc_module_pause_flag[2] && g_aenc_module_pause_flag[3])

			{

				sleep(1);
				continue;	

			}

		}

		
		// 将解码器解出的PCM数据送入ADAC

		audioInGetStream(0, stream, &size);

	//	printf("audioInGetStream: %d\n", size);
		//sleep(1);
		
#if 1
		for (i=0; i<g_aenc_max_channel; i++)

		{	
			// 将码流送入解码器解码

		//	ret = audioEncGetStream(i, stream, &size);

			//printf("audioEncGetStream(%d): %p %d\n", ret, stream, size);
			//sleep(1);

			//putAudioBitRateBuffer(i, stream, size);

			

			for (j=0; j<2; j++)

			{

				putAudioBitRateBuffer(i, j, stream, size);

			}

			if (g_send_fun)
			{
				//printf("Entry g_send_fun function\n");
				//printf("channel = %d\n", i);

				g_send_fun(i, stream, &size);
			}

			// 释放解码器的码流

			audioEncReleaseStream(i);

#ifdef AUDIO_LOOP
			// test playback audio
			audioDecModuleSendStream(stream, size);
#endif

		}
#endif		

		

		// 释放VIN的码流

		audioInReleaseStream(0);
	}



	if (stream)

	{

		free(stream);

		stream = NULL;	

	}

	

	pthread_exit(NULL);

	

	return 0;

}



int audioEncModuleStart()

{

	int ret = -1;

	pthread_t threadID;	



	g_aenc_module_run_flag = 1;



	ret = pthread_create(&threadID, NULL, (void *)audioEncFunc, NULL);

	if (ret < 0)

	{

		g_aenc_module_run_flag = 0;
		

		return -1;

	}
	usleep(10);	

	

	return 0;

}



int audioEncModuleStop()

{

	g_aenc_module_run_flag = 0;

	

	return 0;

}



int audioEncModulePause(int nChannel)

{

	g_aenc_module_pause_flag[nChannel] = 1;		


	return 0;

}



int audioEncModuleResume(int nChannel)

{

	g_aenc_module_pause_flag[nChannel] = 0;

	

	return 0;

}



int audioEncModuleUpdate()

{

	return 0;

}



// API

int audioEncModuleStartup(int maxChn, int encType, audioSendFun_t sendFunc)
{

	int ret = -1;	

	ret = audioEncModuleSetaudioSendFunc(sendFunc);

	if (ret < 0)	

	{		

		return -1;	

	}

	ret = audioEncModuleOpen(maxChn, encType);

	if (ret < 0)	
	{

		audioEncModuleClose();

		return -1;	

	}

	ret = audioEncModuleStart();
	if (ret < 0)
	{
		audioEncModuleStop();
		audioEncModuleClose();
		return -1;	
	}
	return 0;
}
