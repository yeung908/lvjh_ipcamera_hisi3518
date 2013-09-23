#ifndef __TALK_MODULE_H_
#define __TALK_MODULE_H_

int talkFun(unsigned long ipAddr, unsigned short port, char *buffer, int size);
int UdpTalkFun(unsigned long ipAddr, unsigned short port, char *buffer, int size, struct sockaddr_in addr);

int talkStart(unsigned long ipAddr, unsigned short port);
int UdpTalkStart(unsigned long ipAddr, unsigned short port,struct sockaddr_in addr);
int talkStop();
int getTalkStatus();
int audioSendBitRateFun(int nChannel, void *stream, int *size);
int UdpAudioSendBitRateFun(int nChannel, void *stream, int *size);

#endif
