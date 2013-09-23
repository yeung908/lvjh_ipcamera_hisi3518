#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>


#include  "../audioIn/audioInModule.h"  //mbl add

#include "audioDecModule.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"

static int g_Hi3511_adec_open_flag = 0;
static int g_adec_chn_flag[MAX_ADEC_CHANNEL];
static ADEC_CHN g_adec_chn[MAX_ADEC_CHANNEL];

static AUDIO_FRAME_INFO_S  stAudioFrameInfo;
#if 0//G726
//mody by lv start add------------
g726_state_t *g_State726_32 = NULL; //for g726_32 

FILE *g_pDecFile;
//mody by lv end add------------
#endif
static unsigned long g_audio_seq = 0;

int Hi3511AudioDecOpen(int nChannel, int encType)
{
	HI_S32 s32ret;
	AIO_ATTR_S stAttr;
	ADEC_CHN_ATTR_S stAdecAttr;
	ADEC_ATTR_ADPCM_S stAdpcm;
	AENC_ATTR_G726_S stG726;
	
	if (nChannel<0 || nChannel>=MAX_ADEC_CHANNEL)
	{
		return -1;	
	}
#if 0//G726	
	stAdecAttr.enType = PT_G726;
	stAdecAttr.u32BufSize = 8;
	stAdecAttr.u32BufSize = 10;      ////mody by lv add------------
	
	stAdecAttr.enMode = ADEC_MODE_PACK;
	stAdecAttr.pValue = &stG726;
	stG726.enG726bps = G726_16K;

	s32ret = HI_MPI_ADEC_CreateChn(0, &stAdecAttr);
	if (s32ret)
    {
        printf("create adnc chn %d err:0x%x\n", 0, s32ret);
        return -1;
    }

//mody by lv start add------------
	#if 1
	g_State726_32 = (g726_state_t *)malloc(sizeof(g726_state_t));
	g_State726_32 = g726_init(g_State726_32, 8000*4);

	g_pDecFile = fopen("/mnt/mtd/dvs/mobile/tmpfs/g726.dec", "w+");
	if(!g_pDecFile){
		printf("fopen error\n");
	}
	#endif
//mody by lv end add------------
#endif 
	g_adec_chn_flag[nChannel] = 1;
		
	return 0;
} 

int Hi3511AudioDecClose(int nChannel)
{
	HI_S32 s32ret;
	ADEC_CHN AdChn = 0;

	if (nChannel<0 || nChannel>=MAX_ADEC_CHANNEL)
	{
		return -1;	
	}


#if  0   //mbl old:worked
	/* release audio frame */
	s32ret = HI_MPI_ADEC_ReleaseData(AdChn, &stAudioFrameInfo);
	if (HI_SUCCESS != s32ret)
	{
		printf("adec release data %d  err:0x%x\n",0, s32ret);
		return s32ret;
	}	
	// 关闭通道编码器
	if (g_adec_chn_flag[nChannel])
	{
		HI_MPI_ADEC_DestroyChn(nChannel);
		g_adec_chn_flag[nChannel] = 0;
	}
	
#endif
	

	
	return 0;
}

int Hi3511AudioDecSetup(int opt, void *param)
{
	return 0;
}

int Hi3511AudioDecGetSetup(int opt, void *param)
{
	return 0;
}

int Hi3511AudioDecStart(int nChannel)
{	
	return 0;
}

int Hi3511AudioDecStop(int nChannel)
{
	return 0;
}

typedef struct 
{
	unsigned long long u64TimeStamp;
	unsigned int u32FrameNum;
	unsigned int u32Len;
		
}AUDIO_DEC_HEADER;

typedef struct
{
	short nFlag;
	short nLen;

}HI_G711_HEADER;

//mody by lv start add------------
#if 1
int Hi3511AudioDecSendStream(int nChannel, void *stream, int *size)
{
	HI_S32 s32ret;
	AUDIO_FRAME_S stStream;
	AUDIO_DEC_HEADER *audioHeader; // delete by liujw 12-3-5

	char ucOutBuff[1024];
	unsigned long currentPosition = 0;
	unsigned long audioSize = 0;
	int iRet = 0;
	 
    if (stream==NULL || size==NULL)
    {
    	return -1;
    }
    
	audioHeader = (AUDIO_DEC_HEADER *)stream;
	
	if (audioHeader->u32Len >= 512)
	{
		return -1;	
	}

#ifdef G726	
	//printf("audioHeader->u32Len = %d\n", audioHeader->u32Len);
	fwrite(stream+16, 1, audioHeader->u32Len, g_pDecFile);
	 while(currentPosition < audioHeader->u32Len){
		iRet = g726_decode(g_State726_32, (short*)(ucOutBuff+audioSize),(stream+16+currentPosition), 80);
		currentPosition += 80;
		audioSize += iRet;
	}
	memcpy(stream, ucOutBuff, audioSize*2);
	memcpy(stStream.aData, stream, audioSize*2);
	*size = audioSize;
	stStream.enBitwidth = 1;
	stStream.enSoundmode = 0;
	stStream.u64TimeStamp = 0;
	stStream.u32Seq = g_audio_seq++;
	stStream.u32Len = audioSize*2;
	memset(stream, 0, 1024*40);
	memcpy(stream, &stStream, sizeof(AUDIO_FRAME_S));
	//fwrite(ucOutBuff, 1, audioSize, g_pDecFile);
#else  //G711--zhangjing--g711解码
	G711Decoder((short *)stStream.aData,stream+sizeof(AUDIO_DEC_HEADER),(audioHeader->u32Len*2),0);

	*size = audioHeader->u32Len;
	stStream.enBitwidth = 1;
	stStream.enSoundmode = 0;
	stStream.u64TimeStamp = 0;
	stStream.u32Seq = g_audio_seq++;
	stStream.u32Len = audioHeader->u32Len*2;
	//printf("stStream-u32len:%d\n",stStream.u32Len);
	memset(stream, 0,sizeof(AUDIO_FRAME_S));
	memcpy(stream, &stStream, sizeof(AUDIO_FRAME_S));
#endif
	return 0;
}
#endif
//mody by lv end add------------

#if 0  //mbl old:worked
int Hi3511AudioDecSendStream(int nChannel, void *stream, int *size)
{
	HI_S32 s32ret;
	AUDIO_STREAM_S stStream;
	AUDIO_DEC_HEADER *audioHeader; // delete by liujw 12-3-5
	//AUDIO_FRAME_INFO_S stAudioFrameInfo; // delete by liujw 12-3-5
    
    if (stream==NULL || size==NULL)
    {
    	return -1;
    }
    
	audioHeader = (AUDIO_DEC_HEADER *)stream;
	
	if (audioHeader->u32Len >= 512)
	{
		return -1;	
	}
	
	stStream.pStream = stream+sizeof(AUDIO_DEC_HEADER);
	stStream.u32Len = audioHeader->u32Len;
	
	s32ret = HI_MPI_ADEC_SendStream(nChannel, (AUDIO_FRAME_S *)&stStream);
	if (s32ret)
	{
		printf("send audio frame to adec chn %d err:%x\n", nChannel, s32ret);	
		return -1;
	}
	else
	{
		//printf("send audio frame to adec chn %d OK \n", nChannel);	
	}
	
	s32ret = HI_MPI_ADEC_GetData(nChannel, &stAudioFrameInfo);
	if (HI_SUCCESS != s32ret )
	{
		printf("get data from aenc chn %d fail \n", nChannel);	
		return -1;
	}
	else
	{
		//printf("get data from aenc chn %d OK \n", nChannel);	
	}
	
	memcpy(stream, stAudioFrameInfo.pstFrame, sizeof(AUDIO_FRAME_S));
	
	HI_MPI_ADEC_ReleaseData(nChannel, &stAudioFrameInfo);
		
	return 0;
}
#endif


int Hi3511AudioDecReleaseStream(int nChannel)
{
	return 0;
}

int Hi3511AudioDecGetAudio(int nChannel, void *stream, int *size)
{
	return 0;
}

int Hi3511AudioDecReleaseAudio(int nChannel)
{
	return 0;
}

static struct adecModuleInfo Hi3511AudioDecInfoStruct = 
{
	open:			Hi3511AudioDecOpen,
	close:			Hi3511AudioDecClose,
	setup:			Hi3511AudioDecSetup,
	getSetup:			Hi3511AudioDecGetSetup,
	start:				Hi3511AudioDecStart,
	stop:				Hi3511AudioDecStop,
	sendStream:		Hi3511AudioDecSendStream,
	releaseStream:		Hi3511AudioDecReleaseStream,
	getAudio:			Hi3511AudioDecGetAudio,
	releaseAudio:		Hi3511AudioDecReleaseAudio,
	
};

adecModuleInfo_t Hi3511AudioDecInfo = &Hi3511AudioDecInfoStruct;

