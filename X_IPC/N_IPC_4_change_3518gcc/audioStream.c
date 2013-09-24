#include <stdlib.h>
#include <stdio.h>
#include "audioDecAppModule.h"

#include "audioStream.h"

AUDIO_STREAM_BUFFER *audioBufferOpen(int size)
{
	int ret = -1;
	AUDIO_STREAM_BUFFER *audioStream = NULL;
	pthread_mutexattr_t mutex_attr;
	
	if (size <= 0)
	{
		return NULL;	
	}
	
	audioStream = (AUDIO_STREAM_BUFFER *)malloc(sizeof(AUDIO_STREAM_BUFFER));
	if (audioStream == NULL)
	{
		return NULL;
	}
	
	memset(audioStream, 0, sizeof(AUDIO_STREAM_BUFFER));
	
	audioStream->nPacketCount = 0;
	audioStream->nReadPos = 0;
	audioStream->nWritePos = 0;
	audioStream->nValidSize = 0;
	audioStream->nCurSize = 0;
	//audioStream->nTotalSize = size;
	audioStream->buffer = NULL;
	
	audioStream->buffer = (char *)malloc(size);
	if (audioStream->buffer == NULL)
	{
		free(audioStream);
		audioStream = NULL;
		return NULL;
	}
	else
	{
		audioStream->nTotalSize = size;
	}
	
	// 创建互锁量，保护数据
	pthread_mutexattr_init(&mutex_attr);
#ifndef CYG	
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
	pthread_mutex_init(&audioStream->mutex, &mutex_attr);
	pthread_mutexattr_destroy(&mutex_attr);
	
	return audioStream;	
}

int audioBufferClose(AUDIO_STREAM_BUFFER *audioStream)
{
	if (audioStream == NULL)
	{
		return -1;
	}
	
	if (audioStream->buffer)
	{
		free(audioStream->buffer);
		audioStream->buffer = NULL;
	}
	
	pthread_mutex_destroy(&audioStream->mutex);
	
	audioStream->nPacketCount = 0;
	audioStream->nReadPos = 0;
	audioStream->nWritePos = 0;
	audioStream->nValidSize = 0;
	audioStream->nCurSize = 0;
	audioStream->nTotalSize = 0;

	free(audioStream);
	
	return 0;
}

int audioBufferReset(AUDIO_STREAM_BUFFER *audioStream)
{
	if (audioStream == NULL)
	{
		return -1;
	}
	pthread_mutex_lock(&audioStream->mutex);
	
	audioStream->nPacketCount = 0;
	audioStream->nReadPos = 0;
	audioStream->nWritePos = 0;
	audioStream->nValidSize = 0;
	audioStream->nCurSize = 0;
	
	pthread_mutex_unlock(&audioStream->mutex);
	
	return 0;
}

static int audioBufferReleaseOnePacket(AUDIO_STREAM_BUFFER *audioStream)
{
	int n = 0;
	char *temp = NULL;
	AUDIO_STREAM_HEADER header;
	
	if (audioStream == NULL)
	{
		return -1;
	}
		
	if (audioStream->buffer == NULL)
	{
		return -1;
	}

	//printf("audioBufferReleaseOnePacket() start!\n");

	// 先读取音频包头
	n = audioStream->nTotalSize - audioStream->nReadPos;
	if (n >= sizeof(AUDIO_STREAM_HEADER))
	{
		memcpy(&header, audioStream->buffer+audioStream->nReadPos, sizeof(AUDIO_STREAM_HEADER));
	}
	else
	{
		temp = (char *)&header;
		memcpy(temp, audioStream->buffer+audioStream->nReadPos, n);
		memcpy(temp+n, audioStream->buffer, sizeof(AUDIO_STREAM_HEADER)-n);
	}
	
	if (n >= (header.u32Len+sizeof(AUDIO_STREAM_HEADER)))
	{
		audioStream->nReadPos += (header.u32Len+sizeof(AUDIO_STREAM_HEADER));
	}
	else
	{
		audioStream->nReadPos = header.u32Len+sizeof(AUDIO_STREAM_HEADER) - n;
	}	
	
	audioStream->nValidSize -= (header.u32Len+sizeof(AUDIO_STREAM_HEADER));

	//printf("audioBufferReleaseOnePacket() stop!\n");

	return 0;
}

int audioBufferSendOnePacket(AUDIO_STREAM_BUFFER *audioStream, void *stream, int size)
{
	int n = 0;
	int remainSize = 0;
	char *temp = NULL;
	AUDIO_STREAM_HEADER *header = NULL;
	//int over_flag = 0;  //add by zj
	
	//printf("audioBufferSendOnePacket: %x %d\n", stream, size);

	if (stream == NULL)
	{
	
		printf("send audio one--1\n");
		return -1;
	}
	if (audioStream == NULL)
	{
	
		printf("send audio one--2\n");
		return -1;
	}
	if (size <= 0 || size > audioStream->nTotalSize)
	{
	
		printf("send audio one--3\n");
		return -1;
	}
	if (audioStream->buffer == NULL)
	{
	
		printf("send audio one--4\n");
		return -1;
	}
	if (size <= sizeof(AUDIO_STREAM_HEADER))
	{
	
		printf("send audio one--5\n");
		return -1;
	}
	if (size > audioStream->nTotalSize)
	{
	
		printf("send audio one--6\n");
		return -1;
	}
	
	pthread_mutex_lock(&audioStream->mutex);
	
	// 判断数据流是否合法
	header = (AUDIO_STREAM_HEADER *)stream;
	if (header->u32Len<=0 || header->u32Len>audioStream->nTotalSize  ||  size != header->u32Len+sizeof(AUDIO_STREAM_HEADER))
	{
		//printf("audio data len: %d\n", header->u32Len);
		
		printf("send audio one--7\n");
		pthread_mutex_unlock(&audioStream->mutex);
		return -1;
	}	
	
	// BUFFER满了，则释放音频包，直到BUFFER可以容纳当前的数据包
	n = audioStream->nTotalSize - audioStream->nValidSize;
	if (n < size)
	{			
		while (n < size)
		{
			audioBufferReleaseOnePacket(audioStream);
			n = audioStream->nTotalSize - audioStream->nValidSize;
			//over_flag = 1;
		}		
	}
	/***********************************cut buffer data***************************************/
	//add by zhangjing 裁剪buffer数据
	n = audioStream->nPacketCount;
	if (n > AUDIO_BALANCE_START)
	{		
		printf("--------cuted audio buffer-----------\n");
		while (n > AUDIO_BALANCE_END)
		{
			if(!audioBufferReleaseOnePacket(audioStream)){
				audioStream->nPacketCount --;
				n--;
				sem_wait(&g_audio_dec_sem_store);
				sem_post(&g_audio_dec_sem_empty);
			}else{
				break;
			}
			//over_flag = 1;
		}		
	}
	/*=================================================*/
	
	n = audioStream->nTotalSize - audioStream->nValidSize;
	if (n >= size)
	{
		remainSize = audioStream->nTotalSize - audioStream->nWritePos;
		if (remainSize >= size)
		{			
			memcpy(audioStream->buffer + audioStream->nWritePos, stream, size);
			audioStream->nWritePos += size;
		}
		else
		{			
			temp = (char *)stream;
			memcpy(audioStream->buffer + audioStream->nWritePos, temp, remainSize);
			memcpy(audioStream->buffer, temp+remainSize, size-remainSize);			
			audioStream->nWritePos = size - remainSize;
		}
		
		audioStream->nValidSize += size;
		audioStream->nPacketCount ++ ;
	}

	pthread_mutex_unlock(&audioStream->mutex);

	//printf("audioBufferSendOnePacket(): OK!\n");
	//if(over_flag == 1) return -1;
	return 0;
}

int audioBufferSendPackets(AUDIO_STREAM_BUFFER *audioStream, void *stream, int size)
{
	int ret = -1;
	int len = 0;
	void *buffer = stream;
	AUDIO_STREAM_HEADER *header = NULL;
	//int over_flag = 0;

	if (stream == NULL)
	{
		printf("send audio sss--1\n");
		return -1;
	}
	if (size <= 0)
	{
		printf("send audio sss--2\n");
		return -1;
	}
	if (audioStream == NULL)
	{
		printf("send audio sss--3\n");
		return -1;
	}	
	if (audioStream->buffer == NULL)
	{
		printf("send audio sss--4\n");
		return -1;
	}
	if (size > audioStream->nTotalSize)
	{
		printf("send audio sss--5\n");
		return -1;
	}
	
	// 判断数据流是否合法
	header = (AUDIO_STREAM_HEADER *)buffer;
	if (header->u32FrameNum==0 || header->u32Len<=0 || header->u32Len+sizeof(AUDIO_STREAM_HEADER) > audioStream->nTotalSize)
	{
		printf("send audio sss--6\n");
		return -1;
	}

	len = 0;	
	
	while (len < size)
	{
		if (header->u32Len <= 0 || header->u32Len+sizeof(AUDIO_STREAM_HEADER) > audioStream->nTotalSize)
		{
			printf("send audio sss--7\n");
			return -1;
		}

		ret = audioBufferSendOnePacket(audioStream, buffer, header->u32Len+sizeof(AUDIO_STREAM_HEADER));		
		//if(ret)	over_flag = 1;
		if (len >= size)
		{
			break;
		}
		else
		{
			len +=  (header->u32Len+sizeof(AUDIO_STREAM_HEADER));
			buffer += len;
			header = (AUDIO_STREAM_HEADER *)buffer;			
		}
	}
	//if(over_flag == 1) return -1;
	return 0;
}

int audioBufferGetOnePacket(AUDIO_STREAM_BUFFER *audioStream, void *stream, int *size)
{
	int n = 0;
	char *temp = NULL;
	AUDIO_STREAM_HEADER header;
	
	if (stream == NULL)
	{
		printf("get buffer --5\n");
		return -1;
	}
	if (size == NULL)
	{
		printf("get buffer --6\n");
		return -1;
	}
	if (audioStream == NULL)
	{
		printf("get buffer --7\n");
		return -1;
	}
	
	//printf("audioBufferGetOnePacket(0): OK!\n");


	pthread_mutex_lock(&audioStream->mutex);
	
	if (audioStream->buffer == NULL)
	{
		pthread_mutex_unlock(&audioStream->mutex);
		printf("get buffer --1\n");
		return -1;
	}
	if (audioStream->nPacketCount < 1)
	{
		pthread_mutex_unlock(&audioStream->mutex);		
		printf("get buffer --2\n");
		return -1;
	}
	
	// Add the code by zhb, 2007-9-19
	if (audioStream->nTotalSize - audioStream->nReadPos < 0)
	{
		pthread_mutex_unlock(&audioStream->mutex);
		printf("get buffer --3\n");
		return -1;	
	}
	
	// 先读取音频包头
	n = audioStream->nTotalSize - audioStream->nReadPos;
	if (n >= sizeof(AUDIO_STREAM_HEADER))
	{
		memcpy(&header, audioStream->buffer+audioStream->nReadPos, sizeof(AUDIO_STREAM_HEADER));
	}
	else
	{
		temp = (char *)&header;
		memcpy(temp, audioStream->buffer+audioStream->nReadPos, n);
		memcpy(temp+n, audioStream->buffer, sizeof(AUDIO_STREAM_HEADER)-n);
	}

	//printf("audioStream: %d\n", header.u32Len);
	
	if (sizeof(AUDIO_STREAM_HEADER)+header.u32Len > audioStream->nTotalSize)
	{
		printf("get buffer -p%d-%d-4\n",header.u32Len,audioStream->nTotalSize);
		pthread_mutex_unlock(&audioStream->mutex);
		return -1;
	}
	
	// 再读取数据内容
	if (n >= sizeof(AUDIO_STREAM_HEADER)+header.u32Len)
	{
		memcpy(stream, audioStream->buffer + audioStream->nReadPos, header.u32Len+sizeof(AUDIO_STREAM_HEADER));
		audioStream->nReadPos +=(header.u32Len+sizeof(AUDIO_STREAM_HEADER));
	}
	else
	{
		temp = (char *)stream;
		memcpy(stream, audioStream->buffer + audioStream->nReadPos, n);
		memcpy(stream+n, audioStream->buffer, sizeof(AUDIO_STREAM_HEADER)+header.u32Len-n);
		audioStream->nReadPos =header.u32Len+sizeof(AUDIO_STREAM_HEADER)-n;
	}	
	
	audioStream->nValidSize -= (sizeof(AUDIO_STREAM_HEADER)+header.u32Len);
	audioStream->nPacketCount -- ;
	if (audioStream->nPacketCount < 1)
	{
		audioStream->nReadPos = 0;
		audioStream->nWritePos = 0;
	}
	
	*size = header.u32Len+sizeof(AUDIO_STREAM_HEADER);	

	pthread_mutex_unlock(&audioStream->mutex);

	//printf("audioBufferGetOnePacket(1): OK!\n");
	
	return 0;
}

