#include <stdlib.h>
#include <stdio.h>

#include "audioInModule.h"
#include "audioIn.h"

static ainModuleInfo_t g_audioInInfo = NULL;

int audioInInit(ainModuleInfo_t ainInfo)
{
	if (ainInfo == NULL)
	{		
		return -1;
	}
	
	g_audioInInfo = ainInfo;
	
	return 0;
}

int audioInOpen(int nChannel)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->open(nChannel);
	
	return ret;
}

int audioInClose(int nChannel)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->close(nChannel);
	
	return ret;
}

int audioInSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->setup(opt, param);
	
	return ret;
}

int audioInGetSetup(int opt, void *param)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->getSetup(opt, param);
	
	return ret;
}

int audioInStart(int nChannel)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->start(nChannel);
	
	return ret;
}

int audioInStop(int nChannel)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->stop(nChannel);
	
	return ret;
}

int audioInGetStream(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->getStream(nChannel, stream, size);
	
	return ret;
}

int audioInReleaseStream(int nChannel)
{
	int ret = -1;
	
	if (g_audioInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_audioInInfo->releaseStream(nChannel);
	
	return ret;
}

