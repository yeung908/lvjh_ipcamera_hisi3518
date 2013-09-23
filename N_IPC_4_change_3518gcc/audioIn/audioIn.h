#ifndef __AUDIO_IN_H_
#define __AUDIO_IN_H_

#include "audioInModule.h"

int audioInInit(ainModuleInfo_t ainInfo);

int audioInOpen(int nChannel);
int audioInClose(int nChannel);
int audioInSetup(int nChannel, void *param);
int audioInGetSetup(int nChannel, void *param);
int audioInStart(int nChannel);
int audioInStop(int nChannel);
int audioInGetStream(int nChannel, void *stream, int *size);
int audioInReleaseStream(int nChannel);

#endif
