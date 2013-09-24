#ifndef __USERLIST_H_
#define __USERLIST_H_

#include "netcomm.h"
#include "netSDK.h"

int TcpReceive(int hSock, char *precvBuf, int nSize);
int UdpReceive(int hSock, char *precvBuf, int nSize, struct sockaddr_in addr);

int TcpMsgRecvThread();
int TcpMsgQuitThread();
int ClientLogon(struct sockaddr_in addr, NET_USER_INFO userInfo, int nID, int nRight, int nflag);
int ClientLogoff(int nID);
CLIENT_INFO *FindWaitProccessClient();
int GetExitClient();
int ClientExist(unsigned long ip, unsigned long port);
int GetClient(int nID, CLIENT_INFO *pClientInfo);
int StopAllClient();
int FreeAllClient();

#endif
