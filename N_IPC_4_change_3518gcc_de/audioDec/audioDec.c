#include <stdlib.h>
#include <stdio.h>

#include "audioDecModule.h"
#include "audioDec.h"

static adecModuleInfo_t g_audioDecInfo = NULL;

int audioDecInit(adecModuleInfo_t adecInfo)
{
	if (adecInfo == NULL)
	{
		return -1;
	}
	
	g_audioDecInfo = adecInfo;
	
	return 0;
}

int audioDecOpen(int nChannel, int encType)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->open(nChannel, encType);
	
	return ret;
}

int audioDecClose(int nChannel)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->close(nChannel);
	
	return ret;
}

int audioDecSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->setup(opt, param);
	
	return ret;
}

int audioDecGetSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->getSetup(opt, param);
	
	return ret;
}

int audioDecStart(int nChannel)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->start(nChannel);
	
	return ret;
}

int audioDecStop(int nChannel)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->stop(nChannel);
	
	return ret;
}

int audioDecSendStream(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->sendStream(nChannel, stream, size);
	
	return ret;
}

int audioDecReleaseStream(int nChannel)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->releaseStream(nChannel);
	
	return ret;
}

int audioDecGetAudio(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->getAudio(nChannel, stream, size);
	
	return ret;	
}

int audioDecReleaseAudio(int nChannel)
{
	int ret = -1;
	
	if (g_audioDecInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioDecInfo->releaseAudio(nChannel);
	
	return ret;	
}



