#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "audioEncModule.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"
/*--------------------see old program----------------------------*/
//mody by lv start add------------
#include <faac.h>

typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;
typedef char            _TCHAR;
//mody by lv end add------------

static int g_Hi3511_aenc_open_flag = 0;
static int g_aenc_chn_flag[MAX_AENC_CHANNEL] = {0, 0, 0, 0};


int Hi3511AudioEncOpen(int nChannel, int encType)
{
	HI_S32 s32ret;
	AENC_CHN_ATTR_S stAencAttr;
	AENC_ATTR_ADPCM_S stAdpcmAenc;
	ADEC_ATTR_G726_S stG726;

	g_aenc_chn_flag[nChannel] = 1;
	
	return 0;
} 

int Hi3511AudioEncClose(int nChannel)
{
	int i = 0;
	
	if (nChannel<0 || nChannel>=MAX_AENC_CHANNEL)
	{
		return -1;	
	}

	return 0;
}

int Hi3511AudioEncSetup(int opt, void *param)
{
	return 0;
}

int Hi3511AudioEncGetSetup(int opt, void *param)
{
	return 0;
}

int Hi3511AudioEncStart(int nChannel)
{	
	return 0;
}

int Hi3511AudioEncStop(int nChannel)
{
	return 0;
}

typedef struct 
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;
		
}AUDIO_HEADER;





#if 1
int Hi3511AudioEncGetStream(int nChannel, void *stream, int *size)
{
	HI_S32 s32ret;
    AUDIO_STREAM_S stStream;
    AUDIO_HEADER *audioHeader;
    AEC_FRAME_S stRefFrame;

	memcpy(&stRefFrame, stream, sizeof(AUDIO_FRAME_S));
	
	
	audioHeader = (AUDIO_HEADER *)stream;
	audioHeader->u64TimeStamp = stStream.u64TimeStamp;
	audioHeader->u32FrameNum = stStream.u32Seq;
	audioHeader->u32Len = stStream.u32Len;
	
	memcpy(stream+sizeof(AUDIO_HEADER), (char *)stStream.pStream, stStream.u32Len);
		
	*size = sizeof(AUDIO_HEADER) + audioHeader->u32Len;
	return 0;
}
#endif

int Hi3511AudioEncReleaseStream(int nChannel)
{
	return 0;
}

int Hi3511AudioEncGetAudio(int nChannel, void *stream, int *size)
{
	return 0;
}

int Hi3511AudioEncReleaseAudio(int nChannel)
{
	return 0;
}

static struct aencModuleInfo Hi3511AudioEncInfoStruct = 
{
	open:           Hi3511AudioEncOpen,
	close:          Hi3511AudioEncClose,
	setup:			Hi3511AudioEncSetup,
	getSetup:		Hi3511AudioEncGetSetup,
	start:          Hi3511AudioEncStart,
	stop:			Hi3511AudioEncStop,
	getStream:		Hi3511AudioEncGetStream,
	releaseStream:	Hi3511AudioEncReleaseStream,
	getAudio:		Hi3511AudioEncGetAudio,
	releaseAudio:	Hi3511AudioEncReleaseAudio,
	
};

aencModuleInfo_t Hi3511AudioEncInfo = &Hi3511AudioEncInfoStruct;
