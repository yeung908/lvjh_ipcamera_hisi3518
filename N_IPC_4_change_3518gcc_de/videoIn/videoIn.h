#ifndef __VIDEO_IN_H_
#define __VIDEO_IN_H_

#include "videoInModule.h"

int videoInInit(vinModuleInfo_t vinInfo);

int videoInOpen(int nChannel);
int videoInClose(int nChannel);
int videoInSetup(int nChannel, int standard);
int videoInGetSetup(int nChannel, int *standard);
int videoInStart(int nChannel);
int videoInStop(int nChannel);
int videoInGetStream(int nChannel, void *stream, int *size);
int videoInReleaseStream(int nChannel);
int videoInSetMask(int nChannel, VIDEO_MASK mask);

#endif
