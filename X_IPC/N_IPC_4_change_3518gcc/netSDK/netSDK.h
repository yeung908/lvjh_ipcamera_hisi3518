#ifndef __NET_SDK_H_
#define __NET_SDK_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "netcomm.h"

// 定义服务器最大通道

#ifdef QUAD_CHANNEL
	#define SERVER_MAX_CHANNEL		4
#else
	#define SERVER_MAX_CHANNEL		1
#endif

#define TRUE						1
#define FALSE						0
#define BOOL						int
#define WORD						unsigned short
#define DWORD						unsigned long
#define BYTE						unsigned char
#define __int64						unsigned long long

#define SOCK_SNDRCV_LEN				(1024*512)
#define SOCK_TIME_OUT				5
//#define P2P_SOCK_TIME_OUT			3
#define P2P_SOCK_TIME_OUT			60*3

#define MAX_POOL_SIZE				1024


#define UPD_RCV_BUFFER_LEN          (1024*256)
#define TCP_RCV_BUFFER_LEN          (1024)

#define TCP_FLAG					0
#define UDP_FLAG					1


#define MAX_USER_CMD_CHANNEL_NUM	20

#define MAX_USER_DATA_CHANNEL_NUM	MAX_USER_CMD_CHANNEL_NUM	// 最大用户命令通道数

#define MAX_UPDATE_FILE_LEN			8388608

#define  WAVE_FORMAT_ALAW       0x0006  /*  Microsoft Corporation  */
#define  WAVE_FORMAT_MULAW      0x0007  /*  Microsoft Corporation  */


typedef enum
{
	READY = 0,
	RUN = 1,
	STOP = 2
}NODE_STATUS;

typedef struct _MSG_HEAD
{
	int nSock;						// 客户端连接上来的SOCKET
	int nCmd;						// NET命令
	int nRight;						// 客户的权限
	int nErrorCode;					// 错误代码
	int nBufSize;					// 紧随其后的BUFFER数据的大小
	int nflag;						// UDP TCP 网络表示
	struct sockaddr_in addr;        // UDP addr;
	
    
}MSG_HEAD,*PMSG_HEAD;

// 发送数据的节点
typedef struct _SEND_NODE
{
	PROTOCOL_TYPE nType;			// 网络协议类型
	unsigned int nStreamType;		// 码流类型，大码流或小码流
	unsigned int nSerChn;			// 要打开的服务的通道号
	unsigned int nTcpAddr;			// 客户端的IP地址
	unsigned int nTcpCmdSock;		// 命令通道SOCKET
	unsigned int hTcpDataSock;		// TCP数据通道SOCKET
	unsigned int nUdpDataIP;		// UDP和多播的IP
	unsigned int nUdpDataPort;		// UDP和多播的端口
	unsigned int nClientID;			// 客户唯一的ID,	// Add the code by lvjh, 2008-03-28
	NODE_STATUS	status;				// 状态
	
	struct _SEND_NODE *pNext;
	
}SEND_NODE, *PSEND_NODE;

typedef struct _READ_POSITION
{
	int nBegin;
	int nEnd;
	BOOL bLost;
}READ_POSITION, *PREAD_POSITION;

typedef struct _CHANNEL_STREAM
{
	int nPoolSize;
	int nWritePos;

	pthread_mutex_t hReadBlockMutex;
	pthread_cond_t  hReadBlockCond;
  	pthread_mutex_t hReadWriteLock;

	NET_DATA_PACKET *pPackPool[MAX_POOL_SIZE];
}CHANNEL_STREAM, *PCHANNEL_STREAM;

typedef struct _UDP_STREAM_SEND
{
	pthread_t hUdpSendThreadID[SERVER_MAX_CHANNEL];
	SEND_NODE *pUdpSendList[SERVER_MAX_CHANNEL];
	pthread_mutex_t hUdpSendListMutex[SERVER_MAX_CHANNEL];
}UDP_STREAM_SEND, *PUDP_STREAM_SEND;

#if 0
typedef struct _CLIENT_INFO
{
	unsigned int ip;
	unsigned int port;
	char szUserName[USER_NAME_LEN];
	char szUserPsw[USER_PSW_LEN];

	unsigned int level;
	unsigned int hMsgSocket;

	unsigned int nKeepAlive;
	NODE_STATUS status;
	struct _CLIENT_INFO *pNext;
}CLIENT_INFO, *PCLIENT_INFO;
#endif

typedef struct _CLIENT_INFO
{
	unsigned int ip;
	unsigned int port;
	unsigned int nflag;
	char szUserName[USER_NAME_LEN];
	char szUserPsw[USER_PSW_LEN];

	unsigned int level;
	unsigned int hMsgSocket;
	struct sockaddr_in addr;

	unsigned int nKeepAlive;
	NODE_STATUS status;
	struct _CLIENT_INFO *pNext;
}CLIENT_INFO, *PCLIENT_INFO;



typedef struct _CLIENT_LIST
{
	unsigned int nTotalNum;
	pthread_mutex_t hClientMutex;
	CLIENT_INFO *pNext;
}CLIENT_LIST, *PCLIENT_LIST;

typedef struct _TALKTHRD_PARAM
{
	int hSock;
	unsigned int ip;
	unsigned int port;
	struct sockaddr_in addr;
	int nflag;
}TALKTHRD_PARAM;

typedef struct _RECTHRD_PARAM
{
	int hSock;
	unsigned int ip;
	unsigned int port;
	struct sockaddr_in addr;
	char buffer[UPD_RCV_BUFFER_LEN];
}RECTHRD_PARAM;

//typedef int (*CheckUserPsw)(const char *pUserName, const char *pPsw);
typedef int (*CheckUserPsw)(MSG_HEAD *pMsgHead, const char *pUserName, const char *pPsw);
typedef int (*CheckUdpUserPsw)(MSG_HEAD *pMsgHead, const char *pUserName, const char *pPsw, struct sockaddr_in addr);

typedef int (*ServerRecv)(MSG_HEAD *pMsgHead, char *pRecvBuf);
typedef int (*ClientRequestTalk)(unsigned long ip, unsigned short port);
typedef int (*ClientUdpRequestTalk)(unsigned long ip, unsigned short port,struct sockaddr_in addr);

typedef int (*ClientStreamTalk)(unsigned long ip, unsigned short port, char *pbuf, int len);
typedef int (*ClientUdpStreamTalk)(unsigned long ip, unsigned short port, char *pbuf, int len, struct sockaddr_in addr);

typedef int (*FileTransfer)(int sockFd);
typedef int (*UdpFileTransfer)(int sockFd, struct sockaddr_in addr, char *buffer);


typedef struct _SER_INFO
{
	unsigned int nChnNum;		//服务器通道数
	unsigned int nBasePort;		//服务器端口号
	char szMultiIP[16];			//多播IP
	unsigned int multiAddr;		//多播IP

	AV_INFO avInfoStream1[SERVER_MAX_CHANNEL];	//第一码流
	AV_INFO avInfoStream2[SERVER_MAX_CHANNEL];	//第二码流

	unsigned int nSendBufferNum;			//发送缓冲BUFFER尺寸

	int hTcpListenSock;						//TCP监听SOCKET
	int hUdpListenSock;						//UDP监听SOCKET

	pthread_t hTcpListenThread;				//TCP监听线程ID
	pthread_t hUdpListenThread;				//UDP监听线程ID

	SEND_NODE *pTcpSendList;				//TCP发送数据线程池
	pthread_mutex_t hTcpSendListMutex;		//TCP发送数据线程池锁

	UDP_STREAM_SEND udpStreamSend1;			//UDP发送码流1
	UDP_STREAM_SEND udpStreamSend2;			//UDP发送码流2

	CHANNEL_STREAM *pChannelStream1[SERVER_MAX_CHANNEL];	//TCP发送码流1
	CHANNEL_STREAM *pChannelStream2[SERVER_MAX_CHANNEL];	//TCP发送码流2

	char *pSendBuf;					//发送AV数据公共BUFFER
	pthread_mutex_t sendBufMutex;	//发送AV数据公共BUFFER的锁
    
	char *pRecvBuf;					//接收AV数据公共BUFFER
	char *pUpdateFileBuf;			//升级文件数据BUFFER
	int nCurUpdateSock;				//当前用户升级SOCKET
	int nUpdateFlag;				//升级标志，如果有用户在升级，则其它用户不可以再升级，直到该用户升级完毕

	pthread_key_t hReadPosKey;		//

	pthread_t msgThreadID[MAX_USER_CMD_CHANNEL_NUM];	//命令线程ID
	pthread_mutex_t msgThreadMutex;			//命令线程锁
	pthread_cond_t msgThreadCond;			//命令线程条件

	int msgProcessThreadNum;				//要处理的命令线程数
	int msgWaitThreadNum;					//等待的命令线程数
	int msgQuitThreadNum;					//要退出的命令线程数

	pthread_t msgQuitThread;				//处理命令退出的线程ID
	pthread_mutex_t msgQuitThreadMutex;		//处理命令退出线程锁
	pthread_cond_t msgQuitThreadCond;		//处理命令退出线程条件

	pthread_t dataThreadID[MAX_USER_DATA_CHANNEL_NUM];	//数据线程ID
	pthread_mutex_t dataThreadMutex;		//数据线程锁
	pthread_cond_t dataThreadCond;			//数据线程条件

	int dataProcessThreadNum;				//要处理的数据线程数
	int dataWaitThreadNum;					//等待的数据线程数
	int dataQuitThreadNum;					//要退出的数据线程数

	pthread_t dataQuitThread;				//处理数据退出的线程ID
	pthread_mutex_t dataQuitThreadMutex;	//处理数据退出线程锁	
	pthread_cond_t dataQuitThreadCond;		//处理数据退出线程条件

	CheckUserPsw funcCheckUserPsw;			//用户校验回调函数
	CheckUdpUserPsw funcUdpCheckUserPsw;    //UDP用户校验回调函数
	ServerRecv funcServerRecv;				//业务处理回调函数
	
	FileTransfer funcFileTransfer;			//文件传输
	UdpFileTransfer UdpFuncFileTransfer;	//文件传输udp
	

	int bServerStart;							//网络SDK启用标志
	int bServerExit;						//网络SDK退出标志
	int nMsgThreadCount;					//命令线程池计数
	int nDataThreadCount;					//数据线程池计数
    	
	int nAudioChannels;						//对讲音频通道数
	int nAudioBits;							//对讲音频数据位
	int nAudioSamples;						//对讲音频采样率	
	ClientRequestTalk pCallbackRequestTalk;	//对讲请求回调函
	ClientUdpRequestTalk pUdpCallbackRequestTalk;	//UDP对讲请求回调函数
	
	ClientStreamTalk pCallbackStreamTalk;	//对讲音频码流处理回调函数
	ClientUdpStreamTalk pUdpCallbackStreamTalk;	//UDP对讲音频码流处理回调函数
	
	int hTalkbackSocket;					//对讲的SOCKET
	int hUdpTalkbackSocket; 				//udp 对讲的SOCKET
	struct sockaddr_in addr ;
	int nupdate_flag;
	int nflag;
	int curReadPos;
	FILE *hFile;
	char cur_file_name[128];
	int size;
	unsigned long	dwUserID;
}NET_SER_INFO, *PNET_SER_INFO;

// 调试信息
#ifdef NET_SDK_DEBUG
	#define net_debug()\
	{             \
		printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));\
	}
#else
	#define net_debug()
#endif

// 全局变量
NET_SER_INFO g_server_info;
CLIENT_LIST g_client_list;
int g_talkbackSock;
char g_wire_ip[16];
char g_wireless_ip[16];

//网络SDK的API
int NETSDK_ServerInit(int flag);
int NETSDK_ServerOpen();
int NETSDK_ServerSetup(int nChnNum, int nPort, char *pMultiIP, int bufSize);
int NETSDK_GetServerSetup(int *nChnNum, int *nPort, char *pMultiIP, int *bufSize);
int NETSDK_ServerStart();
int NETSDK_ServerStop();
int NETSDK_ServerClose();
int NETSDK_SendMsg(MSG_HEAD	*pMsgHead, char *pSendBuf);
int Udp_NETSDK_SendMsg(MSG_HEAD	*pMsgHead, char *pSendBuf, struct sockaddr_in addr);

int NETSDK_SendAllMsg(char *pMsgBuf, int nLen);
int NETSDK_SendSpecMsg(int nUserID, char *pMsgBuf, int nLen);
int NETSDK_GetUserId(unsigned long ipAddr);
int NETSDK_SetAVInfo1(int nChannel, AV_INFO *pAVInfo);
int NETSDK_SetAVInfo2(int nChannel, AV_INFO *pAVInfo);
int NETSDK_GetAVInfo1(int nChannel, AV_INFO *pAVInfo);
int NETSDK_GetAVInfo2(int nChannel, AV_INFO *pAVInfo);
int NETSDK_WriteOneFrame1(int nChannel, char *pFrame);
int NETSDK_WriteOneFrame2(int nChannel, char *pFrame);
int NETSDK_SetSendBufNum(int nBufNum);
int NETSDK_SetUserCheckFunc(CheckUserPsw pFunc);
int NETSDK_SetUdpUserCheckFunc(CheckUdpUserPsw pFunc);
int NETSDK_SetServerRecvFunc(ServerRecv	pFunc);
int NETSDK_SetFileTransferFunc(FileTransfer pFunc);
int NETSDK_SetFileUdpTransferFunc(UdpFileTransfer pFunc);

int NETSDK_GetUserLoginNum();
int NETSDK_GetOpenChannelNum();
int NETSDK_SetBandWidth(int nBandWidth);

int open_talk(void);
int close_talk(void);
int NETSDK_SetCallback(ClientRequestTalk  fun1,ClientStreamTalk fun2);
int UDPNETSDK_SetCallback(ClientUdpRequestTalk  fun1,ClientUdpStreamTalk fun2);

int NETSDK_SetTalkParam(int nChannel, int nBits, int nSamples);
int NETSDK_Talk_Begin(char *pszIP, unsigned short port);
int NETSDK_Talk_End();
int NETSDK_Talk_Send(char *pBuf, int nLen);
int UDPNETSDK_Talk_Send(char *pBuf, int nLen);
int NETSDK_Talk_Send_Ext(char *pBuf,int nLen);
int NETSDK_Talk_Start(int socket);
int UDPNETSDK_Talk_Start(int socket, struct sockaddr_in addr);
int NETSDK_Talk_Stop();

int NETSDK_NatSetup(char *remoteIP, int remotePort, int interval, char *data, int len);
int NETSDK_NatStart();
int NETSDK_NatStop();
int NETSDK_NatPause();
int NETSDK_NatResume();

int NETSDK_SetWireIP(char *addr);
int NETSDK_SetWirelessIP(char *addr);

#endif


