#include <stdlib.h>
#include <stdio.h>

#include "audioEncModule.h"
#include "audioEnc.h"

static aencModuleInfo_t g_audioEncInfo = NULL;

int audioEncInit(aencModuleInfo_t aencInfo)
{
	if (aencInfo == NULL)
	{
		return -1;
	}
	
	g_audioEncInfo = aencInfo;
	
	return 0;
}

int audioEncOpen(int nChannel, int encType)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->open(nChannel, encType);
	
	return ret;
}

int audioEncClose(int nChannel)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->close(nChannel);
	
	return ret;
}

int audioEncSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->setup(opt, param);
	
	return ret;
}

int audioEncGetSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->getSetup(opt, param);
	
	return ret;
}

int audioEncStart(int nChannel)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->start(nChannel);
	
	return ret;
}

int audioEncStop(int nChannel)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->stop(nChannel);
	
	return ret;
}

int audioEncGetStream(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->getStream(nChannel, stream, size);

	return ret;
}

int audioEncReleaseStream(int nChannel)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->releaseStream(nChannel);
	
	return ret;
}

int audioEncGetAudio(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->getAudio(nChannel, stream, size);
	
	return ret;	
}

int audioEncReleaseAudio(int nChannel)
{
	int ret = -1;
	
	if (g_audioEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioEncInfo->releaseAudio(nChannel);
	
	return ret;	
}



