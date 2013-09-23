#ifndef __AUDIO_STREAM_H_
#define __AUDIO_STREAM_H_

#include <pthread.h>

#define AUDIO_BALANCE_START   35
#define AUDIO_BALANCE_END 	20
typedef struct
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;

}AUDIO_STREAM_HEADER;

typedef struct
{	
	unsigned int nPacketCount;
	unsigned int nReadPos;
	unsigned int nWritePos;
	unsigned int nValidSize;
	unsigned int nCurSize;
	unsigned int nTotalSize;
	unsigned char *buffer;
	
	pthread_mutex_t mutex;

}AUDIO_STREAM_BUFFER;

AUDIO_STREAM_BUFFER *audioBufferOpen(int size);
int audioBufferClose(AUDIO_STREAM_BUFFER *audioStream);
int audioBufferReset(AUDIO_STREAM_BUFFER *audioStream);
int audioBufferSendOnePacket(AUDIO_STREAM_BUFFER *audioStream, void *stream, int size);
int audioBufferSendPackets(AUDIO_STREAM_BUFFER *audioStream, void *stream, int size);
int audioBufferGetOnePacket(AUDIO_STREAM_BUFFER *audioStream, void *stream, int *size);

#endif
