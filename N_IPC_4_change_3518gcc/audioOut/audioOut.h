#ifndef __AUDIO_OUT_H_
#define __AUDIO_OUT_H_

#include "audioOutModule.h"

int audioOutInit();

int audioOutOpen(int nChannel);
int audioOutClose(int nChannel);
int audioOutSetup(int nChannel, void *param);
int audioOutGetSetup(int nChannel, void *param);
int audioOutStart(int nChannel);
int audioOutStop(int nChannel);
int audioOutGetStream(int nChannel, void *stream, int *size);
int audioOutReleaseStream(int nChannel);

#endif
