#ifndef __AUDIO_ENC_H_
#define __AUDIO_ENC_H_

#include "audioEncModule.h"

int audioEncInit();

int audioEncOpen(int nChannel, int nEncType);
int audioEncClose(int nChannel);
int audioEncSetup(int opt, void *param);
int audioEncGetSetup(int opt, void *param);
int audioEncStart(int nChannel);
int audioEncStop(int nChannel);
int audioEncGetStream(int nChannel, void *stream, int *size);
int audioEncReleaseStream(int nChannel);
int audioEncGetAudio(int nChannel, void *stream, int *size);
int audioEncReleaseAudio(int nChannel);

#endif
