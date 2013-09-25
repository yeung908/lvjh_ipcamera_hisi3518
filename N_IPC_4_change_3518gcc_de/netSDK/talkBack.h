#ifndef __TALKBACK_H_
#define __TALKBACK_H_

#include "netcomm.h"
#include "netSDK.h"
int TalkbackThread(void *par);
int UdpTalkbackThread(void *par);    //mbl add

#endif
