#include "netcomm.h"
#include "netSDK.h"
#include "sendList.h"
#include "userList.h"
#include "talkBack.h"
#include "tcpNat.h"
#include "../param.h"
#include "../session.h"


// 本地函数声明
int TcpListenThread();
int UdpListenThread();
int FreeTSD(void *pParam);
unsigned long MakeLong(unsigned short low, unsigned short hi);
unsigned short LowWord(unsigned long value);

static int g_send_sleep = 0;

extern int g_aenc_module_pause_flag[4];

int SendRecordFileExt(void *param)
{
	int socket = -1;
	
	memcpy(&socket, param, sizeof(int));
	
	printf("SendRecordFileExt: %d!\n", socket);

	//socket = *(int *)param;
	
	if (socket<=0 || socket>65535)
	{
		printf("SendRecordFileExt: Failed, socket: %d!\n", socket);
		return -1;
	}
	
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	if (g_server_info.funcFileTransfer != NULL)
	{
		g_server_info.funcFileTransfer(socket);
	}
	else
	{
		printf("g_server_info.funcFileTransfer == NULL!\n");
	}
	
	printf("SendRecordFileExt: OK, socket: %d!\n", socket);

	return 0;
}

int UdpSendRecordFileExt(void *param)
{
	RECTHRD_PARAM thdpar;

	memset(&thdpar, 0, sizeof(thdpar));
	memcpy(&thdpar, param, sizeof(thdpar));
	
	if (thdpar.hSock<=0 || thdpar.hSock>65535)
	{
		printf("SendRecordFileExt: Failed, socket: %d!\n", socket);
		return -1;
	}
	
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	if (g_server_info.UdpFuncFileTransfer != NULL)
	{
		g_server_info.UdpFuncFileTransfer(thdpar.hSock, thdpar.addr, &thdpar.buffer);
	}
	else
	{
		printf("g_server_info.funcFileTransfer == NULL!\n");
	}
	
	printf("SendRecordFileExt: OK, socket: %d!\n", thdpar.hSock);

	return 0;
}

// 打开网络服务器
int NETSDK_ServerOpen()
{
	int i = 0;

	// 初始化数据结构
	memset(&g_server_info, 0, sizeof(NET_SER_INFO));
	memset(&g_client_list, 0, sizeof(CLIENT_LIST));
	
	g_server_info.hTcpListenSock = -1;
	g_server_info.hUdpListenSock = -1;
	g_server_info.hTalkbackSocket = -1;

	for (i=0; i<SERVER_MAX_CHANNEL; i++)
	{
		memset(&g_server_info.avInfoStream1[i], 0, sizeof(AV_INFO));
		memset(&g_server_info.avInfoStream2[i], 0, sizeof(AV_INFO));
		g_server_info.udpStreamSend1.pUdpSendList[i] = NULL;
		g_server_info.udpStreamSend2.pUdpSendList[i] = NULL;
		g_server_info.pChannelStream1[i] = NULL;
		g_server_info.pChannelStream2[i] = NULL;
	}

	g_server_info.pSendBuf = NULL;
	g_server_info.pRecvBuf = NULL;
	g_server_info.pUpdateFileBuf = NULL;
	g_server_info.nSendBufferNum = 0;

	// initialize recode file parameter 
	#if 1
	g_server_info.size = 0;
	g_server_info.curReadPos = 0;
	#endif 

	g_server_info.pTcpSendList = NULL;
	g_server_info.funcCheckUserPsw = NULL;
	g_server_info.funcServerRecv = NULL;
	g_server_info.bServerStart = 0;
	g_server_info.bServerExit = 0;

	g_server_info.pCallbackRequestTalk = NULL;
	g_server_info.pCallbackStreamTalk = NULL;

	g_server_info.funcFileTransfer = NULL;
	g_server_info.funcCheckUserPsw = NULL;
    g_server_info.funcServerRecv = NULL;

	//
	g_client_list.nTotalNum = 0;
	g_client_list.pNext = NULL;
    
	// 
	g_talkbackSock = -1;
	
	return 0;
}

// 设置网络服务器
int NETSDK_ServerSetup(int nChnNum, int nPort, char *pMultiIP, int bufSize)
{
	int i = 0;
	long addr = -1;

	// 输入参数校验
	if (nChnNum<1 || nChnNum>SERVER_MAX_CHANNEL)
	{
		return -1;
	}
	if (nPort<MIN_SOCK_PORT || nPort>MAX_SOCK_PORT)
	{
		return -1;
	}
	if (pMultiIP == NULL)
	{	
		return -1;
	}
	addr = inet_addr(pMultiIP);
	if (addr == -1)
	{
		return -1;
	}
	if (bufSize<=0 || bufSize>MAX_POOL_SIZE)
	{
		return -1;
	}
	
	// 网络缓冲大小
	g_server_info.nSendBufferNum = bufSize;
	
	// 网络参数
	g_server_info.nChnNum = nChnNum;
	//g_server_info.nChnNum = 2;
	g_server_info.nBasePort = nPort;
	strcpy(g_server_info.szMultiIP, pMultiIP);
	g_server_info.multiAddr = addr;
	
	return 0;
}

// 获取网络服务器的设置
int NETSDK_GetServerSetup(int *nChnNum, int *nPort, char *pMultiIP, int *bufSize)
{
	if (nChnNum==NULL || nPort==NULL || pMultiIP==NULL || bufSize==NULL)
	{
		return -1;
	}

	*nChnNum = g_server_info.nChnNum;
	*nPort = g_server_info.nBasePort;	
	*bufSize = g_server_info.nSendBufferNum;
	strcpy(pMultiIP, g_server_info.szMultiIP);

	return 0;
}

void signalProc(int sig)
{
	switch (sig)
	{
	case SIGPIPE:
	//	printf("signalProc: SIGPIPE!\n");
		break;

	default:
		printf("signalProc: Unknow signal!\n");
		break;
	}
}

unsigned long g_channel = 0;

// 启用网络服务
int NETSDK_ServerStart()
{
	int i = 0;
	int ret = 0;
	int hSock = -1;
  	SEND_NODE *pNode = NULL;
	unsigned long nChannel = 0;
  	
	g_server_info.bServerExit = 0;
	
	//signal(SIGPIPE, SIG_IGN);
	signal(SIGPIPE, signalProc);
	
	// add by zhb, 2007-10-27
	ret = pthread_mutex_init(&g_client_list.hClientMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}

	// 创建TCP监听SOCKET
	hSock = TcpSockListen(INADDR_ANY, g_server_info.nBasePort);
	if (hSock < 0)
	{
		net_debug();
		return -1;
	}
	g_server_info.hTcpListenSock = hSock;

	// 创建UDP监听SOCKET
	hSock = UdpSockListen(INADDR_ANY, g_server_info.nBasePort);
	if (hSock < 0)
	{
		net_debug();
		return -1;
	}
	g_server_info.hUdpListenSock = hSock;
	
	// 初始化数据通道BUFFER
	for (i=0; i<g_server_info.nChnNum; i++)
	{

		// 主码流
		g_server_info.pChannelStream1[i] = NULL;
		g_server_info.pChannelStream1[i] = (CHANNEL_STREAM *)malloc(sizeof(CHANNEL_STREAM));
		if (g_server_info.pChannelStream1[i] == NULL)
		{
			net_debug();
			return -1;
		}
		//printf("%p\n", g_server_info.pChannelStream1[i]);
		ret = InitChannel(g_server_info.pChannelStream1[i]);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		
		// 附码流
		g_server_info.pChannelStream2[i] = NULL;
		g_server_info.pChannelStream2[i] = (CHANNEL_STREAM *)malloc(sizeof(CHANNEL_STREAM));
		if (g_server_info.pChannelStream2[i] == NULL)
		{
			return -1;
		}
		//printf("%p\n", g_server_info.pChannelStream2[i]);
		ret = InitChannel(g_server_info.pChannelStream2[i]);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		
		// 主码流UDP发送节点
		pNode = (SEND_NODE *)malloc(sizeof(SEND_NODE)) ;
		if (pNode == NULL)
		{
			net_debug();
			return -1;
		}
		memset(pNode, 0, sizeof(SEND_NODE));
		pNode->status = RUN;
		g_server_info.udpStreamSend1.pUdpSendList[i] = pNode;
		ret = pthread_mutex_init(&g_server_info.udpStreamSend1.hUdpSendListMutex[i], NULL);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}

		// 创建主码流UDP发送线程
		ret = pthread_create(&g_server_info.udpStreamSend1.hUdpSendThreadID[i], NULL, (void*)&UdpSendThread, (void*)MakeLong(i, 0));
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		usleep(1000);
			
		// 附码流UDP发送节点
		pNode = (SEND_NODE *)malloc(sizeof(SEND_NODE)) ;
		if (pNode == NULL)
		{
			net_debug();
			return -1;
		}
		memset(pNode, 0, sizeof(SEND_NODE));
		pNode->status = RUN;
		g_server_info.udpStreamSend2.pUdpSendList[i] = pNode;
		ret = pthread_mutex_init(&g_server_info.udpStreamSend2.hUdpSendListMutex[i], NULL);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		
		// 创建附码流UDP发送线程
		ret = pthread_create(&g_server_info.udpStreamSend2.hUdpSendThreadID[i], NULL, (void*)&UdpSendThread, (void*)MakeLong(i, 1));
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
			usleep(1000);		
	}
	
	pNode = (SEND_NODE *)malloc(sizeof(SEND_NODE));
	if (pNode == NULL)
	{
		net_debug();
		return -1;
	}
	memset(pNode, 0, sizeof(SEND_NODE));
	pNode->status = RUN;
	g_server_info.pTcpSendList = pNode;
	ret = pthread_mutex_init(&g_server_info.hTcpSendListMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}

	g_server_info.pSendBuf = NULL;
	g_server_info.pSendBuf = (char *) malloc(NETCMD_MAX_SIZE);
	if (g_server_info.pSendBuf == NULL)
	{
		net_debug();
		return -1;
	}
	ret = pthread_mutex_init(&g_server_info.sendBufMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
   
	g_server_info.pRecvBuf = NULL;
	/*
	g_server_info.pRecvBuf = (char *) malloc(NETCMD_MAX_SIZE);
	if (g_server_info.pRecvBuf == NULL)
	{
		net_debug();
		return -1;
	}
    */

	// 命令通道线程数信息
	g_server_info.msgProcessThreadNum = 0;
	g_server_info.msgWaitThreadNum = 0;
	g_server_info.msgQuitThreadNum = 0;
	ret = pthread_mutex_init(&g_server_info.msgThreadMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	ret = pthread_cond_init(&g_server_info.msgThreadCond, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}

	#if 1
	// 创建命令通道处理线程和命令通道退出线程
	for (i=0; i<MAX_USER_CMD_CHANNEL_NUM; i++)
	{
		ret = pthread_create(&g_server_info.msgThreadID[i], NULL, (void *)&TcpMsgRecvThread, NULL);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		//printf("MAX_USER_CMD_CHANNEL_NUM: %d\n", i);
	}
	#endif

	ret = pthread_mutex_init(&g_server_info.msgQuitThreadMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	ret = pthread_cond_init(&g_server_info.msgQuitThreadCond, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	ret = pthread_create(&g_server_info.msgQuitThread, NULL, (void *)&TcpMsgQuitThread, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	
	// 数据通道线程数信息	
	g_server_info.dataProcessThreadNum = 0;
	g_server_info.dataWaitThreadNum = 0;
	g_server_info.dataQuitThreadNum = 0;
	ret = pthread_mutex_init(&g_server_info.dataThreadMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	ret = pthread_cond_init(&g_server_info.dataThreadCond, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	
	// 创建数据通道处理线程和数据通道退出线程
	for (i=0; i<MAX_USER_DATA_CHANNEL_NUM; i++)
	{
		ret = pthread_create(&g_server_info.dataThreadID[i], NULL, (void *)&TcpDataSendThread, NULL);
		if (ret < 0)
		{
			net_debug();
			return -1;
		}
		//printf("MAX_USER_DATA_CHANNEL_NUM: %d\n", i);
	}
	usleep(1000);
	ret = pthread_mutex_init(&g_server_info.dataQuitThreadMutex, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	ret = pthread_cond_init(&g_server_info.dataQuitThreadCond, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	
	ret = pthread_create(&g_server_info.dataQuitThread, NULL, (void *)&TcpDataQuitThread, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	usleep(1000);
		
	ret = pthread_key_create(&g_server_info.hReadPosKey, (void *)FreeTSD);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	usleep(1000);
#if 1
	// 创建UDP监听线程
	ret = pthread_create(&g_server_info.hUdpListenThread, NULL, (void*)&UdpListenThread, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	usleep(1000);
#endif


	// 创建TCP监听线程
	ret = pthread_create(&g_server_info.hTcpListenThread, NULL, (void*)&TcpListenThread, NULL);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	usleep(1000);

	g_server_info.nMsgThreadCount = MAX_USER_CMD_CHANNEL_NUM;
	g_server_info.nDataThreadCount = MAX_USER_DATA_CHANNEL_NUM;
	
	g_server_info.bServerStart = 1;
	
	return 0;
}

// 停止网络服务器
int NETSDK_ServerStop()
{
	int i = 0;
    
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	g_server_info.bServerStart = 0;
	g_server_info.bServerExit = 1;

	// 关闭监听SOCKET
	if (g_server_info.hTcpListenSock > 0)
	{
		shutdown(g_server_info.hTcpListenSock, 2);
		close(g_server_info.hTcpListenSock);
		g_server_info.hTcpListenSock = -1;
	}
	if (g_server_info.hUdpListenSock > 0)
	{
		shutdown(g_server_info.hUdpListenSock, 2);
		close(g_server_info.hUdpListenSock);
		g_server_info.hUdpListenSock = -1;
	}
    
	StopAllTcpNode();
	WakeupStreamWait();
	StopAllClient();

	sleep(SOCK_TIME_OUT);

	pthread_cond_broadcast(&g_server_info.dataThreadCond);
	pthread_cond_broadcast(&g_server_info.msgThreadCond);
	usleep(1);
	pthread_cond_signal(&g_server_info.msgQuitThreadCond);
	pthread_cond_signal(&g_server_info.dataQuitThreadCond);
	usleep(1);
    
	FreeAllUdpNode();
	FreeAllTcpNode();
	FreeAllClient();

	// 释放发送公共BUFFER
	if (g_server_info.pSendBuf)
	{
		free(g_server_info.pSendBuf);
		g_server_info.pSendBuf = NULL;
	}
	pthread_mutex_destroy(&g_server_info.sendBufMutex);
    
	// 释放接收公共BUFFER
	if(g_server_info.pRecvBuf)
	{
		free(g_server_info.pRecvBuf);
		g_server_info.pRecvBuf = NULL;
	}
    
	// 释放接收升级文件BUFFER
	if (g_server_info.pUpdateFileBuf)
	{
		free(g_server_info.pUpdateFileBuf);
		g_server_info.pUpdateFileBuf = NULL;
	}
    
	// 释放数据通道
	for (i=0; i<g_server_info.nChnNum; i++)
	{
		DestroyStream(g_server_info.pChannelStream1[i]);
		DestroyStream(g_server_info.pChannelStream2[i]);
	}

	pthread_key_delete(g_server_info.hReadPosKey);
    
	// 命令通道线程的互斥锁和条件变量
	g_server_info.msgProcessThreadNum = 0;
	g_server_info.msgWaitThreadNum = 0;
	g_server_info.msgQuitThreadNum = 0;
	pthread_mutex_destroy(&g_server_info.msgThreadMutex);
	pthread_cond_destroy(&g_server_info.msgThreadCond);
	
	// 释放命令线程的互斥锁和条件变量
	pthread_mutex_destroy(&g_server_info.msgQuitThreadMutex);
	pthread_cond_destroy(&g_server_info.msgQuitThreadCond);
	
	// 释放数据线程的互斥锁和条件变量
	g_server_info.dataProcessThreadNum = 0;
	g_server_info.dataWaitThreadNum = 0;
	g_server_info.dataQuitThreadNum = 0;
	pthread_mutex_destroy(&g_server_info.dataThreadMutex);
	pthread_cond_destroy(&g_server_info.dataThreadCond);
	pthread_mutex_destroy(&g_server_info.hTcpSendListMutex);
	
	// 释放数据线程的互斥锁和条件变量
	pthread_mutex_destroy(&g_server_info.dataQuitThreadMutex);
	pthread_cond_destroy(&g_server_info.dataQuitThreadCond);

	return 0;
}

// 停止网络服务器
int NETSDK_ServerClose()
{
	// 初始化数据结构
	memset(&g_server_info, 0, sizeof(NET_SER_INFO));
	memset(&g_client_list, 0, sizeof(CLIENT_LIST));
	
	g_server_info.hTcpListenSock = -1;
	g_server_info.hUdpListenSock = -1;
	g_server_info.hTalkbackSocket = -1;
	
	g_talkbackSock = -1;
	
	return 0;
}

int Udp_NETSDK_SendMsg(MSG_HEAD	*pMsgHead, char *pSendBuf, struct sockaddr_in addr)
{
	int ret = -1;
	NET_HEAD netHead;
	//struct sockaddr_in addr; 
    
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (g_server_info.pSendBuf == NULL)
	{
		return -1;
	}
	if (pMsgHead==NULL || pSendBuf==NULL)
	{
		return -1;
	}
	if (pMsgHead->nSock < 0)	// 非法的SOCKET
	{
		return -1;
	}
	if ((pMsgHead->nBufSize+sizeof(NET_HEAD)) > NETCMD_MAX_SIZE)	// 数据超过公共的BUFFER的尺寸
	{
		return -1;
	}
	if (pMsgHead->nBufSize < 0)
	{
		return -1;
	}
        
	// 填写网络帧头
	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = pMsgHead->nCmd;
	netHead.nBufSize = pMsgHead->nBufSize;
	netHead.nErrorCode = pMsgHead->nErrorCode;
	if (pMsgHead->nCmd == NETCMD_LOGON)
	{
		netHead.nReserve = pMsgHead->nSock;
	}
	else
	{
		netHead.nReserve = 0;
	}
	
	// 锁取发送公共BUFFER
	pthread_mutex_lock(&g_server_info.sendBufMutex);
    
	memcpy(g_server_info.pSendBuf, &netHead, sizeof(netHead));
	memcpy(g_server_info.pSendBuf + sizeof(netHead), pSendBuf, pMsgHead->nBufSize);
//	ret = send(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0);
	printf("UDP: IP = %s PORT = %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	ret = sendto(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
				
	if (ret < 0)
	{
		net_debug();    	
	}
		
	// 解锁发送公共BUFFER
	pthread_mutex_unlock(&g_server_info.sendBufMutex);

	return ret;
}
// 向指定用户发送消息
int NETSDK_SendMsg(MSG_HEAD	*pMsgHead, char *pSendBuf)
{
	int ret = -1;
	NET_HEAD netHead;

	//printf("NETSDK_SendMsg = %d", pMsgHead->nSock);
#if 1   
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (g_server_info.pSendBuf == NULL)
	{
		return -1;
	}
	if (pMsgHead==NULL || pSendBuf==NULL)
	{
		return -1;
	}
	if (pMsgHead->nSock < 0)	// 非法的SOCKET
	{
		return -1;
	}
	if ((pMsgHead->nBufSize+sizeof(NET_HEAD)) > NETCMD_MAX_SIZE)	// 数据超过公共的BUFFER的尺寸
	{
		return -1;
	}
	if (pMsgHead->nBufSize < 0)
	{
		return -1;
	}
#endif
        
	// 填写网络帧头
	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = pMsgHead->nCmd;
	netHead.nBufSize = pMsgHead->nBufSize;
	netHead.nErrorCode = pMsgHead->nErrorCode;
	if (pMsgHead->nCmd == NETCMD_LOGON)
	{
		netHead.nReserve = pMsgHead->nSock;
	}
	else
	{
		netHead.nReserve = 0;
	}
	
	// 锁取发送公共BUFFER
	pthread_mutex_lock(&g_server_info.sendBufMutex);
    memcpy(g_server_info.pSendBuf, &netHead, sizeof(netHead));
	memcpy(g_server_info.pSendBuf + sizeof(netHead), pSendBuf, pMsgHead->nBufSize);
	#if 1
	if(pMsgHead->nflag == UDP_FLAG){
		printf(" NETSDK_SendMsg UDP:\n");
		//ret = send(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0);
		ret = sendto(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			net_debug();    	
		}
	}
	else{
		//printf(" NETSDK_SendMsg TCP:\n");
		ret = send(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0);
		if (ret < 0)
		{
			net_debug();    	
		}
	}
	#endif
	
	#if 0
	ret = send(pMsgHead->nSock, g_server_info.pSendBuf, sizeof(NET_HEAD)+pMsgHead->nBufSize, 0);
	if (ret < 0)
	{
		net_debug();    	
	}
	#endif
	// 解锁发送公共BUFFER
	pthread_mutex_unlock(&g_server_info.sendBufMutex);

	return ret;
}

// 向所有用户发送消息
int NETSDK_SendAllMsg(char *pMsgBuf, int nLen)
{
	int ret = -1;
	MSG_HEAD msgHead;
	CLIENT_INFO *pUser = NULL;
    
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (pMsgBuf == NULL)
	{
		return -1;
	}
	if ((nLen+sizeof(NET_HEAD)) > NETCMD_MAX_SIZE)	// 数据超过公共的BUFFER的尺寸
	{
		return -1;
	}
	if (nLen <= 0)
	{
		return -1;
	}
    
	memset(&msgHead, 0, sizeof(msgHead));
    
   	pthread_mutex_lock(&g_client_list.hClientMutex);
    
	pUser = g_client_list.pNext;
	while (pUser)
	{
		msgHead.nSock = pUser->hMsgSocket;
		msgHead.addr = pUser->addr;
		msgHead.nflag = pUser->nflag;
		msgHead.nCmd = NETCMD_SERVER_MSG;
		msgHead.nErrorCode = 0;
		msgHead.nBufSize = nLen;        

		ret = NETSDK_SendMsg(&msgHead, pMsgBuf);
		pUser = pUser->pNext;
	}
    
	pthread_mutex_unlock(&g_client_list.hClientMutex);

	return ret;
}

// 向指定用户发送消息
int NETSDK_SendSpecMsg(int nUserID, char *pMsgBuf, int nLen)
{
	int ret = 0;
	MSG_HEAD msgHead;
	CLIENT_INFO *pUser = NULL;

	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (pMsgBuf == NULL)
	{
		return -1;
	}
	if ((nLen+sizeof(NET_HEAD)) > NETCMD_MAX_SIZE)	// 数据超过公共的BUFFER的尺寸
	{
		return -1;
	}
	if (nLen <= 0)
	{
		return -1;
	}
	if (nUserID < 0)
	{
		return -1;
	}
    
	memset(&msgHead, 0, sizeof(msgHead));
    
   	pthread_mutex_lock(&g_client_list.hClientMutex);
   	
	pUser = g_client_list.pNext;
	while (pUser)
	{
		if (nUserID == pUser->hMsgSocket)
		{
			msgHead.nSock = pUser->hMsgSocket;
			msgHead.nCmd = NETCMD_SERVER_MSG;
			msgHead.nErrorCode = 0;
			msgHead.nBufSize = nLen;   

			ret = NETSDK_SendMsg(&msgHead, pMsgBuf);
            
			break;
		}
		pUser = pUser->pNext;
	}
    
	pthread_mutex_unlock(&g_client_list.hClientMutex);

	return ret;
}

// 获取用户的ID,即用户的SOCKET
int NETSDK_GetUserId(unsigned long ipAddr)
{
	int sock = -1;
	CLIENT_INFO *pUser = NULL;

	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (ipAddr < 0)
	{
		return -1;
	}
    
   	pthread_mutex_lock(&g_client_list.hClientMutex);
   	
	pUser = g_client_list.pNext;
	while (pUser)
	{
		if (ipAddr == pUser->hMsgSocket)
		{     
			sock = pUser->hMsgSocket;
			break;
		}
		pUser = pUser->pNext;
	}
    
	pthread_mutex_unlock(&g_client_list.hClientMutex);

	return sock;
}

// 设置主AV信息
int NETSDK_SetAVInfo1(int nChannel, AV_INFO *pAVInfo)
{
	if ((nChannel < 0) || (nChannel >= SERVER_MAX_CHANNEL))
	{
		return -1;
	}
	if (NULL == pAVInfo)
	{
		return -1;
	}

	memcpy(&g_server_info.avInfoStream1[nChannel], pAVInfo, sizeof(AV_INFO));
    
	return 0;
}

// 设置副AV信息
int NETSDK_SetAVInfo2(int nChannel, AV_INFO *pAVInfo)
{       
	if ((nChannel < 0) || (nChannel >= SERVER_MAX_CHANNEL))
	{
		return -1;
	}
	if (NULL == pAVInfo)
	{
		return -1;
	}
        
	memcpy(&g_server_info.avInfoStream2[nChannel], pAVInfo, sizeof(AV_INFO));
    
	return 0;
}

// 设置主AV信息
int NETSDK_GetAVInfo1(int nChannel, AV_INFO *pAVInfo)
{
	if ((nChannel < 0) || (nChannel >= SERVER_MAX_CHANNEL))
	{
		return -1;
	}
	if (NULL == pAVInfo)
	{
		return -1;
	}
        
	memcpy(pAVInfo, &g_server_info.avInfoStream1[nChannel], sizeof(AV_INFO));
    
	return 0;
}

// 设置副AV信息
int NETSDK_GetAVInfo2(int nChannel, AV_INFO *pAVInfo)
{       
	if ((nChannel < 0) || (nChannel >= SERVER_MAX_CHANNEL))
	{
		return -1;
	}
	if (NULL == pAVInfo)
	{
		return -1;
	}
        
	memcpy(pAVInfo, &g_server_info.avInfoStream2[nChannel], sizeof(AV_INFO));
    
	return 0;
}

// 发送一帧数据到网络主码流的BUFFER
int NETSDK_WriteOneFrame1(int nChannel, char *pFrame)
{
	int ret = -1;
	
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=SERVER_MAX_CHANNEL)
	{
		return -1;
	}    
	if (pFrame == NULL)
	{
		return -1;
	}
    
	ret = WriteFrameToStream(nChannel, g_server_info.pChannelStream1[nChannel], pFrame);

	return ret;
}

// 发送一帧数据到网络副码流的BUFFER
int NETSDK_WriteOneFrame2(int nChannel, char *pFrame)
{
	int ret = -1;
	
	if (!g_server_info.bServerStart)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=SERVER_MAX_CHANNEL)
	{
		return -1;
	}    
	if (pFrame == NULL)
	{
		return -1;
	}
        
	ret = WriteFrameToStream(nChannel, g_server_info.pChannelStream2[nChannel], pFrame);
    
	return ret;
}

// 系统初始化之后，最好不要再设置，因为如果nBufNum大于g_server_info.nSendBufferNum，就会出现非法指针
int NETSDK_SetSendBufNum(int nBufNum)
{
	if ((nBufNum <= 0) || (nBufNum > MAX_POOL_SIZE))
	{
		return -1;
	}
	if (g_server_info.bServerStart)
	{
		return -1;
	}

	//g_server_info.nSendBufferNum = nBufNum;

	return 0;
}

int NETSDK_SetUdpUserCheckFunc(CheckUdpUserPsw pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_server_info.funcUdpCheckUserPsw = pFunc;

	return 0;
}


int NETSDK_SetUserCheckFunc(CheckUserPsw pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_server_info.funcCheckUserPsw = pFunc;

	return 0;
}

int NETSDK_SetServerRecvFunc(ServerRecv pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_server_info.funcServerRecv = pFunc;

	return 0;
}

int NETSDK_SetFileTransferFunc(FileTransfer pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_server_info.funcFileTransfer = pFunc;

	return 0;
}

#if 1
int NETSDK_SetFileUdpTransferFunc(UdpFileTransfer pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_server_info.UdpFuncFileTransfer = pFunc;

	return 0;
}
#endif



// TCP监听线程
int TcpListenThread()
{
	int ret = -1;
	int len = 0;
	int hConnSock = -1;
	int hListenSock = -1;	

	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	
	NET_HEAD netHead;
	MSG_HEAD msgHead;
	NET_MSG_HEAD HOSSNetHead;
	PTZ_CMD   ptzCmd;

	char buffer[TCP_RCV_BUFFER_LEN];

	FD_ZERO(&fset);
	memset(buffer, 0, TCP_RCV_BUFFER_LEN);
	memset(&to, 0, sizeof(to));
	memset(&addr, 0, sizeof(addr));
	memset(&netHead, 0, sizeof(NET_HEAD));	
	
	len = sizeof(addr);
	hListenSock = g_server_info.hTcpListenSock;
	if (hListenSock < 0)
	{
		return -1;
	}
    printf("%s %d\n", __func__, __LINE__);
	g_server_info.bServerExit = 0;
	while (!g_server_info.bServerExit)
	{
	//   printf("%s %d\n", __func__, __LINE__);
		hConnSock = -1;
		
		to.tv_sec = SOCK_TIME_OUT;
		to.tv_usec = 0;
    	
		FD_SET(hListenSock, &fset);
		ret = select(hListenSock+1, &fset, NULL, NULL, &to);

			// 判断服务器是否退出       
		if (g_server_info.bServerExit)
		{       	
			break;
		}
		
		// SOCKET接收超时,继续
		if (ret == 0)
		{			
			continue;
		}
		// SOCKET接收错误,判断是否是中断引起，如果是，继续，否则重启系统
		if (ret == -1)
		{		
			net_debug();			
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				restartSystem();
				break;
			}
		}
		
		// 判断SELECT是否设置，如果没有，则继续
		if (!FD_ISSET(hListenSock, &fset))
		{		
			continue;
		}
		
		printf("%s %d\n", __func__, __LINE__);
		// 接收用户的连接
		hConnSock = accept(hListenSock, (struct sockaddr*)&addr, (socklen_t *)&len);
		if (hConnSock < 0)
		{		
			net_debug();
			
			// 如果出现打开系统文件太多，则只有重启（在打开许多SOCKET或文件之后，又没关闭情况下会出现）
			if (errno == 1000) // Too many open files
			{
				restartSystem();
				break;
			}
			else
			{
				continue;
			}
		}

		// 设置连接的SOCKET的属性		
		ret = SetConnSockAttr(hConnSock, SOCK_TIME_OUT);
		if (ret < 0)
		{
			net_debug();
			shutdown(hConnSock, 2);
			close(hConnSock);
			hConnSock = -1;
			continue;
		}
		
		// 接收网络帧头,如果出现错误,则关闭SOCKET
		memset(&netHead, 0, sizeof(NET_HEAD));
		ret = TcpReceive(hConnSock, &netHead,  sizeof(netHead));
		if (ret < sizeof(netHead))
		{			
			net_debug();
			shutdown(hConnSock, 2);
			close(hConnSock);
			continue;
		}
		
		
		printf("netHead.nFlag = %x %x\n\n", netHead.nFlag, netHead.nCommand);
		if(netHead.nFlag == HSS_HDVS_FLAG)
		{
			printf("android \n");
			memcpy(&buffer, &netHead, sizeof(netHead));
			#if 1
				ret = TcpReceive(hConnSock, buffer+sizeof(netHead),  sizeof(HOSSNetHead)-sizeof(netHead));
				if (ret < (sizeof(HOSSNetHead)- sizeof(netHead)))
				{			
					net_debug();
					shutdown(hConnSock, 2);
					close(hConnSock);
					printf("HOSS ::TcpReceive errror\n");
					continue;
				}
			#endif
			memcpy(&HOSSNetHead, buffer, sizeof(HOSSNetHead));
#if 1
			printf(" HSS android login on \\\n\n");
			printf(" HOSSNetHead.nTypeMain = %x \\\n\n", HOSSNetHead.nTypeMain);
			printf(" HOSSNetHead.dwDataSize = %d \\\n\n", HOSSNetHead.dwDataSize);
			
#endif
			switch(HOSSNetHead.nTypeMain)	
			{
				case NET_CMD_LINK:
				{
					int nCurNum = 0;
					int nRight = 0;
					int nClientID = 0;				  
					NETSDK_USER HssUserInfo;  
					NET_USER_INFO YiviewUserInfo;  
					NETSDK_USERINFO userInfo;
					memset(&YiviewUserInfo, 0, sizeof(NET_USER_INFO));
					
					// 参数的大小不对
					if (HOSSNetHead.dwDataSize < sizeof(NETSDK_USER))
					{
						HOSSNetHead.pMask1 = HSS_HDVS_FLAG;
						HOSSNetHead.nTypeSub = -1;
						printf("NET_USER_INFO: %d %d\n", HOSSNetHead.dwDataSize, sizeof(NETSDK_USER));
			
						ret = send(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD), 0);
						if (ret < 0)
						{
							net_debug();
						}
						sleep(1);
						shutdown(hConnSock, 2);
						close(hConnSock);
						break;
					}								
					
					// 接收用户的信息,包括用户名和密码
					memset(&HssUserInfo, 0, sizeof(HssUserInfo));
					ret = TcpReceive(hConnSock, &HssUserInfo, sizeof(NETSDK_USER));
					if (ret < sizeof(NETSDK_USER))
					{
						HOSSNetHead.pMask1 = HSS_HDVS_FLAG;
						HOSSNetHead.nTypeSub = -1;
						//printf("NET_USER_INFO: %d %d\n", netHead.nBufSize, sizeof(NET_USER_INFO));
			
						ret = send(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD), 0);
						if (ret < 0)
						{
							net_debug();
						}
						sleep(1);
						shutdown(hConnSock, 2);
						close(hConnSock);
						
						break;
					}
					strcpy(YiviewUserInfo.szUserName, HssUserInfo.lpUserName);
					strcpy(YiviewUserInfo.szUserPsw,  HssUserInfo.lpPassword);
					printf("HssUserInfo.lpUserName = %s, HssUserInf.lpPassword = %s\n", HssUserInfo.lpUserName, HssUserInfo.lpPassword);

					// 校验用户 			
					if (strcmp(ADMIN_USER, HssUserInfo.lpUserName) || strcmp(HssUserInfo.lpPassword, ADMIN_PASSWD))
					{	
						printf("HOSS login in error \n");
						break;
						
					}

					memset(&HOSSNetHead, 0, sizeof(HOSSNetHead));
					memset(&userInfo, 0, sizeof(userInfo));
					memset(buffer, 0, TCP_RCV_BUFFER_LEN);
					HOSSNetHead.pMask1 = HSS_HDVS_FLAG;
					HOSSNetHead.pMask2 = HSS_HDVS_FLAG2;
					HOSSNetHead.dwDataSize = sizeof(userInfo);
					HOSSNetHead.nTypeMain = NET_CMD_LINK;
					HOSSNetHead.nTypeSub = 0;
					userInfo.server_sock = hConnSock;
					userInfo.rights = 0;
					
					memcpy(buffer, &HOSSNetHead, sizeof(HOSSNetHead));
					memcpy(buffer+sizeof(HOSSNetHead), &userInfo, sizeof(userInfo));
					
					ret = send(hConnSock, buffer , sizeof(HOSSNetHead)+sizeof(userInfo), 0);
					if (ret < 0)
					{
						net_debug();
					}
					
					#if 1
						memset(&HOSSNetHead, 0, sizeof(NET_MSG_HEAD));
						ret = TcpReceive(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD));
					//	printf("TcpReceive ret = %d\n", ret);
					//	printf("UDP: %s %d %d %d \n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), netHead.nCommand, ret);
					
						if (ret < sizeof(NET_MSG_HEAD))
						{
							HOSSNetHead.pMask1 = HSS_HDVS_FLAG;
							HOSSNetHead.nTypeSub = -1;
							//printf("NET_USER_INFO: %d %d\n", netHead.nBufSize, sizeof(NET_USER_INFO));
					
							ret = send(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD), 0);
							if (ret < 0)
							{
								net_debug();
							}
							sleep(1);
							shutdown(hConnSock, 2);
							close(hConnSock);
							break;
						}
					
						printf("main CMD: %x\n", HOSSNetHead.nTypeMain);
						switch(HOSSNetHead.nTypeMain)
						{
							case NET_CMD_PTZCTRL:
								printf("sub CMD: %x\n", HOSSNetHead.nTypeSub);
								switch(HOSSNetHead.nTypeSub)
								{
									case PTZ_LEFT:
										ptzCmd.nCmd = LEFT_START;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
								
										break;
									case PTZ_LEFTSTOP:
										ptzCmd.nCmd = LEFT_STOP;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
										
										
									case PTZ_RIGHT:
										ptzCmd.nCmd = RIGHT_START;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										
										break;
									case PTZ_RIGHTSTOP:
										ptzCmd.nCmd = RIGHT_STOP;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
										
									case PTZ_UP:
										ptzCmd.nCmd = UP_START;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										
										break;
									case PTZ_UPSTOP:
										ptzCmd.nCmd = UP_STOP;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
									break;
					
					
									case PTZ_DOWN:
										ptzCmd.nCmd = DOWN_START;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										
										break;
									case PTZ_DOWNSTOP:
										ptzCmd.nCmd = DOWN_STOP;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
									break;
										
									case PTZ_AUTO:
										ptzCmd.nCmd = ROTATION_START;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
									case PTZ_AUTOSTOP:
										ptzCmd.nCmd = ROTATION_STOP;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
										
									case PTZ_SETPOINT:
										ptzCmd.nCmd = SET_POINT;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
									case PTZ_GOTOPOINT:
										ptzCmd.nCmd = GOTO_POINT;
										ret = ptzControl(0, ptzCmd);
										if (ret < 0)
										{
											return -1;
										}
										break;
										
									default:
										break;
										
								}
								
								break;

							default:
								break;
						}
					#endif

				#if 1
					memset(&HOSSNetHead, 0, sizeof(NET_MSG_HEAD));
					ret = TcpReceive(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD));
				//	printf("TcpReceive ret = %d\n", ret);
				//	printf("UDP: %s %d %d %d \n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), netHead.nCommand, ret);
		
					if (ret < sizeof(NET_MSG_HEAD))
					{
						HOSSNetHead.pMask1 = HSS_HDVS_FLAG;
						HOSSNetHead.nTypeSub = -1;
						//printf("NET_USER_INFO: %d %d\n", netHead.nBufSize, sizeof(NET_USER_INFO));
			
						ret = send(hConnSock, &HOSSNetHead, sizeof(NET_MSG_HEAD), 0);
						if (ret < 0)
						{
							net_debug();
						}
						sleep(1);
						shutdown(hConnSock, 2);
						close(hConnSock);
						break;
					}

					printf("main CMD: %x\n", HOSSNetHead.nTypeMain);
					switch(HOSSNetHead.nTypeMain)
					{
						case NET_CMD_PTZCTRL:
							printf("sub CMD: %x\n", HOSSNetHead.nTypeSub);
							switch(HOSSNetHead.nTypeSub)
							{
								case PTZ_LEFT:
									ptzCmd.nCmd = LEFT_START;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
								case PTZ_LEFTSTOP:
									ptzCmd.nCmd = LEFT_STOP;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
									
									
								case PTZ_RIGHT:
									ptzCmd.nCmd = RIGHT_START;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									
									break;
								case PTZ_RIGHTSTOP:
									ptzCmd.nCmd = RIGHT_STOP;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
									
								case PTZ_UP:
									ptzCmd.nCmd = UP_START;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
									
								case PTZ_UPSTOP:
									ptzCmd.nCmd = UP_STOP;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
								break;


								case PTZ_DOWN:
									ptzCmd.nCmd = DOWN_START;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}

									break;
								case PTZ_DOWNSTOP:
									ptzCmd.nCmd = DOWN_STOP;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
								break;
									
								case PTZ_AUTO:
									ptzCmd.nCmd = ROTATION_START;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
								case PTZ_AUTOSTOP:
									ptzCmd.nCmd = ROTATION_STOP;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
									
								case PTZ_SETPOINT:
									ptzCmd.nCmd = SET_POINT;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
								case PTZ_GOTOPOINT:
									ptzCmd.nCmd = GOTO_POINT;
									ret = ptzControl(0, ptzCmd);
									if (ret < 0)
									{
										return -1;
									}
									break;
									
								default:
									break;
									
							}
							
							break;

							
						default:
							break;
					}
					#endif
					
					#if 0
						printf(" HSS android login on \\\n\n");
						printf(" HOSSNetHead.nTypeMain = %x \\\n\n", HOSSNetHead.nTypeMain);
						printf(" HOSSNetHead.dwDataSize = %d \\\n\n", HOSSNetHead.dwDataSize);
					#endif
					
				}
				break;
				
				default:
						printf("NOT support command\n");
						break;
			}
			printf("HSS return select.......\n");
			continue;
		}

		#if 0
			printf("sizeof(int)= %d sizeof(long)= %x\n\n", sizeof(int), sizeof(long));
			printf("sizeof(NET_HEAD)= %d sizeof(NET_MSG_HEAD)= %d\n\n", sizeof(NET_HEAD), sizeof(NET_MSG_HEAD));
			
			printf("netHead.nBufSize = %d\n\n", netHead.nBufSize);
			printf("netHead.nCommand = %x\n\n", netHead.nCommand);
		#endif
		
	// 处理网络命令
		switch(netHead.nCommand)
		{
			case NETCMD_LOGON:	// 用户登陆
			{
				int nCurNum = 0;
				int nRight = 0;
				int nClientID = 0;                
				NET_USER_INFO userInfo;                

				memset(&userInfo, 0, sizeof(userInfo));
				
				// 参数的大小不对
				if (netHead.nBufSize < sizeof(NET_USER_INFO))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;

					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                	
					break;
				}								
				
				// 接收用户的信息,包括用户名和密码
				memset(&userInfo, 0, sizeof(userInfo));
								
				#if 1
				ret = TcpReceive(hConnSock, &userInfo, sizeof(userInfo));
				
				if (ret < sizeof(userInfo))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_LOGON_FULL;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                	
					break;
				}
				#endif

				// 判断用户登陆个数是否已满,如果已满,则关闭该用户
				pthread_mutex_lock(&g_server_info.msgThreadMutex);
				nCurNum = g_server_info.msgProcessThreadNum;
				pthread_mutex_unlock(&g_server_info.msgThreadMutex);				
				if (nCurNum >= MAX_USER_CMD_CHANNEL_NUM)	
				{
					netHead.nFlag = HDVS_FLAG;

					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_LOGON_FULL;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                	
					break;
				}

				// 校验用户		
				
				NETSDK_SetUserCheckFunc(checkUser);
				if (g_server_info.funcCheckUserPsw)
				{
					MSG_HEAD msgHead;

					msgHead.nCmd = NETCMD_LOGON;
					msgHead.nSock = hConnSock;
					
					// 增加IP, 2007-11-13
					msgHead.nErrorCode = addr.sin_addr.s_addr;

					ret = g_server_info.funcCheckUserPsw(&msgHead, userInfo.szUserName, userInfo.szUserPsw);
					if (ret < 0)
					{
						printf("LOGIN: check error\n");

						netHead.nFlag = HDVS_FLAG;
						netHead.nCommand = NETCMD_LOGON;
						netHead.nErrorCode = NETERR_USER_PSW;
						netHead.nBufSize = 0;
						netHead.nReserve = 0;
					
						ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
						if (ret < 0)
						{

							net_debug();
						}
						sleep(1);
						shutdown(hConnSock, 2);
						close(hConnSock);

						break;
					}
					nRight = ret;
				}

				// 判断用户是否已经存在,如果存在,则先退出再登陆
				nClientID = ClientExist(addr.sin_addr.s_addr, addr.sin_port);
				if (nClientID)
				{					
					net_debug();
					ClientLogoff(nClientID);
				}

				// 处理用户登陆,加入命令通道的线程池
				ret = ClientLogon(addr, userInfo, hConnSock, nRight, TCP_FLAG);
				if (ret < 0)
				{
					printf("ClientLogon: check error\n");

					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_LOGON_FULL;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                		        		
					break;
				}

				// 登陆成功,通知命令线程池
				pthread_mutex_lock(&g_server_info.msgQuitThreadMutex);
				g_server_info.msgWaitThreadNum ++;
				pthread_mutex_unlock(&g_server_info.msgQuitThreadMutex);
				pthread_cond_signal(&g_server_info.msgThreadCond);
			}
			break;
						
		case NETCMD_OPEN_CHANNEL:	// 打开通道
  			{
				int nCurNum = 0;
				OPEN_CHANNEL openChannel;
				CLIENT_INFO clientInfo;

				memset(&clientInfo, 0, sizeof(CLIENT_INFO));
				memset(&openChannel, 0, sizeof(OPEN_CHANNEL));

				// 参数的大小不对
				if (netHead.nBufSize < sizeof(OPEN_CHANNEL))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                	
					break;
				}								
				
				// 接收客户端要打开的通道信息
				memset(&openChannel, 0, sizeof(OPEN_CHANNEL));
				ret = TcpReceive(hConnSock, &openChannel, sizeof(OPEN_CHANNEL));
				if (ret < sizeof(OPEN_CHANNEL))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
                	
					break;
				}

				// 	检验参数是否合法
				if (openChannel.nSerChn<0 || openChannel.nSerChn>=SERVER_MAX_CHANNEL)
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);        	
					
					break;
				}

				#if 1

				// 判断用户是否已经登陆
				if (!GetClient(openChannel.nID, &clientInfo))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_NOT_LOGON;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);        	
					//printf("MSG Socket: %d\n", openChannel.nID);
					break;

				}
                    
				// 判断打开通道个数是否超过最大限制
				pthread_mutex_lock(&g_server_info.dataThreadMutex);
				nCurNum = g_server_info.dataProcessThreadNum;
				pthread_mutex_unlock(&g_server_info.dataThreadMutex);
				if (nCurNum >= MAX_USER_DATA_CHANNEL_NUM)
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_CHANNEL_FULL;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);        	
					
					break;

				}         
				#endif
				

				//ret = RequestTcpPlay(openChannel, hConnSock);
				printf("NETCMD_OPEN_CHANNEL(%d): %d %d %d %d \n", hConnSock, openChannel.nSerChn, openChannel.nID, openChannel.nProtocolType, openChannel.nClientID);

				ret = RequestTcpPlay(openChannel, hConnSock, addr.sin_addr.s_addr);	// Add the by lvjh, 2008-02-28
				if (ret < 0)
				{
					break;
				}
				
				
				// 返回打开通道成功
				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_OPEN_CHANNEL;
				netHead.nErrorCode = 0;
				netHead.nBufSize = 0;
				netHead.nReserve = 0;
					
				ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
				if (ret < 0)
				{
					net_debug();
					sleep(1);
					shutdown(hConnSock, 2);
					close(hConnSock);
					break;
				}
			 	#if 1
				// 登陆成功,通知数据线程池
				pthread_mutex_lock(&g_server_info.dataQuitThreadMutex);
				g_server_info.dataWaitThreadNum++;
				pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
				pthread_cond_signal(&g_server_info.dataThreadCond); 
				#endif
			}
			break;
			
		case NETCMD_OPEN_TALK:
			{
				printf("NETCMD_OPEN_TALK \n");
				pthread_t thrdID;
				TALKTHRD_PARAM talkThrdParam;
				NET_DATA netData;
				TALK_PARAM *talkParam=NULL;
				
			/********************************************用于对讲互斥*****************************************///add by zhangjing 
				ret = audioGetTalkState();
				if(ret){
					printf("Talk alreay opend by another client \n");
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_TALK;
					netHead.nErrorCode = NETERR_TALK_OPENED;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					printf("NETCMD_OPEN_TALK: close %d\n", hConnSock);

					shutdown(hConnSock, 2);
					close(hConnSock);
					break;
				}
				
				printf("**.D--ZJ-NETCMD_OPEN_TALK: open %d (%s %d)\n", hConnSock , __FILE__, __LINE__);
			/*==============================================================*/	
				g_aenc_module_pause_flag[0] = 0;
				ret = audioDecModuleOpen(1000);  //add code by liujw  //mbl old:3
				if (ret)	
				{   
					printf("audioOutOpen(%s %d) Failed!\n", __FILE__, __LINE__);
					audioDecModuleClose();
					//return -1;	
				}
				else
				printf("audioOutOpen ok !\n");

				// 对讲请求
				//if (g_server_info.pCallbackRequestTalk(addr.sin_addr.s_addr, addr.sin_port) != 0)
				if (g_server_info.pCallbackRequestTalk(hConnSock, 0) != 0)
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_TALK;
					netHead.nErrorCode = NETERR_TALK;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					printf("NETCMD_OPEN_TALK: close %d\n", hConnSock);

					shutdown(hConnSock, 2);
					close(hConnSock);
					break;
				}

				// 返回对讲
				talkParam = (TALK_PARAM *)netData.pBuf;

				netData.netHead.nFlag = HDVS_FLAG;
				netData.netHead.nCommand = NETCMD_OPEN_TALK;
				netData.netHead.nErrorCode = 0;
				netData.netHead.nBufSize = sizeof(TALK_PARAM);
				netData.netHead.nReserve = 0;
				
#ifdef 	G726
				talkParam->nEncType = ENCODE_AUDIO_G726;//modify by zhangjing //通知客服端音频编码格式
#else				
				talkParam->nEncType = WAVE_FORMAT_ALAW;//modify by zhangjing //通知客服端音频编码格式
#endif
				talkParam->nAinChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAinBits = 16;
				talkParam->nAinSamples = AUDIOSAMPLES;
#ifdef 	G726
				talkParam->nDecType = ENCODE_AUDIO_G726;//modify by zhangjing //通知客服端音频编码格式
#else				
				talkParam->nDecType = WAVE_FORMAT_ALAW;//modify by zhangjing //通知客服端音频编码格式
#endif

				talkParam->nAoutChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAoutBits = 16;
				talkParam->nAoutSamples = AUDIOSAMPLES;   //mody by lv add
					
				ret = send(hConnSock, &netData, sizeof(NET_HEAD)+sizeof(TALK_PARAM), 0);
				if (ret != sizeof(NET_HEAD)+sizeof(TALK_PARAM))
				{
					shutdown(hConnSock, 2);
					close(hConnSock);
					break;
				}
				
				// 创建对讲线程
				talkThrdParam.hSock = hConnSock;
				talkThrdParam.ip = addr.sin_addr.s_addr;
				talkThrdParam.port = addr.sin_port;
				g_talkbackSock = talkThrdParam.hSock;  
				pthread_create(&thrdID, NULL, (void *)&TalkbackThread, &talkThrdParam);
				usleep(1000);
			}
			break;

		case NETCMD_REC_FILE:
			{
				if (g_server_info.funcFileTransfer != NULL)
				{
					pthread_t thrdID;
					ret = pthread_create(&thrdID, NULL, (void *)&SendRecordFileExt, (void *)&hConnSock);
					printf("NETCMD_REC_FILE: result: %d, socket:%d!\n", ret, hConnSock);
					usleep(1000);
				}
				else
				{
					// Add the code by lvjh, 2008-05-22
					shutdown(hConnSock, 2);
					close(hConnSock);
					printf("NETCMD_REC_FILE: %d Error!\n", hConnSock);
				}
			}
			break;
            
		default:
#if 1
			
			// 返回不支持的网络命令
			netHead.nFlag = HDVS_FLAG;
			//netHead.nCommand = 0;
			netHead.nErrorCode = NETERR_NOT_SUPPORT;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;

			printf("Can not support command(%d NET: %x)!\n", hConnSock, netHead.nCommand);
	
			ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
			if (ret < 0)
			{
				net_debug();
			}
			shutdown(hConnSock, 2);
			close(hConnSock);
			break;
#endif
		}
	}
	
    printf("%s %d\n", __func__, __LINE__);
	restartSystem();
	
	return 0;
}

// UDP监听线程只用来打开DATA通道 
int UdpListenThread()
{
	int i = 0;
	int ret = -1;	
	int hListenSock = -1;
	int hConnSock = -1;
	int nLen = 0;
	char buffer[UPD_RCV_BUFFER_LEN];
	char cmd_buffer[UPD_RCV_BUFFER_LEN];
	
	unsigned long	dwUserID;
	int p2p_server_register_flag = -1;
	int upnp_port;
	
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	struct sockaddr_in udp_hole_addr;
	struct sockaddr_in p2p_addr;
	struct sockaddr_in p2pserver_keepalive_addr;
	int p2psever_sock = -1;	

	NET_HEAD netHead;
	NET_MSG_HEAD HOSSNetHead;
	
	PTZ_CMD   ptzCmd;
	MSG_HEAD msgHead;
	OPEN_CHANNEL openChannel;
	COMMAND_HEAD p2p_net_comm;
	NET_PARAM net_param;

	DVSNET_UPNP_PARAM upnp_param;

	FD_ZERO(&fset);
	memset(&to, 0, sizeof(to));
	memset(&addr, 0, sizeof(addr));
	memset(&netHead, 0, sizeof(NET_HEAD));	

	getNetParam(&net_param);
	hListenSock = g_server_info.hUdpListenSock;
	if (hListenSock < 0)
	{
		return -1;
	}
	upnp_port = UPNP_MAPPING_PROT;

	TcpSendMsgToAll_UDPremote(REMOTE_IP, REMOTE_PORT, upnp_port);

	while (!g_server_info.bServerExit)
	{
#if 1	
		to.tv_sec  = P2P_SOCK_TIME_OUT;
		to.tv_usec = 0;
		FD_SET(hListenSock, &fset);
		ret = select(hListenSock+1, &fset, NULL, NULL, &to);
		
		// 判断服务器是否退出		
		if (g_server_info.bServerExit)
		{			
			break;
		}
		
		// SOCKET接收超时,继续
		if (ret == 0)
		{	
			if(p2p_server_register_flag != 0){
				p2p_server_register_flag = TcpSendMsgToAll_UDPremote(REMOTE_IP, REMOTE_PORT, upnp_port);
				printf("TcpSendMsgToAll_UDPremote success ! = %d \n", p2p_server_register_flag);
			}
			else{	
				memset(&p2p_addr,0,sizeof(p2p_addr));
				p2p_addr.sin_family=AF_INET;
				p2p_addr.sin_addr.s_addr=inet_addr(REMOTE_IP);
				p2p_addr.sin_port = htons(REMOTE_PORT);

				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_KEEP_ALIVE;
				netHead.nBufSize = 0;
				netHead.nReserve = dwUserID;
				ret = udpSendMsg(hListenSock, &netHead, sizeof(NET_HEAD), p2p_addr);
				if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
				{
					printf("NAT(%d): Keepalive Error!\n", hListenSock);
					break;
				}
			}	
			continue;
		}
		// SOCKET接收错误,判断是否是中断引起，如果是，继续，否则重启系统
		if (ret == -1)
		{		
			net_debug();			
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				printf("UDP REBOOT:\n");
				restartSystem();
				break;
			}
			
		}
		
		// 判断SELECT是否设置，如果没有，则继续
		if (!FD_ISSET(hListenSock, &fset))
		{
			continue;
		}
#endif

		// 接收网络帧头,如果出现错误,则关闭SOCKET
		nLen = sizeof(addr);
		ret = recvfrom(hListenSock, buffer, UPD_RCV_BUFFER_LEN, MSG_WAITALL, (struct sockaddr*)&addr, (socklen_t *)&nLen);
		if (ret<=0 || ret<sizeof(NET_HEAD))
		{
			net_debug();
			continue;
		}
		memcpy(&netHead, buffer, sizeof(NET_HEAD));
	//	printf("UDP: %s %d %d %d \n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), netHead.nCommand, ret);
	
		// 处理网络命令
		switch(netHead.nCommand)
		{
#if 1
			case NETCMD_P2P_CMD:
				memcpy(&p2p_net_comm, buffer+sizeof(NET_HEAD), sizeof(COMMAND_HEAD));	
				printf("p2p_net_comm.nCmdID = %p\n", p2p_net_comm.nCmdID);
				switch(p2p_net_comm.nCmdID)
				{
					//PC端请求IPC打洞 PC---->P2P Server----->IPC	
					case NETCMD_P2P_REQHOLE:
						printf("CMD: NETCMD_P2P_REQHOLE\n");

						DVSNET_REQHOLE_INFO hole_info;
						DVSNET_ENDHOLE_INFO hole_end_info;
						NET_HEAD hole_netHead;
						NET_HEAD udp_hole_info;
						COMMAND_HEAD hole_net_comm;
						DVSNET_CMDRECEIVED_INFO cmd_received_info;
						
						char hole_infor_return[1000];
						int  hole_send_lenght;
						int  clientSock;

						#if 1
						//添加确定命令机制
						memset(&hole_netHead, 0, sizeof(hole_netHead));
						memset(&udp_hole_addr,0,sizeof(udp_hole_addr));
						memset(&cmd_received_info, 0, sizeof(cmd_received_info));

						hole_netHead.nFlag = HDVS_FLAG;
						hole_netHead.nCommand = NETCMD_P2P_CMD;
						hole_netHead.nBufSize = sizeof(DVSNET_CMDRECEIVED_INFO) + sizeof(COMMAND_HEAD);

						hole_net_comm.nCmdID = NETCMD_P2P_CMDRECEIVED;
						hole_net_comm.nChannel = 0;
						hole_net_comm.nCmdLen = sizeof(DVSNET_CMDRECEIVED_INFO);
						
						cmd_received_info.dwCmdReceived = NETCMD_P2P_REQHOLE;
						cmd_received_info.dwUserID = dwUserID;
						
						memset(cmd_buffer, 0, 1024);
						memcpy(cmd_buffer, &hole_netHead, sizeof(NET_HEAD));
						memcpy(cmd_buffer + sizeof(NET_HEAD), &hole_net_comm, sizeof(COMMAND_HEAD));
						memcpy(cmd_buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &hole_end_info,  sizeof(DVSNET_CMDRECEIVED_INFO));
						hole_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_CMDRECEIVED_INFO);
						ret = udpSendMsg(hListenSock, cmd_buffer, hole_send_lenght, addr);
						if (ret <= 0 && errno == EPIPE)
						{
							printf("NAT(%d): Send Error!\n", hListenSock);
							return -1;
						}
						#endif
						printf("send p2p server cmd recive sussd!!!\n");

						memcpy(&hole_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQHOLE_INFO));
						
						//打洞
						udp_hole_info.nFlag = HDVS_FLAG;
						udp_hole_info.nCommand = NETCMD_P2P_CMD;
						udp_hole_info.nBufSize = 0;
						udp_hole_info.nReserve = 0;
						printf("udp hole IP = %s	port = %ld\n", hole_info.strIp, hole_info.dwPort);

					#if 1	
						clientSock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
						if(clientSock < 0)
						{
							 printf("create sock erroe!!\r\n");
							 return -1;
						}
						memset(&udp_hole_addr,0,sizeof(udp_hole_addr));
						udp_hole_addr.sin_family=AF_INET;
						udp_hole_addr.sin_addr.s_addr=inet_addr(hole_info.strIp);
						udp_hole_addr.sin_port = htons(hole_info.dwPort);
					#endif

						udpSendMsg(clientSock, &netHead, sizeof(NET_HEAD), udp_hole_addr);
						close(clientSock);
						printf("Make Hole has sucess!!\n");
										

						/*tell p2p hole success!*/
						hole_netHead.nFlag = HDVS_FLAG;
						hole_netHead.nCommand = NETCMD_P2P_CMD;
						hole_netHead.nBufSize = sizeof(DVSNET_ENDHOLE_INFO) + sizeof(COMMAND_HEAD);


						hole_net_comm.nCmdID = NETCMD_P2P_ENDHOLE;
						hole_net_comm.nChannel = 0;
						hole_net_comm.nCmdLen = sizeof(DVSNET_ENDHOLE_INFO);

						hole_end_info.dwReserved = 1;
						hole_end_info.dwUserID = hole_info.dwUserID;

						memset(buffer, 0, 1024);
						memcpy(buffer, &hole_netHead, sizeof(NET_HEAD));
						memcpy(buffer + sizeof(NET_HEAD), &hole_net_comm, sizeof(COMMAND_HEAD));
						memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &hole_end_info,  sizeof(DVSNET_ENDHOLE_INFO));
						hole_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_ENDHOLE_INFO);
						ret = udpSendMsg(hListenSock, buffer, hole_send_lenght, addr);
						if (ret <= 0 && errno == EPIPE)
						{
							printf("NAT(%d): Send Error!\n", hListenSock);
							return -1;
						}

						#if 0
						memset(buffer, 0, UPD_RCV_BUFFER_LEN);
						int counter = 0;
//						for(counter = 0; counter < 3; counter++){
						while(counter != 3)
						{
							ret = recvfrom(hListenSock, buffer, UPD_RCV_BUFFER_LEN, MSG_WAITALL, (struct sockaddr*)&addr, (socklen_t *)&nLen);
							if (ret<=0 || ret<sizeof(NET_HEAD))
							{
								net_debug();
								continue;
							}
							memcpy(&p2p_net_comm, buffer+sizeof(NET_HEAD), sizeof(COMMAND_HEAD));	
							printf("recvfrom p2p server %d %p %p....\n", ret, p2p_net_comm.nCmdID, NETCMD_P2P_CMDRECEIVED);
							if(p2p_net_comm.nCmdID == NETCMD_P2P_ENDHOLE)
							{
								printf("receive p2p server NETCMD_P2P_CMDRECEIVED\n");
								counter = 3;
								break;
							}
							else
							{
								//重发
								printf("resend..\n");
								hole_netHead.nFlag = HDVS_FLAG;
								hole_netHead.nCommand = NETCMD_P2P_CMD;
								hole_netHead.nBufSize = sizeof(DVSNET_ENDHOLE_INFO) + sizeof(COMMAND_HEAD);


								hole_net_comm.nCmdID = NETCMD_P2P_ENDHOLE;
								hole_net_comm.nChannel = 0;
								hole_net_comm.nCmdLen = sizeof(DVSNET_ENDHOLE_INFO);

								hole_end_info.dwReserved = 1;
								hole_end_info.dwUserID = hole_info.dwUserID;

								memset(buffer, 0, 1024);
								memcpy(buffer, &hole_netHead, sizeof(NET_HEAD));
								memcpy(buffer + sizeof(NET_HEAD), &hole_net_comm, sizeof(COMMAND_HEAD));
								memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &hole_end_info,  sizeof(DVSNET_ENDHOLE_INFO));
								hole_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_ENDHOLE_INFO);
								ret = udpSendMsg(hListenSock, buffer, hole_send_lenght, addr);
								if (ret <= 0 && errno == EPIPE)
								{
									printf("NAT(%d): Send Error!\n", hListenSock);
									return -1;
								}
								counter++;
							}
						}
						#endif
						
						break;
						
					//请求反向连接PC---->P2P Server------>IPC
					case NETCMD_P2P_REQCONBACK:
						printf("CMD: NETCMD_P2P_REQCONBACK\n");
					#if 1
						DVSNET_REQCONBACK_INFO conback_info;
						char buffer_return[1000];
						SYS_INFO sysInfo;
						NET_PARAM netParam;

						memcpy(&conback_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQCONBACK_INFO));

						NETSDK_NatPause();
						getSysInfoParam(&sysInfo);
						getNetParam(&netParam);
						memcpy(buffer_return, &sysInfo, sizeof(SYS_INFO));
						memcpy(buffer_return+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));

						NETSDK_NatSetup(conback_info.strConnectURL, conback_info.nPort, conback_info.nInterval, buffer_return, sizeof(SYS_INFO)+sizeof(NET_PARAM));

						if (conback_info.nOnFlag)
						{
							NETSDK_NatResume();
						}
				
					#endif	
						
						//反向连接结果IPC---------->P2P Server------>PC
						DVSNET_ENDCONBACK_INFO	result_endconback;
					#if 1
						result_endconback.dwReserved = 1;
						result_endconback.dwUserID = conback_info.dwUserID;
						
						ret = udpSendMsg(hListenSock, &result_endconback, sizeof(DVSNET_ENDCONBACK_INFO), addr);
						if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
						{
							printf("NAT(%d): Keepalive Error!\n", hListenSock);
							break;
						}
					#endif	
						break;
						
						case NETCMD_P2P_REGDATAPORT:
							printf("CMD: NETCMD_P2P_REGDATAPORT\n");
							DVSNET_REGISTER_INFO p2p_register_return_info;
							memcpy(&p2p_register_return_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REGISTER_INFO));
							dwUserID = p2p_register_return_info.dwUserID;
							g_server_info.dwUserID = p2p_register_return_info.dwUserID;
							printf("		p2p_register_return_info.wRemotePort = %d\n", p2p_register_return_info.wRemotePort);
							printf("g_p2p_UserID = %ld : %d\n\n\n", p2p_register_return_info.dwUserID, p2p_register_return_info.wLocalPort);
							p2p_server_register_flag = 0;
							to.tv_sec = P2P_SOCK_TIME_OUT;
							break;					
								
						default:
								printf("undefined command\n");
								break;
									
				}
				
				break;
#endif

		case NETCMD_OPEN_CHANNEL:	// 打开通道
			{
				int nCurNum = 10;
				CLIENT_INFO clientInfo;

				printf("open video channel\n");
				memcpy(&openChannel, buffer+sizeof(NET_HEAD), sizeof(OPEN_CHANNEL));
				memset(&clientInfo, 0, sizeof(CLIENT_INFO));
				g_server_info.hUdpListenSock = hListenSock;

				// 参数的大小不对
				if (netHead.nBufSize < sizeof(OPEN_CHANNEL))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
					if (ret < 0)
					{
						net_debug();
					}
					
					break;
				}								

				//	检验参数是否合法
				if (openChannel.nSerChn<0 || openChannel.nSerChn>=SERVER_MAX_CHANNEL)
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_CHANNEL;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
					if (ret < 0)
					{
						net_debug();
					}	
					
					break;
				}

				// 处理打开通道,加入数据通道的线程池
				ret = RequestUdpPlay(openChannel, addr.sin_addr.s_addr, addr.sin_port);
				if (ret < 0)
				{
					break;
				}
				
			}
			break;
		case NETCMD_LOGON:	// 用户登陆
			{
				printf("CMD: NETCMD_LOGON\n");
				int nCurNum = 0;
				int nRight = 0;
				int nClientID = 0;				  
				NET_USER_INFO userInfo; 			   
				
				// 参数的大小不对
				if (netHead.nBufSize < sizeof(NET_USER_INFO))
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;

					ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hListenSock, 2);
					close(hListenSock);
					
					break;
				}								
				
				// 接收用户的信息,包括用户名和密码
				memset(&userInfo, 0, sizeof(userInfo));
				memcpy(&userInfo, buffer+sizeof(NET_HEAD), sizeof(NET_USER_INFO));
				printf("userInfo.szUserName = %s \n userInfo.szUserPsw = %s\n", userInfo.szUserName, userInfo.szUserPsw);

				// 判断用户登陆个数是否已满,如果已满,则关闭该用户
				pthread_mutex_lock(&g_server_info.msgThreadMutex);
				nCurNum = g_server_info.msgProcessThreadNum;
				pthread_mutex_unlock(&g_server_info.msgThreadMutex);				
				if (nCurNum >= MAX_USER_CMD_CHANNEL_NUM)	
				{
					netHead.nFlag = HDVS_FLAG;

					netHead.nCommand = NETCMD_LOGON;
					netHead.nErrorCode = NETERR_LOGON_FULL;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
					
					//ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
					ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
					shutdown(hListenSock, 2);
					close(hListenSock);
					
					break;
				}

				// 校验用户 	
					NETSDK_SetUserCheckFunc(checkUser);
				if (g_server_info.funcUdpCheckUserPsw)
				{
					MSG_HEAD msgHead;

					msgHead.nCmd = NETCMD_LOGON;
					msgHead.nSock = hListenSock;
					
					// 增加IP, 2007-11-13
					msgHead.nErrorCode = addr.sin_addr.s_addr;

					ret = g_server_info.funcUdpCheckUserPsw(&msgHead, userInfo.szUserName, userInfo.szUserPsw, addr);
					if (ret < 0)
					{
						printf("LOGIN: check error\n");

						netHead.nFlag = HDVS_FLAG;
						netHead.nCommand = NETCMD_LOGON;
						netHead.nErrorCode = NETERR_USER_PSW;
						netHead.nBufSize = 0;
						netHead.nReserve = 0;
					  //ret = send(hConnSock, &netHead, sizeof(NET_HEAD), 0);
						ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
						if (ret < 0)
						{

							net_debug();
						}
						sleep(1);
						shutdown(hListenSock, 2);
						close(hListenSock);

						break;
					}
					nRight = ret;
				}

			}
			break;
						
	case NETCMD_OPEN_TALK:
			{
				printf("CMD: NETCMD_OPEN_TALK\n");
				pthread_t thrdID;
				TALKTHRD_PARAM talkThrdParam;
				NET_DATA netData;
				TALK_PARAM *talkParam=NULL;
				
				printf("NETCMD_OPEN_TALK: open %d (%s %d)\n", hListenSock , __FILE__, __LINE__);
				g_aenc_module_pause_flag[0] = 0;
				open_talk();
				ret = audioDecModuleOpen(1000);  
				if (ret)	
				{	
					printf("audioOutOpen(%s %d) Failed!\n", __FILE__, __LINE__);
					audioDecModuleClose();
					//return -1;	
				}
				else

				// 对讲请求
				if (g_server_info.pUdpCallbackRequestTalk(hListenSock, 0, addr) != 0)
				{
					netHead.nFlag = HDVS_FLAG;
					netHead.nCommand = NETCMD_OPEN_TALK;
					netHead.nErrorCode = NETERR_TALK;
					netHead.nBufSize = 0;
					netHead.nReserve = 0;
				
					ret = sendto(hListenSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
					if (ret < 0)
					{
						net_debug();
					}
					sleep(1);
				
					shutdown(hListenSock, 2);
					close(hListenSock);
					break;
				}
				
				// 返回对讲
				talkParam = (TALK_PARAM *)netData.pBuf;

				netData.netHead.nFlag = HDVS_FLAG;
				netData.netHead.nCommand = NETCMD_OPEN_TALK;
				netData.netHead.nErrorCode = 0;
				netData.netHead.nBufSize = sizeof(TALK_PARAM);
				netData.netHead.nReserve = 0;
		
#ifdef 	G726
				talkParam->nEncType = ENCODE_AUDIO_G726;//modify by zhangjing //通知客服端音频编码格式
#else				
				talkParam->nEncType = WAVE_FORMAT_ALAW;//modify by zhangjing //通知客服端音频编码格式
#endif
				talkParam->nAinChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAinBits = 16;
				talkParam->nAinSamples = g_server_info.avInfoStream1[0].nAudioSamples;
#ifdef 	G726
				talkParam->nDecType = ENCODE_AUDIO_G726;//modify by zhangjing //通知客服端音频编码格式
#else				
				talkParam->nDecType = WAVE_FORMAT_ALAW;//modify by zhangjing //通知客服端音频编码格式
#endif
				talkParam->nAoutChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAoutBits = 16;
				talkParam->nAoutSamples = 8000;  
					
				ret = sendto(hListenSock, &netData, sizeof(NET_HEAD)+sizeof(TALK_PARAM), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
				if (ret != sizeof(NET_HEAD)+sizeof(TALK_PARAM))
				{
					shutdown(hListenSock, 2);
					close(hListenSock);
					break;
				}
			
				// 创建对讲线程
				talkThrdParam.hSock = hListenSock;
				talkThrdParam.addr = addr;
				talkThrdParam.ip = addr.sin_addr.s_addr;
				talkThrdParam.port = addr.sin_port;
				talkThrdParam.nflag = UDP_FLAG;
				g_talkbackSock = talkThrdParam.hSock;  
				
				pthread_create(&thrdID, NULL, (void *)&UdpTalkbackThread, &talkThrdParam);
				usleep(1000);
			}
			break;

		case NETCMD_CLOSE_TALK:
				printf("CMD: NETCMD_CLOSE_TALK\n");
				close_talk();
			break;

		case NETCMD_REC_FILE:
				{
					printf("CMD: NETCMD_REC_FILE\n");
					RECTHRD_PARAM recThrdParam;
					recThrdParam.hSock = hListenSock;
					recThrdParam.addr = addr;
					recThrdParam.ip = addr.sin_addr.s_addr;
					recThrdParam.port = addr.sin_port;
					memcpy(recThrdParam.buffer, buffer, UPD_RCV_BUFFER_LEN);

					sendUdpRecordFile(hListenSock, addr, buffer);
				break;			
			case NETCMD_KEEP_ALIVE:
				printf("CMD: NETCMD_KEEP_ALIVE\n");
				printf("P2P SERVER SEND dwUserID = %d\n", netHead.nReserve);
				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_KEEP_ALIVE;
				netHead.nBufSize = 0;
				if(netHead.nReserve == dwUserID){
					printf("**************************************************\n");
					TcpSendMsgToAll_UDPremote(REMOTE_IP, REMOTE_PORT, NETCMD_NAT_CONNECT);
					printf("		TcpSendMsgToAll_UDPremote success ! \n");
					printf("**************************************************\n");
				}
				break;

			default:
				msgHead.nSock = hListenSock;
				msgHead.addr = addr;
				msgHead.nCmd = netHead.nCommand;
				msgHead.nRight = 3;
				msgHead.nErrorCode = 0;
				msgHead.nBufSize = netHead.nBufSize;
				msgHead.nflag  = UDP_FLAG;
				g_server_info.nupdate_flag = UDP_FLAG;
				
				ParseCommand(&msgHead, buffer+sizeof(NET_HEAD));
				break;
			}
		}
	}
	
	restartSystem();
	
	return 0;
}
	


// 解析命令
int ParseCommand(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	BOOL bInner = FALSE;
	NET_HEAD netHead;
	OPEN_CHANNEL *pOpenChn = NULL;
	CLIENT_INFO clientInfo;
	char *pTmp = NULL;
	NET_UPDATE_FILE_HEAD *pUpdateHead = NULL;
	char buffer_update[512*1024];
    
	//printf("ParseCommand ...\n");

	// 检查参数的合法性
	if (pMsgHead == NULL)
	{
		return -1;
	}

	// 命令处理
	switch (pMsgHead->nCmd)
	{
	case NETCMD_OPEN_MULTI_CHANNEL:
		bInner = TRUE;

		//printf("NETCMD_OPEN_MULTI_CHANNEL: ... \n");

		// 检查参数的合法性
		pOpenChn = (OPEN_CHANNEL *)pRecvBuf;
		if (pOpenChn == NULL)
		{
			printf("NETCMD_OPEN_MULTI_CHANNEL: ERROR1 \n");

			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_OPEN_MULTI_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;
					
			if(pMsgHead->nflag == TCP_FLAG)
			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
			else if(pMsgHead->nflag == UDP_FLAG)
			ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
			if (ret < 0)
			{
				net_debug();
			}
		}

		if (pOpenChn->nSerChn<0 || pOpenChn->nSerChn>=SERVER_MAX_CHANNEL)
		{
			printf("NETCMD_OPEN_MULTI_CHANNEL: ERROR2 \n");

			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_OPEN_MULTI_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;
					
			if(pMsgHead->nflag == TCP_FLAG)
			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
			else if(pMsgHead->nflag == UDP_FLAG)
			ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
			if (ret < 0)
			{
				net_debug();
			}
		}

		#if 0
		// 判断用户是否存在
		if (!GetClient(pOpenChn->nID, &clientInfo))
		{
			printf("NETCMD_OPEN_MULTI_CHANNEL: ERROR3 \n");

			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_OPEN_MULTI_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;
					
			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
			if (ret < 0)
			{
				net_debug();
			}        	
			break;
		}
		#endif

		printf("NETCMD_OPEN_MULTI_CHANNEL: %d %d %d %d \n", pOpenChn->nSerChn, pOpenChn->nID, pOpenChn->nProtocolType, pOpenChn->nClientID);
        
		// 判断用户是否存在
		//if (!ExistMultiUser(*pOpenChn))
		{
			RequestMultiPlay(*pOpenChn);
		}
		//printf("NETCMD_OPEN_MULTI_CHANNEL: 1\n");
		// 返回
		netHead.nFlag = HDVS_FLAG;
		netHead.nCommand = NETCMD_OPEN_MULTI_CHANNEL;
		netHead.nErrorCode = 0;
		netHead.nBufSize = 0;
		netHead.nReserve = 0;
					
		if(pMsgHead->nflag == TCP_FLAG)
		ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
		else if(pMsgHead->nflag == UDP_FLAG)
		ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			net_debug();
		}	
		//printf("NETCMD_OPEN_MULTI_CHANNEL: 2\n");
		break;
        
	case NETCMD_CLOSE_CHANNEL:
		bInner = TRUE;
		
		// 检查参数的合法性
		pOpenChn = (OPEN_CHANNEL *)pRecvBuf;
		if (pOpenChn == NULL)
		{
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_OPEN_MULTI_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;
			
			if(pMsgHead->nflag == TCP_FLAG)
			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
			else if(pMsgHead->nflag == UDP_FLAG)
			ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
			if (ret < 0)
			{
				net_debug();
			}
			
			break;
		}

		printf("NETCMD_CLOSE_CHANNEL: %d %d %d %d\n", pOpenChn->nSerChn, pOpenChn->nID, pOpenChn->nProtocolType, pOpenChn->nClientID);

		if (pOpenChn->nSerChn<0 || pOpenChn->nSerChn>=SERVER_MAX_CHANNEL)
		{
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_CLOSE_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;
					
			if(pMsgHead->nflag == TCP_FLAG)
			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
			else if(pMsgHead->nflag == UDP_FLAG)
			ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
			if (ret < 0)
			{
				net_debug();
			}
			
			break;
		}

		// 判断用户是否存在
		#if 0
		if (!GetClient(pOpenChn->nID, &clientInfo))
		{
   			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_CLOSE_CHANNEL;
			netHead.nErrorCode = NETERR_ILLEGAL_PARAM;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;

			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
			if (ret < 0)
			{
				net_debug();
			}
			break;
		}
		#endif
		
        
		if (PROTOCOL_TCP == pOpenChn->nProtocolType)
		{
        	printf("NETCMD_CLOSE_CHANNEL: TCP\n");
		}
		else if(PROTOCOL_UDP == pOpenChn->nProtocolType)
		{
			printf("NETCMD_CLOSE_CHANNEL: UDP\n");
			CloseUdpPlay(*pOpenChn);
		}
		else if(PROTOCOL_MULTI == pOpenChn->nProtocolType)
		{
			printf("NETCMD_CLOSE_CHANNEL: MULTI\n");
			CloseMultiPlay(*pOpenChn);
		}
		
		// 返回
		netHead.nFlag = HDVS_FLAG;
		netHead.nCommand = NETCMD_CLOSE_CHANNEL;
		netHead.nErrorCode = 0;
		netHead.nBufSize = 0;
		netHead.nReserve = 0;
	#if 0				
		ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), MSG_NOSIGNAL);
		if (ret < 0)
		{
			net_debug();
		}
	#endif


		if(pMsgHead->nflag == TCP_FLAG)
		ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
		else if(pMsgHead->nflag == UDP_FLAG)
		ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			net_debug();
		}
	
		break;
	case NETCMD_UPDATE:
		bInner = TRUE;

		pUpdateHead = (NET_UPDATE_FILE_HEAD *) pRecvBuf;

		//printf("UPDATE(%d): %d %d\n", pMsgHead->nBufSize, pUpdateHead->nBlockNo, pUpdateHead->nFileOffset);


#if 1
		// Add the code by lvjh, 2009-02-13
		if (g_server_info.nCurUpdateSock!=pMsgHead->nSock && g_server_info.nUpdateFlag==1)
		{
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_UPDATE;
			netHead.nErrorCode = -1;
			netHead.nBufSize = 0;
			netHead.nReserve = 0;

			ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
			
			printf("Client: %d Update Failed(%d)!\n", pMsgHead->nSock, g_server_info.nUpdateFlag);
			
			break;
		}
		
		if (pUpdateHead->nBlockNo == 0)
		{
			// 如果文件太大则，直接返回失败，不再升级
			if (pUpdateHead->nFileLen >= MAX_UPDATE_FILE_LEN)
			{
				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_UPDATE;
				netHead.nErrorCode = -1;
				netHead.nBufSize = 0;
				netHead.nReserve = 0;
	
				if(pMsgHead->nflag == TCP_FLAG)
				ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
				else if(pMsgHead->nflag == UDP_FLAG)
				ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
			
				printf("Client: %d Update Failed(%d): update file too large!\n", pMsgHead->nSock, g_server_info.nUpdateFlag);
				
				break;
			}
			
			// Add the code by lvjh, 2009-02-13
			g_server_info.nUpdateFlag = 1;
			g_server_info.nCurUpdateSock = pMsgHead->nSock;
			
			if (g_server_info.pUpdateFileBuf)
			{
				free(g_server_info.pUpdateFileBuf);
				g_server_info.pUpdateFileBuf = NULL;			
			}
			//g_server_info.pUpdateFileBuf = malloc(pUpdateHead->nFileLen+sizeof(NET_UPDATE_FILE_HEAD));
			g_server_info.pUpdateFileBuf = malloc(pUpdateHead->nFileLen+sizeof(NET_UPDATE_FILE_HEAD)+1024);	// Add the code by lvjh, 2009-09-03 
			if (g_server_info.pUpdateFileBuf == NULL)
			{
				printf("malloc(): failed!\n");
				net_debug();
				// Add the code by lvjh, 2009-02-13
				g_server_info.nUpdateFlag = 0;
				g_server_info.nCurUpdateSock = 0;
				break;
			}
			
			// test
			printf("malloc(%p): %d\n", g_server_info.pUpdateFileBuf, pUpdateHead->nFileLen+sizeof(NET_UPDATE_FILE_HEAD));
			memset(g_server_info.pUpdateFileBuf, 0, pUpdateHead->nFileLen+sizeof(NET_UPDATE_FILE_HEAD)+1024);

			memcpy(g_server_info.pUpdateFileBuf+pUpdateHead->nFileOffset, pRecvBuf, pUpdateHead->nBlockSize+sizeof(NET_UPDATE_FILE_HEAD));

			//pTmp = pRecvBuf+sizeof(NET_UPDATE_FILE_HEAD);
			//printf("%d %d %d %d\n", pTmp[0], pTmp[1], pTmp[2], pTmp[3]);

			printf("Updating: %d\n", pMsgHead->nSock);
			printf("@");
			
			// return 
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_UPDATE;
			netHead.nErrorCode = 0;
			netHead.nBufSize = sizeof(NET_UPDATE_FILE_HEAD);
			netHead.nReserve = 0;

			if(pMsgHead->nflag == TCP_FLAG){
				ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
				if (ret < 0)
				{
					net_debug();
					
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				} 
				ret = send(pMsgHead->nSock, pUpdateHead, sizeof(NET_UPDATE_FILE_HEAD), 0);
				if (ret < 0)
				{
					net_debug();
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				}
			}
			else if(pMsgHead->nflag == UDP_FLAG){
				memset(buffer_update, 0, 512*1024);
				memcpy(buffer_update, &netHead, sizeof(NET_HEAD));
				memcpy(buffer_update+sizeof(NET_HEAD), 	pUpdateHead, sizeof(NET_UPDATE_FILE_HEAD));
				
				ret = sendto(pMsgHead->nSock, buffer_update, sizeof(NET_HEAD)+sizeof(NET_UPDATE_FILE_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
				if (ret < 0)
				{
					net_debug();
					
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				} 
			}
		

			// Add the code by lvjh, 2008-01-29
			if (pUpdateHead->nFileLen <= 1024)
			{
				printf("@\n");
				printf("Update File End!\n");
				
				pMsgHead->nCmd = NETCMD_UPDATE;
				pMsgHead->nBufSize = pUpdateHead->nFileLen;
				pMsgHead->nRight = 3;	//
				g_server_info.funcServerRecv(pMsgHead, g_server_info.pUpdateFileBuf);	
				
				// Add the code by lvjh, 2009-02-13
				g_server_info.nUpdateFlag = 0;
				g_server_info.nCurUpdateSock = 0;
				if (g_server_info.pUpdateFileBuf)
				{
					free(g_server_info.pUpdateFileBuf);
					g_server_info.pUpdateFileBuf = NULL;			
				}
			}
			printf("*");
		}
		else
		{
			if (NULL == g_server_info.pUpdateFileBuf)
			{
				// Add the code by lvjh, 2009-02-13
				g_server_info.nUpdateFlag = 0;
				g_server_info.nCurUpdateSock = 0;
				if (g_server_info.pUpdateFileBuf)
				{
					free(g_server_info.pUpdateFileBuf);
					g_server_info.pUpdateFileBuf = NULL;			
				}
				
				break;
			}
			
			printf(">");
			
			memcpy(g_server_info.pUpdateFileBuf+pUpdateHead->nFileOffset+sizeof(NET_UPDATE_FILE_HEAD), 
				pRecvBuf+sizeof(NET_UPDATE_FILE_HEAD), pUpdateHead->nBlockSize);

			memcpy(g_server_info.pUpdateFileBuf, pRecvBuf, sizeof(NET_UPDATE_FILE_HEAD));
			
			// return 
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_UPDATE;
			netHead.nErrorCode = 0;
			netHead.nBufSize = sizeof(NET_UPDATE_FILE_HEAD);
			netHead.nReserve = 0;

		 if(pMsgHead->nflag == TCP_FLAG){
				ret = send(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0);
				if (ret < 0)
				{
					net_debug();
					
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				} 
				ret = send(pMsgHead->nSock, pUpdateHead, sizeof(NET_UPDATE_FILE_HEAD), 0);
				if (ret < 0)
				{
					net_debug();
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				}
			}
			else if(pMsgHead->nflag == UDP_FLAG){
				memset(buffer_update, 0, 512*1024);
				memcpy(buffer_update, &netHead, sizeof(NET_HEAD));
				memcpy(buffer_update+sizeof(NET_HEAD), 	pUpdateHead, sizeof(NET_UPDATE_FILE_HEAD));
				
				ret = sendto(pMsgHead->nSock, buffer_update, sizeof(NET_HEAD)+sizeof(NET_UPDATE_FILE_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
				if (ret < 0)
				{
					net_debug();
					
					// Add the code by lvjh, 2009-02-13
					g_server_info.nUpdateFlag = 0;
					g_server_info.nCurUpdateSock = 0;
					if (g_server_info.pUpdateFileBuf)
					{
						free(g_server_info.pUpdateFileBuf);
						g_server_info.pUpdateFileBuf = NULL;			
					}
					printf("$\n");
					break;
				} 
			}

			if (pUpdateHead->nBlockNo == pUpdateHead->nBlockNum-1)
			{
				printf("#\n");
				//printf("Update File End!\n");

				pMsgHead->nCmd = NETCMD_UPDATE;
				pMsgHead->nBufSize = pUpdateHead->nFileLen;
				pMsgHead->nRight = 3;;
			
				g_server_info.funcServerRecv(pMsgHead, g_server_info.pUpdateFileBuf);	
				
				// Add the code by lvjh, 2009-02-13
				g_server_info.nUpdateFlag = 0;
				g_server_info.nCurUpdateSock = 0;
				if (g_server_info.pUpdateFileBuf)
				{
					free(g_server_info.pUpdateFileBuf);
					g_server_info.pUpdateFileBuf = NULL;			
				}
			}
		}
#endif	
		break;

	case NETCMD_OPEN_CHANNEL_R:
		bInner = TRUE;
		natOpenChannel(pMsgHead, pRecvBuf);
		break;

	case NETCMD_OPEN_TALK_R:
		bInner = TRUE;
		tcpNatConnect(NETCMD_OPEN_TALK_R);
		break;

	default:
		bInner = FALSE;
		//printf("Not Support NET_CMD(%x)\n", pMsgHead->nCmd);
		break;

		#if 0
		bInner = FALSE;
		netHead.nFlag = HDVS_FLAG;
		//netHead.nCommand = 0;
		netHead.nErrorCode = NETERR_NOT_SUPPORT;
		netHead.nBufSize = 0;
		netHead.nReserve = 0;
		
		printf("Not Support NET_CMD(%x)\n", pMsgHead->nCmd);
		ret = sendto(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&pMsgHead->addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			net_debug();
		}
		break;
		#endif
	
	}

	NETSDK_SetServerRecvFunc(recvCmdProcFun);

	if (!bInner && g_server_info.funcServerRecv!=NULL)
	{
		g_server_info.funcServerRecv(pMsgHead, pRecvBuf);
	}
    
	return TRUE;
}

// TCP监听SOCKET创建和设置
int TcpSockListen(int ip, int port)
{
	int ret = -1;
	int hSock = -1;
	int opt = -1;	
	socklen_t len = 0;
	struct sockaddr_in addr;
	
	// 判断输入参数的合法性
	if (ip<0 || port<0)
	{
		return -1;
	}
	
	printf("sssip = %d\n", ip);

	// 创建SOCKET
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	hSock = socket(AF_INET, SOCK_STREAM, 0);
	if (hSock < 0)
	{
		net_debug();
		return -1;
	}

	
	// 设置SOCKET的属性
	
	//while(ret == -1)
	{
#if 1
		opt = 1;
		ret = setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
		//	break;
		}
		opt = 1;
		ret = setsockopt(hSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
		//	break;
		}
		// 设置发送BUFFER的大小
		opt = SOCK_SNDRCV_LEN;
		ret = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
		//	break;
		}
		// 设置接收BUFFER的大小
		opt = SOCK_SNDRCV_LEN;
		ret = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
		//	break;
		}
		// 获取发送BUFFER的大小
		opt = sizeof(len);
		ret = getsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &len, (socklen_t *)&opt);
		if (ret < 0)
		{
			net_debug();
		//	break;
		}
		// 获取接收BUFFER的大小
		opt = sizeof(len);
		ret = getsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &len, (socklen_t *)&opt);
		if (ret < 0)
		{
			net_debug();
		//	break;
		}


		// Add the code by lvjh, 2008-04-17
		opt = 0;
		ret = setsockopt(hSock, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt));
#endif

		// 绑定SOCKET
		ret = bind(hSock, (struct sockaddr *)&addr, sizeof(addr));
		if (ret < 0)
		{
			net_debug();
			//break;
		}
		// 监听
		ret = listen(hSock, 10);
		if (ret < 0)
		{
			net_debug();
			//break;
		}
	}
	// 如果SOCKET设置失败，则关闭SOCKET
	if (ret < 0)
	{
		shutdown(hSock, 2);
		close(hSock);
		printf("set error\n");
		return -1;
	}

	return hSock;
}


// UDP监听SOCKET创建和设置
int UdpSockListen(int ip, int port)
{
	int ret = -1;
	int hSock = -1;
	int opt = -1;
	struct sockaddr_in addr;

	// 判断输入参数的合法性
	if (ip<0 || port<0)
	{
		return -1;
	}

	// 创建SOCKET
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(ip);
	addr.sin_port = htons(port);
	hSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (hSock < 0)
	{
		net_debug();
		return -1;
	}
	
	// 设置SOCKET的属性
	do
	{
		//
		opt = 1;
		ret = setsockopt(hSock, SOL_SOCKET,SO_REUSEADDR, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
			break;
		}
		// 设置发送BUFFER的大小
		opt = SOCK_SNDRCV_LEN;
		ret = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
			break;
		}
		// 设置接收BUFFER的大小
		opt = SOCK_SNDRCV_LEN;
		ret = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
		if (ret < 0)
		{
			net_debug();
			break;
		}
		// Add the code by lvjh, 2008-04-17
		opt = 0;
		ret = setsockopt(hSock, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt));		

		// 绑定SOCKET
		ret = bind(hSock, (struct sockaddr *)&addr, sizeof(addr));
		if (ret < 0)
		{
			net_debug();
			break;
		}
	}while(FALSE);

	// 如果SOCKET设置失败，则关闭SOCKET
	if (ret < 0)
	{
		shutdown(hSock, 2);
		close(hSock);
		return -1;
	}
	
	return hSock;
}

// 设置连接SOCKET属性
int SetConnSockAttr(int hSock, int nTimeOver)
{
	int ret = -1;
	int opt = 0;	
	int len = 0;
	
	opt = 1;
	ret = setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	
	opt = 1;
	ret = setsockopt(hSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	
	// 设置发送BUFFER的大小
	opt = SOCK_SNDRCV_LEN;
	//opt = 8196;
	ret = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	// 设置接收BUFFER的大小
	opt = SOCK_SNDRCV_LEN;
	//opt = 8196;
	ret = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	// 获取发送BUFFER的大小
	opt = sizeof(len);
	ret = getsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &len, (socklen_t *)&opt);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	//printf("SO_SNDBUF: %d\n", len);

	// 获取接收BUFFER的大小
	opt = sizeof(len);
	ret = getsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &len, (socklen_t *)&opt);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	//printf("SO_RCVBUF: %d\n", len);
	
	return 0;
}

int SetConnSockAttrExt(int hSock, int nTimeOver)
{
	int ret = -1;
	int opt = 0;	
	int len = 0;
	
	opt = 1;
	ret = setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	/*
	opt = 1;
	ret = setsockopt(hSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	*/
	// 设置发送BUFFER的大小
	opt = SOCK_SNDRCV_LEN;
	//opt = 8196;
	ret = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	// 设置接收BUFFER的大小
	opt = SOCK_SNDRCV_LEN;
	//opt = 8196;
	ret = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	// 获取发送BUFFER的大小
	opt = sizeof(len);
	ret = getsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &len, (socklen_t *)&opt);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	//printf("SO_SNDBUF: %d\n", len);

	// 获取接收BUFFER的大小
	opt = sizeof(len);
	ret = getsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &len, (socklen_t *)&opt);
	if (ret < 0)
	{
		net_debug();
		return -1;
	}
	//printf("SO_RCVBUF: %d\n", len);
	
	return 0;
}

int FreeTSD(void *pParam)
{
	free((READ_POSITION*)pParam);
    
	return 0;
}

unsigned long MakeLong(unsigned short low, unsigned short hi)
{
	return (unsigned long)(low | (((unsigned long)hi) << 16));
}

unsigned short HeightWord(unsigned long value)
{
	return (unsigned short)((value >> 16) && 0xFFFF);
}

unsigned short LowWord(unsigned long value)
{
	return (unsigned short)value;
}

int GetTime()
{
	struct timeval tv;
	
	gettimeofday(&tv,NULL);
	
	return (tv.tv_sec*1000+tv.tv_usec/1000);
}

int NETSDK_GetUserLoginNum()
{
	return g_server_info.msgProcessThreadNum;
}

int NETSDK_GetOpenChannelNum()
{
	return g_server_info.dataProcessThreadNum;
}

int restartSystem()
{
	printf("NETSDK: reboot!\n");
	//system("reboot -f ");
	
	return 0;
}

int NETSDK_SetBandWidth(int nBandWidth)
{
	int value = 0;
	float temp = 0;
	
	if (nBandWidth > 4096000 || nBandWidth<64000)
	{
		return -1;
	}

	//temp = (512000.0/nBandWidth)*1000.0;
	//temp = (400000.0/nBandWidth)*1000.0;
	//temp = (800000.0/nBandWidth)*1000.0;

	temp = nBandWidth/8.0/(sizeof(NET_DATA_PACKET)+54);
	
	g_send_sleep = (int)(1000000/(temp+1));

	printf("BandWidth: %d---%d\n", nBandWidth, g_send_sleep);

	return 0;
}

int get_sleep_time()
{
	return g_send_sleep;
}


int NETSDK_ServerInit(int flag)
{
	return 0;	// Add the code by lvjh, 2008-11-21

	FILE *fp = NULL;
	
	fp = fopen("/proc/sys/net/ipv4/tcp_congestion_control", "r+");
	if (fp == NULL)
	{
		return -1;
	}
	
	switch (flag)
	{
	case 1:
		fwrite("bic", 1, 3, fp);
		break;

	case 2:
		fwrite("westwood", 1, 8, fp);
		break;

	case 3:
		fwrite("highspeed", 1, 9, fp);
		break;

	case 4:
		fwrite("hybla", 1, 5, fp);
		break;

	case 5:
		fwrite("htcp", 1, 4, fp);
		break;

	case 6:
		fwrite("vegas", 1, 5, fp);
		break;

	case 7:
		fwrite("scalable", 1, 8, fp);
		break;

	default:
		break;
	}

	fclose(fp);

	return 0;
}

int NETSDK_SetWireIP(char *addr)
{
	if (addr == NULL)
	{
		return -1;	
	}
	
	strncpy(g_wire_ip, addr, 16);
	
	return 0;
}

int NETSDK_SetWirelessIP(char *addr)
{
	if (addr == NULL)
	{
		return -1;	
	}
	
	strncpy(g_wireless_ip, addr, 16);
	
	return 0;
}
