#ifndef __AUDIO_ENC_APP_MODULE_H_
#define __AUDIO_ENC_APP_MODULE_H_

#define AUDIO_ENC_BUFFER_SIZE 25*1024
#define AUDIO_BUFFER_SIZE 	  640*1024

#define MAX_AUDIO_CHANNEL 4

typedef int (*audioSendFun_t)(int nChannel, void *data, int *size);

int getAudioBitRateBuffer(int nChannel, int nStreamType, char *buffer, int *size);
int audioEncModuleStartup(int maxChannel, int encType, audioSendFun_t senFunc);

#endif
