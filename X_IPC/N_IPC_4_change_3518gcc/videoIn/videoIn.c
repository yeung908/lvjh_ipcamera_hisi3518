#include <stdlib.h>
#include <stdio.h>

#include "videoInModule.h"
#include "videoIn.h"

static vinModuleInfo_t g_videoInInfo = NULL;

int videoInInit(vinModuleInfo_t vinInfo)
{
	if (vinInfo == NULL)
	{
		return -1;
	}
	
	g_videoInInfo = vinInfo;
	
	return 0;
}

int videoInOpen(int nChannel)
{
	int ret = -1;

	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->open(nChannel);
	
	return ret;
}

int videoInClose(int nChannel)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->close(nChannel);
	
	return ret;
}

int videoInSetup(int nChannel, int standard)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{		
		return -1;
	}
	
	ret = g_videoInInfo->setup(nChannel, standard);
	
	return ret;
}

int videoInGetSetup(int nChannel, int *standard)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->getSetup(nChannel, standard);
	
	return ret;
}

int videoInStart(int nChannel)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->start(nChannel);
	
	return ret;
}

int videoInStop(int nChannel)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->stop(nChannel);
	
	return ret;
}

int videoInGetStream(int nChannel, void *stream, int *size)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->getStream(nChannel, stream, size);
	
	return ret;
}

int videoInReleaseStream(int nChannel)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->releaseStream(nChannel);
	
	return ret;
}

int videoInSetMask(int nChannel, VIDEO_MASK mask)
{
	int ret = -1;
	
	if (g_videoInInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoInInfo->setMask(nChannel, mask);
	
	return ret;
}


