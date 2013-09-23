#ifndef __VIDEO_ENC_APP_MODULE_H_
#define __VIDEO_ENC_APP_MODULE_H_

#include "videoEnc/videoEncModule.h"
#include "ddns.h"

typedef int (*videoSendFun_t)(int nChannel, int nStreamType, void *data, int size, int type);

int getVideoInMotionStatus(int nChannel);
int videoEncModuleStartup(videoSendFun_t bitratevideoSendFunc, videoSendFun_t jpegvideoSendFunc);

int videoJpegSnapShot(int nChannel, int type);

#endif
