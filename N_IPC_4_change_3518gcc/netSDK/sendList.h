#ifndef __SENDLIST_H_
#define __SENDLIST_H_

#include "netcomm.h"
#include "netSDK.h"

int TcpDataSendThread();
int TcpDataQuitThread();
//int RequestTcpPlay(OPEN_CHANNEL openHead, int hDataSock);
int RequestTcpPlay(OPEN_CHANNEL openHead, int hDataSock, int nDataIP);
int InsertPlayNode(SEND_NODE *pNodeHead, pthread_mutex_t hMutex, SEND_NODE *pNew);
SEND_NODE *FindWaitProcessNode(NODE_STATUS status);
SEND_NODE *GetWaitProcessHeadNode();
int FreeTcpSendNode(SEND_NODE *pNode);
int StopTcpNode(int id);
int StopAllTcpNode();
int FreeAllTcpNode();
int UdpSendThread(void *pPar);
int RequestUdpPlay(OPEN_CHANNEL openHead, unsigned long ip, unsigned long port);
BOOL ExistUdpUser(OPEN_CHANNEL openHead, unsigned long ip, unsigned long port);
BOOL CloseUdpPlay(OPEN_CHANNEL closeHead);
BOOL ExistMultiUser(OPEN_CHANNEL openHead);
BOOL CloseMultiPlay(OPEN_CHANNEL closeHead);
int RequestMultiPlay(OPEN_CHANNEL openHead);
int GetUdpRunNode(int nChannel,int nStream);
int StopUdpNode(int id);
int FreeAllUdpNode();

#endif
