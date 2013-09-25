#ifndef __AUDIO_DEC_H_
#define __AUDIO_DEC_H_

#include "audioDecModule.h"

int audioDecInit();

int audioDecOpen(int nChannel, int encType);
int audioDecClose(int nChannel);
int audioDecSetup(int opt, void *param);
int audioDecGetSetup(int opt, void *param);
int audioDecStart(int nChannel);
int audioDecStop(int nChannel);
int audioDecSendStream(int nChannel, void *stream, int *size);
int audioDecReleaseStream(int nChannel);
int audioDecGetAudio(int nChannel, void *stream, int *size);
int audioDecReleaseAudio(int nChannel);

#endif
