#ifndef __VIDEO_ENC_H_
#define __VIDEO_ENC_H_

#include "videoEncModule.h"

int videoEncInit(vencModuleInfo_t vencInfo);

int videoEncOpen(int nChannel, int nStreamType);
int videoEncClose(int nChannel, int nStreamType);
int videoEncSetup(int nChannel, int nStreamType, int opt, void *param);
int videoEncGetSetup(int nChannel, int nStreamType, int opt, void *param);
int videoEncStart(int nChannel, int nStreamType);
int videoEncStop(int nChannel, int nStreamType);
int videoEncGetStream(int nChannel, int nStreamType, void *stream, int *size, int fpH264File);
int videoEncReleaseStream(int nChannel, int nStreamType);
int videoEncGetJpeg(int nChannel, int nStreamType, void *stream, int *size);
int videoEncGetVideo(int nChannel, int nStreamType, void *stream, int *size);
int videoEncReleaseVideo(int nChannel, int nStreamType);
int videoEncRequestIFrame(int nChannel, int nStreamType);
int videoEncInserUserInfo(int nChannel, int nStreamType, void *info, int size);
int videoEncGetMDStatus(int nChannel, int nStreamType, MD_STATUS *status);

#endif
