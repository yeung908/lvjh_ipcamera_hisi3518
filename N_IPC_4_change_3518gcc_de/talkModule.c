#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "audioDecAppModule.h"
#include "audioStream.h"

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "talkModule.h"

//#define TALK_DEBUG

// 全局变量
static int g_talk_flag = 0;

#ifdef TALK_DEBUG
FILE *talk_fp = NULL;
#endif

int audioSendBitRateFun(int nChannel, void *stream, int *size)
{
	int ret = 0;
	int len = 0;
	
	// 发送到PC端，不需要包头，包头尺寸：16字节
	len = (*size)-16;
	
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	if (nChannel == 0)
	{
		ret = NETSDK_Talk_Send(stream+16, len);
	}
	
#ifdef DARWIN_TRANSMIT
	DARWIN_SendAudioData(nChannel, stream+16, len, 0x00, 0x00, 512);
#endif

	return 0;
}

int talkFun(unsigned long ipAddr, unsigned short port, char *buffer, int size)
{
	int ret = -1;
	
	if (buffer == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
//	printf("talkFun function starting ... \n");
   	ret = audioDecModuleSendStream(buffer, size);

#ifdef TALK_DEBUG
	fwrite(buffer, 1, size, talk_fp);
#endif
	
	return ret;
}

int UdpTalkStart(unsigned long ipAddr, unsigned short port, struct sockaddr_in addr)
{
	
	//NETSDK_SetTalkParam(1, 32, 8000); 
	// printf("Entry UDPNETSDK_Talk_Start \n");
	 UDPNETSDK_Talk_Start(ipAddr, addr);
	
	//g_talk_flag = 1;

#ifdef TALK_DEBUG
	talk_fp = fopen("talk.711", "w+b");
#endif
	 
	return 0;
}

int talkStart(unsigned long ipAddr, unsigned short port)
{
	
	//NETSDK_SetTalkParam(1, 32, 8000); 

	NETSDK_Talk_Start(ipAddr);
	
	//g_talk_flag = 1;

#ifdef TALK_DEBUG
	talk_fp = fopen("talk.711", "w+b");
#endif
	 
	return 0;
}

int talkStop()
{
	int ret = -1;
	//NETSDK_Talk_End();
	NETSDK_Talk_Stop();
	
	//g_talk_flag = 0;

#ifdef TALK_DEBUG
	fclose(talk_fp);
#endif
	
	return 0;
}

int getTalkStatus()
{
	return g_talk_flag;
}

