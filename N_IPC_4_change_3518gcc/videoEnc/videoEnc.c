#include <stdlib.h>
#include <stdio.h>

#include "videoEncModule.h"
#include "videoEnc.h"

static vencModuleInfo_t g_videoEncInfo = NULL;

int videoEncInit(vencModuleInfo_t vencInfo)
{
	if (vencInfo == NULL)
	{
		return -1;
	}
	
	g_videoEncInfo = vencInfo;
	
	return 0;
}

int videoEncOpen(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->open(nChannel, nStreamType);
	
	return ret;
}

int videoEncClose(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->close(nChannel, nStreamType);
	
	return ret;
}

int videoEncSetup(int nChannel, int nStreamType, int opt, void *param)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->setup(nChannel, nStreamType, opt, param);
	
	return ret;
}

int videoEncGetSetup(int nChannel, int nStreamType, int opt, void *param)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->getSetup(nChannel, nStreamType, opt, param);
	
	return ret;
}

int videoEncStart(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->start(nChannel, nStreamType);
	
	return ret;
}

int videoEncStop(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->stop(nChannel, nStreamType);
	
	return ret;
}

int videoEncGetStream(int nChannel, int nStreamType, void *stream, int *size, int fpH264File)
{
	int ret = -1;

	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->getStream(nChannel, nStreamType, stream, size, fpH264File);
	
	return ret;
}

int videoEncReleaseStream(int nChannel, int nStreamType)
{
	int ret = -1;

	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->releaseStream(nChannel, nStreamType);
	
	return ret;
}

int videoEncGetJpeg(int nChannel, int nStreamType, void *stream, int *size)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->getJpeg(nChannel, nStreamType, stream, size);
	
	return ret;	
}

int videoEncGetVideo(int nChannel, int nStreamType, void *stream, int *size)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->getVideo(nChannel, nStreamType, stream, size);
	
	return ret;	
}

int videoEncReleaseVideo(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->releaseVideo(nChannel, nStreamType);
	
	return ret;	
}

int videoEncRequestIFrame(int nChannel, int nStreamType)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->requestIFrame(nChannel, nStreamType);
	
	return ret;
}

int videoEncInserUserInfo(int nChannel, int nStreamType, void *info, int size)
{
	int ret = -1;
	
	if (g_videoEncInfo == NULL)
	{
		return -1;
	}
	
	ret = g_videoEncInfo->inserUserInfo(nChannel, nStreamType, info, size);
	
	return ret;	
}

int videoEncGetMDStatus(int nChannel, int nStreamType, MD_STATUS *status)
{
	int ret = -1;

	if (g_videoEncInfo == NULL)
	{
		return -1;
	}

	ret = g_videoEncInfo->getMDStatus(nChannel, nStreamType, status);
	
	return ret;	
}
