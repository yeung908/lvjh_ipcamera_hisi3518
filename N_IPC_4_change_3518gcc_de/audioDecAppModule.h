#ifndef __AUDIO_DEC_APP_MODULE_H_
#define __AUDIO_DEC_APP_MODULE_H_

#include <semaphore.h>
#define AUDIO_DEC_BUFFER_SIZE 64*1024    //mody by lv old value:16*1024
#define TALK_CHANNEL 0

sem_t g_audio_dec_sem_empty;  //add by zhangjing 2013-05-30
sem_t g_audio_dec_sem_store;    //add by zhangjing 2013-05-30

int audioReciveBufferOpen(void);
int audioReciveBufferClose(void);
int audioGetTalkState(void);
int audioDecModuleStartup(int decType);
int audioDecModuleSendStream(void *stream, int size);

#endif
