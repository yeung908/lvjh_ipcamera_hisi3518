#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include "audioDecAppModule.h"
#include "audioOut/audioOut.h"
#include "audioDec/audioDec.h"
#include "audioDec/audioDec.h"
#include "audioStream.h"
#include "audioOut/aoutHi3511.h"
#include "audioDec/adecHi3511.h"

// 全局变量
int g_adec_module_flag = 0;
int g_adec_module_run_flag = 0;
int g_adec_module_pause_flag = 0;

static int g_audio_dec_type = 0;

AUDIO_STREAM_BUFFER *g_audio_dec_stream = NULL;
static int g_talk_states = 0;  // 1-开启状态 0-关闭状态

unsigned int pack_count_send = 0;//add by zhangjing-test
unsigned int send_count_size = 0;//add by zhangjing -test
unsigned int pack_count_get = 0;//add by zhangjing-test
unsigned int get_count_size = 0;//add by zhangjing-test
int audioDecModuleSemInit()
{
	int ret;
	g_adec_module_pause_flag = 1;    //zhangjing 2013-05-30
	sem_post(&g_audio_dec_sem_store);   //暂停解码播放线程
	usleep(500000);                                          //带解码播放线程暂停后复位计数信号量
	sem_destroy(&g_audio_dec_sem_empty);
	sem_destroy(&g_audio_dec_sem_store);
	// 创建信号量

	ret = sem_init(&g_audio_dec_sem_empty, 0, AUDIO_DEC_BUFFER_SIZE/176);
	if (ret)
	{
		printf("sem_init(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
	ret = sem_init(&g_audio_dec_sem_store, 0, 0);
	if (ret)
	{
		printf("sem_init(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}	
	audioBufferReset(g_audio_dec_stream);//add by zhangjing 2013-06-03
	g_talk_states = 0;//add by zhangjinging 2013-06-09  用于对讲互斥
	return 0;
}
/*static open buffer---zj--2013-06-02*/
//add by zhangjing --静态分配音频buffer
int audioReciveBufferOpen()
{
	#if 1  
	// 分配音频BUFFER
	g_audio_dec_stream = audioBufferOpen(AUDIO_DEC_BUFFER_SIZE);
	if (g_audio_dec_stream == NULL)
	{
		return -1;
	}
	return 0;
	#endif
}
int audioGetTalkState()
{
	return g_talk_states;
}
int audioReciveBufferClose()
{
	if (g_audio_dec_stream)
	{
		audioBufferClose(g_audio_dec_stream);
	}
		
}

int audioDecModuleOpen(int decType)
{
	int ret = -1;
	AOUT_PARAM aoutParam;

	if (decType<0 || decType>=1000)//mody by lv old value:4
	{
		g_audio_dec_type = 0;	
	}
	else
	{
		g_audio_dec_type = decType;
	}
	
	#if 0   //add for debug by liujw 12-3-2
	// 分配音频BUFFER
	g_audio_dec_stream = audioBufferOpen(AUDIO_DEC_BUFFER_SIZE);
	if (g_audio_dec_stream == NULL)
	{
		goto exit;
	}
	#endif
	audioBufferReset(g_audio_dec_stream);//add by zhangjing 2013-06-09
	g_talk_states = 1;//add by zhangjing 2013-06-09  用于对讲互斥
	#if 0  //delete by liujw 12-3-5
	// 创建信号量
	ret = sem_init(&g_audio_dec_sem_empty, 0, 0);
	if (ret)
	{
		printf("sem_init(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	#endif
#if  1//close aout--zhangjing debug2013-05-31
	// 初始化AOUT

	ret = audioOutInit(Hi3511AoutInfo);
	if (ret)
	{
		printf("audioDecInit(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	
		
	// 打开AOUT
	ret = audioOutOpen(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutOpen(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	
	
	#if 1
	// 设置AOUT
	ret = audioOutSetup(TALK_CHANNEL, &aoutParam);
	if (ret)
	{
		printf("audioOutSetup(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	#endif

	#if 1
	// 启动AOUT
	ret = audioOutStart(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutStart(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}

	#endif
#endif
	
	#if 1
	// 初始化音频解码器

	ret = audioDecInit(Hi3511AudioDecInfo);
	if (ret)
	{
		printf("audioDecInit(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	
	// 打开音频解码器
	ret = audioDecOpen(TALK_CHANNEL, g_audio_dec_type);
	if (ret)
	{
		printf("audioDecOpen(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	
	// 设置音频解码器
	ret = audioDecSetup(TALK_CHANNEL, &aoutParam);
	if (ret)
	{
		printf("audioDecSetup(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}
	
	// 启动音频解码器
	ret = audioDecStart(TALK_CHANNEL);
	if (ret)
	{
		printf("audioDecStart(%s %d) Failed!\n", __FILE__, __LINE__);
		goto exit;
	}	

	#endif
	
	//g_adec_module_flag = 1;
	//g_adec_module_run_flag = 0;
	g_adec_module_pause_flag = 0;

	return 0;
	
exit:
	audioDecStop(TALK_CHANNEL);
	audioDecClose(TALK_CHANNEL);
	
	audioOutStop(TALK_CHANNEL);
	audioOutClose(TALK_CHANNEL);
	
	/*if (g_audio_dec_stream)
	{
		audioBufferClose(g_audio_dec_stream);
	}*/
		
	sem_destroy(&g_audio_dec_sem_empty);
	sem_destroy(&g_audio_dec_sem_store);
	
	
	g_adec_module_flag = 0;
	g_adec_module_run_flag = 0;
	g_adec_module_pause_flag = 1;
	
	return -1;	
}

int audioDecModuleClose()
{
	if (g_adec_module_flag)
	{
		audioDecStop(TALK_CHANNEL);
		audioDecClose(TALK_CHANNEL);
	
		audioOutStop(TALK_CHANNEL);
		audioOutClose(TALK_CHANNEL);
		/*
		if (g_audio_dec_stream)
		{
			audioBufferClose(g_audio_dec_stream);
		}
		*/	
		sem_destroy(&g_audio_dec_sem_empty);
		sem_destroy(&g_audio_dec_sem_store);
	}
	
	g_adec_module_flag = 0;
	g_adec_module_run_flag = 0;
	g_adec_module_pause_flag = 1;
		
	return 0;		
}

char g_buffer[8192];

typedef struct
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;

}AUDIO_HEADER;

int audioDecModuleSendStream(void *stream, int size)
{
	int ret = -1;
	int sem_value = 0;
	AUDIO_HEADER *header = NULL;

	if (g_audio_dec_stream==NULL || stream==NULL || size<=0)
	{
		return -1;	
	}
	if (size >= 10240)    //mody by lv old value:8192
	{
		printf("audioDecModuleSendStream: data too large, %d\n", size);
		
		return -1;
	}

	header = (AUDIO_HEADER *)g_buffer;
	header->u64TimeStamp = 1;
	header->u32FrameNum = 1;
	header->u32Len = size;

	memcpy(g_buffer+sizeof(AUDIO_HEADER), stream, size);
	//printf("audioDecModuleSendStream function starting :%d \n",size);

	if(sem_trywait(&g_audio_dec_sem_empty)) return 0;
#ifdef AUDIO_LOOP
	ret = audioBufferSendPackets(g_audio_dec_stream, stream, size);
#else
	ret = audioBufferSendPackets(g_audio_dec_stream, g_buffer, size+sizeof(AUDIO_HEADER));
#endif
	if (ret < 0)
	{
		printf("audioBufferSendOnePacket(%d): Failed\n", size);
		sem_post(&g_audio_dec_sem_empty);
		return -1;
	}
	else
	{
		//printf("audioBufferSendOnePacket(%d): OK\n", size);
		pack_count_send ++;
		send_count_size += size+sizeof(AUDIO_HEADER);
	}
//	if(!sem_getvalue(&g_audio_dec_sem_empty,&sem_value)){
		//printf("sem valude %d\n",sem_value);
		//if(sem_value == 0 ) 
	sem_post(&g_audio_dec_sem_store);
//	}
	//usleep(100);
	//printf("sem_post: %d\n", ret);

	return 0;
}

int audioDecModuleGetStream(void *stream, int *size)
{
	int ret = -1;
	
	if (g_audio_dec_stream==NULL || stream==NULL || size==NULL)
	{
		return -1;	
	}
	
	ret = audioBufferGetOnePacket(g_audio_dec_stream, stream, size);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;	
}

int audioDecFunc()
{
	int ret = -1;
	int ret_send = -1;
	int size = 0;
	char *stream = NULL;
	struct timeval tv;
	unsigned long long nStartTimeCount = 0;
	unsigned long long nCurTimeCount = 0;
	unsigned int sem_empty_value = 0;
	unsigned int sem_store_value=0;
	
	
	stream = (char *)malloc(AUDIO_DEC_BUFFER_SIZE);
	if (stream == NULL)
	{
		return -1;
	}
	if (g_adec_module_flag != 1)
	{
		return -1;
	}

	while (g_adec_module_run_flag)
	{
		if (g_adec_module_pause_flag)
		{
			sleep(1);
			//printf("play audio pause\n");
			continue;
		}
		//printf("audioDecModule before sem_wait line 266!\n");//add code by liujw 2012-0302
		//printf("------1-----\n");
		sem_wait(&g_audio_dec_sem_store);   //chang by zhangjing 2013-05-30
	//	printf("------2-----\n");
		// 从BUFFER获取音频码流
		gettimeofday(&tv, NULL);
		nStartTimeCount = tv.tv_sec*1000000+tv.tv_usec;
		ret = audioDecModuleGetStream(stream, &size);
		if (!ret)
		{
			//printf("audioDecModuleGetStream: %d OK!\n", size);
			
			// 将码流送入解码器解码
			get_count_size += size;
			
			ret = audioDecSendStream(TALK_CHANNEL, stream, &size);
			if (ret < 0)
			{
				//sleep(1);	// add code by liujw 20120229
				sem_post(&g_audio_dec_sem_empty); 
				continue;
			}			
			//printf("audioDecSendStream(): %d\n", size);
		
			// 将解码器解出的PCM数据送入ADAC
			audioOutGetStream(TALK_CHANNEL, stream, &size);
		
			// 释放AOUT的码流
			audioOutReleaseStream(TALK_CHANNEL);
					
			// 释放解码器的码流
			audioDecReleaseStream(TALK_CHANNEL);
		}
		else
		{
			sem_post(&g_audio_dec_sem_store);
			continue;
			//printf("audioDecModuleGetStream: %d Failed!\n", size);
		}
		gettimeofday(&tv, NULL);
		nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
		//printf("------3-----\n");
		sem_post(&g_audio_dec_sem_empty);   //chang by zhangjing 2013-05-30
	//	printf("------4-----\n");
		pack_count_get ++;
		if(pack_count_get % 100 == 0){
			printf("--send pack:%d\tsize:%d--\t\n--get pack:%d\t,get size:%d--\nbuffer packet:%d\n,time_sec:%d\n",pack_count_send,send_count_size,\
																		     pack_count_get,get_count_size,\
																		     g_audio_dec_stream->nPacketCount,\
																		     nCurTimeCount-nStartTimeCount);
			sem_getvalue(&g_audio_dec_sem_empty,&sem_empty_value);
			sem_getvalue(&g_audio_dec_sem_store,&sem_store_value);
			printf("sem_empty value:%d---sem_store value:%d\n",sem_empty_value,sem_store_value);
		}
		
	}
//printf("------5-----\n");
	if (stream)
	{
		free(stream);
		stream = NULL;
	}
	
	pthread_exit(NULL);
	
	return 0;
}

int audioDecModuleStart()
{
	int ret = -1;
	pthread_t threadID;
	g_adec_module_flag = 1;
	g_adec_module_run_flag = 1;
	g_adec_module_pause_flag = 0;

	// 创建信号量
	ret = sem_init(&g_audio_dec_sem_empty, 0, AUDIO_DEC_BUFFER_SIZE/176);
	if (ret)
	{
		printf("sem_init(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
	ret = sem_init(&g_audio_dec_sem_store, 0, 0);
	if (ret)
	{
		printf("sem_init(%s %d) Failed!\n", __FILE__, __LINE__);
		return -1;
	}
	
	ret = pthread_create(&threadID, NULL, (void *)audioDecFunc, NULL);
	if (ret)
	{
		g_adec_module_run_flag = 0;
		return -1;
	}
	
	return 0;
}

int audioDecModuleStop()
{
	if (g_adec_module_run_flag == 0)
	{
		return -1;
	}
	else
	{
		g_adec_module_run_flag = 0;
		sem_post(&g_audio_dec_sem_empty);	
		sem_post(&g_audio_dec_sem_store);	
		return 0;
	}
}

int audioDecModulePause()
{
	if (g_adec_module_pause_flag == 1)
	{
		return -1;
	}
	else
	{
		g_adec_module_pause_flag = 1;
		//sem_post(&g_audio_dec_sem_empty);	
		return 0;
	}
}

int audioDecModuleResume()
{
	g_adec_module_pause_flag = 0;
}

int audioDecModuleUpdate()
{
	return 0;
}

// API
int audioDecModuleStartup(int decType)
{
	int ret = -1;
	//add code by ljh 
	#if 1
	ret =audioReciveBufferOpen();
	if (ret)	
	{
		audioReciveBufferClose();
		return -1;	
	}
	#endif
	
	#if 1
	ret = audioDecModuleStart();
	if (ret)
	{
		audioDecModuleStop();
		audioDecModuleClose();
		return -1;	
	}
	#endif
	
	return 0;
}
