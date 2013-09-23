#include <stdlib.h>
#include <stdio.h>

#include "audioOutModule.h"
#include "audioOut.h"

static aoutModuleInfo_t g_audioOutInfo = NULL;

int audioOutInit(aoutModuleInfo_t aoutInfo)
{
	if (aoutInfo == NULL)
	{
		return -1;
	}
	
	g_audioOutInfo = aoutInfo;
	
	return 0;
}

int audioOutOpen(int nChannel)
{
	int ret = -1;

	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->open(nChannel);
	
	return ret;
}

int audioOutClose(int nChannel)
{
	int ret = -1;
	
	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->close(nChannel);
	
	return ret;
}

int audioOutSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->setup(opt, param);
	
	return ret;
}

int audioOutGetSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->getSetup(opt, param);
	
	return ret;
}

int audioOutStart(int nChannel)
{
	int ret = -1;
	
	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->start(nChannel);
	
	return ret;
}

int audioOutStop(int nChannel)
{
	int ret = -1;
	
	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->stop(nChannel);
	
	return ret;
}

int audioOutGetStream(int nChannel, void *stream, int *size)
{
	int ret = -1;

	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->getStream(nChannel, stream, size);
	
	return ret;
}

int audioOutReleaseStream(int nChannel)
{
	int ret = -1;

	if (g_audioOutInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioOutInfo->releaseStream(nChannel);
	
	return ret;
}


