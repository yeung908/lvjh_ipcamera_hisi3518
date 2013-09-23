#include "talkBack.h"
#include "../audioDecAppModule.h"
#include "../audioStream.h"
#include "../netsdk/tcpNat.h"  //mbl add
unsigned char g_talk_buffer[2048];



extern int g_aenc_module_pause_flag[4];
extern int g_adec_module_pause_flag;


//mody by lv start add--------------------------------------------
int    g_talk_break_flag = 1;


int open_talk(void)
{
	g_talk_break_flag  = 1;
	return 0;
}

int close_talk(void)
{
	g_talk_break_flag  = 0;
	return 0;
}
//mody by lv end add--------------------------------------------


int NETSDK_SetCallback(ClientRequestTalk fun1, ClientStreamTalk fun2)
{
	g_server_info.pCallbackRequestTalk = fun1;
	g_server_info.pCallbackStreamTalk = fun2;

	return 0;
}
//mody by lv start add--------------------------------------------
int UDPNETSDK_SetCallback(ClientUdpRequestTalk fun1, ClientUdpStreamTalk fun2)
{
	g_server_info.pUdpCallbackRequestTalk = fun1;
	g_server_info.pUdpCallbackStreamTalk = fun2;

	return 0;
}
//mody by lv end add--------------------------------------------

int NETSDK_SetTalkParam(int nChannel, int nBits, int nSamples)
{
    g_server_info.nAudioChannels = nChannel;
    g_server_info.nAudioBits = nBits;
    g_server_info.nAudioSamples = nSamples;

	return 0;
}

int NETSDK_Talk_End()
{
	int ret = -1;
	if (g_server_info.hTalkbackSocket > 0)
    {
		printf("NETSDK_Talk_End(%d) ...\n", g_server_info.hTalkbackSocket);
		
		shutdown(g_server_info.hTalkbackSocket, 2);
		close(g_server_info.hTalkbackSocket);
        g_server_info.hTalkbackSocket = -1;
    }
	
	return 0;
}

int NETSDK_Talk_Begin(char *pszIP, unsigned short port)
{
	int ret = -1;
	struct sockaddr_in addr;

    memset(&addr, 0, sizeof(struct sockaddr_in));
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(pszIP);
    addr.sin_port = htons(port);
    
	if (g_server_info.hTalkbackSocket)
	{
		NETSDK_Talk_End();
		return -1;
	}
    
	g_server_info.hTalkbackSocket = socket(AF_INET, SOCK_STREAM, 0);
    
	ret = connect(g_server_info.hTalkbackSocket, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0)
	{
		NETSDK_Talk_End(); 

		return -1;    
	}
	return 0;
}

int NETSDK_Talk_Start(int socket)
{
	g_server_info.hTalkbackSocket = socket;
	g_server_info.nflag = TCP_FLAG;        //mbl add
	return 0;
}
//mody by lv start add--------------------------------------------
int UDPNETSDK_Talk_Start(int socket,struct sockaddr_in addr)
{
	g_server_info.hUdpTalkbackSocket = socket;
	g_server_info.addr = addr;
	g_server_info.nflag = UDP_FLAG;
	return 0;
}
//mody by lv end add--------------------------------------------

int NETSDK_Talk_Stop()
{
	if (g_server_info.hTalkbackSocket)
    {
		printf("NETSDK_Talk_Stop(%d) ...\n", g_server_info.hTalkbackSocket);
		
		shutdown(g_server_info.hTalkbackSocket, 2);
		close(g_server_info.hTalkbackSocket);
        g_server_info.hTalkbackSocket = -1;
    }

	return 0;
}
//mody by lv start add--------------------------------------------
int UDPNETSDK_Talk_Send(char *pBuf, int nLen)
{
	int ret = -1;
	NET_HEAD netHead;

		
	if (pBuf == NULL)
	{
		return -1;
	}
	if (nLen <= 0)
	{
		return -1;
	}
	if (g_server_info.hUdpListenSock <= 0)
	{
		return -1; 
	}

	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_TALK_DATA;
	netHead.nErrorCode = 0;
	netHead.nBufSize = nLen;
	netHead.nReserve = 0;

	printf("UDPNETSDK_Talk_Send ...\n");
	memcpy(g_talk_buffer, &netHead, sizeof(NET_HEAD));
	memcpy(g_talk_buffer+sizeof(NET_HEAD), pBuf, nLen);
	ret = sendto(g_server_info.hUdpListenSock, g_talk_buffer, nLen+sizeof(NET_HEAD), 0, (struct sockaddr*)&g_server_info.addr, sizeof(struct sockaddr));
	if (ret <= 0)
	{
		net_debug();
		return -1;
	}
	#if 0
	if(g_server_info.addr.sin_port > 0){
		ret = sendto(g_server_info.hUdpListenSock, g_talk_buffer, nLen+sizeof(NET_HEAD), 0, (struct sockaddr*)&g_server_info.addr, sizeof(struct sockaddr));
		if (ret <= 0)
		{
			net_debug();
			return -1;
		}
	}
	else{
		ret = send(g_server_info.hTalkbackSocket, g_talk_buffer, nLen+sizeof(NET_HEAD), 0);
		if (ret <= 0)
		{
			net_debug();
			return -1;
		}
	
	}
	#endif 
	
	return 0;
}
//mody by lv end add--------------------------------------------

int NETSDK_Talk_Send(char *pBuf, int nLen)
{
	int ret = -1;
	NET_HEAD netHead;

	//printf("NETSDK_Talk_Send: %p %d %d\n", pBuf, nLen, g_server_info.hTalkbackSocket);

	if (pBuf == NULL)
	{
		return -1;
	}
	if (nLen <= 0)
	{
		return -1;
	}

	#if 0
	if (g_server_info.hTalkbackSocket <= 0 || g_server_info.hUdpListenSock <= 0)
	{
		return -1; 
	}
	#endif 
	

	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_TALK_DATA;
	netHead.nErrorCode = 0;
	netHead.nBufSize = nLen;
	netHead.nReserve = 0;

	memcpy(g_talk_buffer, &netHead, sizeof(NET_HEAD));
	memcpy(g_talk_buffer+sizeof(NET_HEAD), pBuf, nLen);
	
	if(g_server_info.nflag == TCP_FLAG){
		ret = send(g_server_info.hTalkbackSocket, g_talk_buffer, nLen+sizeof(NET_HEAD), 0);
		if (ret <= 0)
		{
			net_debug();
			return -1;
		}
	}
	else if(g_server_info.nflag == UDP_FLAG){
	ret = sendto(g_server_info.hUdpListenSock, g_talk_buffer, nLen+sizeof(NET_HEAD), 0, (struct sockaddr*)&g_server_info.addr, sizeof(struct sockaddr));
	if (ret <= 0)
	{
		net_debug();
		return -1;
	}
	}
	//mody by lv end add--------------------------------------------
	return 0;
}

int NETSDK_Talk_Send_Ext(char *pBuf,int nLen)
{
	if (pBuf == NULL)
	{
		return -1;
	}
	if (nLen <= 0)
	{
		return -1;
	}
	if (g_talkbackSock <= 0)
	{
		return -1; 
	}

	return send(g_talkbackSock, pBuf, nLen, 0);
}

/*
int TalkbackThread(void *par)
{
    TALKTHRD_PARAM thdpar;
    fd_set fset;
    int ret = -1;
    struct timeval to;
    char acktest = '0';
    char pBuf[NETCMD_TALKBACK_SIZE];

    pthread_detach(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

    memcpy(&thdpar,par,sizeof(thdpar));
    FD_ZERO(&fset);
    memset(&to, 0, sizeof(to));

    while (!g_server_info.bServerExit)
    {
		to.tv_sec = SOCK_TIME_OUT;
		to.tv_usec = 0;

		FD_SET(thdpar.hSock,&fset);
		ret = select(thdpar.hSock+1,&fset,NULL,NULL,&to);
		if (g_server_info.bServerExit)
        {
			break;
        }
		if (ret == 0)
		{
			ret = send(thdpar.hSock, &acktest, 1, 0);
			if (ret <= 0)
			{
				break;
			}
                    
			continue;
		}
		if (ret == -1)
		{
			break;
		}

		if (!FD_ISSET(thdpar.hSock, &fset))
		{
			continue;
		}

		ret = recv(thdpar.hSock,pBuf,NETCMD_TALKBACK_SIZE,0); 
		if (ret < 0)
		{
			if(ECONNRESET == errno)
			{
				break;
			}
			continue;
		}
		else if(ret == 0) 
		{
			break;
		}
       	
       	//check command
		g_server_info.pCallbackStreamTalk(thdpar.ip, thdpar.port, pBuf, ret);
    }

	return 0;
}
*/


//mody by lv start add--------------------------------------------
int UdpTalkbackThread(void *par)
{
    TALKTHRD_PARAM thdpar;
    fd_set fset;
    int ret = -1;
    struct timeval to;
    char acktest = '0';
    char pBuf[NETCMD_TALKBACK_SIZE];
	NET_HEAD netHead;
	MSG_HEAD msgHead;
	int nCount = 0;			// Add the code by lvjh, 2009-09-23

	//struct sockaddr_in addr;
	int nLen = 0;

	extern AUDIO_STREAM_BUFFER *g_audio_dec_stream;

    pthread_detach(pthread_self());
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    memcpy(&thdpar, par, sizeof(thdpar));
    FD_ZERO(&fset);
    memset(&to, 0, sizeof(to));

    while ((!g_server_info.bServerExit)&&g_talk_break_flag)
    {
    	nCount++; // Add the code by lvjh, 2009-09-23
    	
		to.tv_sec = SOCK_TIME_OUT;
		to.tv_usec = 0;

		FD_SET(thdpar.hSock, &fset);
		ret = select(thdpar.hSock+1, &fset, NULL, NULL, &to);
		if (g_server_info.bServerExit)
        {
			break;
        }
        if (ret == -1)
		{
			break;
		}
		if (ret == 0)
		{
			/*
			ret = send(thdpar.hSock, &acktest, 1, 0);
			if (ret <= 0)
			{
				break;
			}
			*/

	#if 1		
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			netHead.nReserve = g_server_info.dwUserID;
			printf("talkBack udp send keep alive '''' = %d\n", g_server_info.dwUserID);
			ret = udpSendMsg(thdpar.hSock, &netHead, sizeof(NET_HEAD), thdpar.addr);
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", thdpar.hSock);
				break;
			}
	#endif	
	
		
		}

		if (!FD_ISSET(thdpar.hSock, &fset))
		{
			continue;
		}
		nLen = sizeof(struct sockaddr_in);
		ret = recvfrom(thdpar.hSock, pBuf, NETCMD_TALKBACK_SIZE, MSG_WAITALL, (struct sockaddr*)&thdpar.addr, (socklen_t *)&nLen);
		if (ret > 0)
		{
			memcpy(&netHead, pBuf, sizeof(NET_HEAD));
			printf("g_server_info.dwUserID = %d netHead.nCommand = %x\n\n", g_server_info.dwUserID, netHead.nCommand);
		#if 1
		if(netHead.nCommand == NETCMD_REC_FILE ||netHead.nCommand == NETCMD_OPEN_TALK ||netHead.nCommand ==NETCMD_LOGON ||netHead.nCommand ==NETCMD_OPEN_CHANNEL ||netHead.nCommand ==NETCMD_P2P_CMD ||netHead.nCommand == NETCMD_CLOSE_TALK)break;
		else if (netHead.nCommand == NETCMD_KEEP_ALIVE){
			printf("CMD: NETCMD_KEEP_ALIVE\n");
			printf("P2P SERVER SEND dwUserID = %d\n", netHead.nReserve);
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			if(netHead.nReserve == g_server_info.dwUserID){
				printf("**************************************************\n");
				TcpSendMsgToAll_UDPremote(REMOTE_IP, REMOTE_PORT, NETCMD_NAT_CONNECT);
				printf("		TcpSendMsgToAll_UDPremote success ! \n");
				printf("**************************************************\n");
			}
		}
		else {
			msgHead.nSock = thdpar.ip;
			msgHead.addr = thdpar.addr;
			msgHead.nCmd = netHead.nCommand;
			msgHead.nRight = 3;
			msgHead.nErrorCode = 0;
			msgHead.nBufSize = netHead.nBufSize;
			msgHead.nflag  = UDP_FLAG;
			g_server_info.nupdate_flag = UDP_FLAG;
			
			ParseCommand(&msgHead, pBuf+sizeof(NET_HEAD));
		}
		#endif			
			ret = g_server_info.pCallbackStreamTalk(thdpar.ip, thdpar.port, pBuf+sizeof(NET_HEAD), (ret-sizeof(NET_HEAD)));
			
		}
    }

#if 1
	printf("Talk End (BY lvjh)(%d)!\n", thdpar.hSock);

	/******************add code by liujw 12-3-5***************************/
	// 关闭AOUT  设备
	ret =audioDecStop(TALK_CHANNEL);
	if (ret)
	{
		printf("audioDecStop Failed! 0x%x \n", ret);
	   // return -1;
	}
	else
	printf("audioDecStop ok!\n");
	
	audioDecClose(TALK_CHANNEL);
	if (ret)
	{
		printf("audioDecClose Failed! 0x%x \n", ret);
	   // return -1;
	}
	else
	printf("audioDecClose ok!\n");

	audioOutStop(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutStop Failed! 0x%x \n", ret);
	    //return -1;
	}
	else
	printf("audioOutStop ok!\n");
	
	ret = audioOutClose(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutClose(%s %d) Failed!\n", __FILE__, __LINE__);
	   // return -1;
	}
	else
	printf("audioOutClose ok!\n");

	audioDecModuleSemInit();              //zhangjing 2013-05-30
	/*delete by zhangjing 2013-06-09 static open
	if (g_audio_dec_stream)
	{
		audioBufferClose(g_audio_dec_stream);
	}*/
#endif
	//HI_MPI_ADEC_DestroyChn(0);
	//g_aenc_module_pause_flag[0] = 1;
	

	//close(thdpar.hSock);
	//g_server_info.hUdpTalkbackSocket = -1;
	return 0;
}
//mody by lv end add--------------------------------------------


int TalkbackThread(void *par)
{
	TALKTHRD_PARAM thdpar;
	fd_set fset;
	int ret = -1;
	struct timeval to;
	char acktest = '0';
	char pBuf[NETCMD_TALKBACK_SIZE];
	NET_HEAD netHead;
	int nCount = 0;			// Add the code by lvjh, 2009-09-23

	extern AUDIO_STREAM_BUFFER *g_audio_dec_stream;

	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	memcpy(&thdpar, par, sizeof(thdpar));
	FD_ZERO(&fset);
	memset(&to, 0, sizeof(to));

	printf("Talk Socket: %d\n", thdpar.hSock);

	while (!g_server_info.bServerExit)
	{
			
		to.tv_sec = SOCK_TIME_OUT;
		to.tv_usec = 0;

		FD_SET(thdpar.hSock, &fset);
		ret = select(thdpar.hSock+1, &fset, NULL, NULL, &to);
		if (g_server_info.bServerExit)
		{
			printf("ZJ_TALK_ServerExit\n");
			break;
		}
		if (ret == -1)
		{
			printf("ZJ--select talk socket failed\n");
			break;
		}
		if (ret == 0)
		{
			/*
			ret = send(thdpar.hSock, &acktest, 1, 0);
			if (ret <= 0)
			{
				break;
			}
			*/
			//printf("Talk Time Out!\n");
			nCount++; // Add the code by lvjh, 2009-09-23
			printf("ZJ--select talk socket over time\n");
			
			//continue;	// add by lvjh 2012-11-16
			
			// Add the code by lvjh, 2009-09-23
			if (nCount > 10)
			{
				break;
			}
			else
			{
				continue;
			}
		}

		if (!FD_ISSET(thdpar.hSock, &fset))
		{
			continue;
		}

		ret = recv(thdpar.hSock, &netHead, sizeof(NET_HEAD), 0); 
		if (ret < 0)
		{
			if(ECONNRESET == errno)
			{
				printf("ZJ-tcp socket connect abort\n");
				break;
			}
			continue;
		}
		else if(ret == 0) 
		{
			break;
		}

		ret = recv(thdpar.hSock, pBuf, netHead.nBufSize, 0);
		if (ret > 0)
		{
			ret = g_server_info.pCallbackStreamTalk(thdpar.ip, thdpar.port, pBuf, ret);
			//printf("pCallbackStreamTalk: %d\n", ret);
		}
	}

	printf("Talk End (BY lvjh)(%d)!\n", thdpar.hSock);

	/******************add code by liujw 12-3-5***************************/
	// 关闭AOUT  设备
	#if 1
	ret =audioDecStop(TALK_CHANNEL);
	if (ret)
	{
		printf("audioDecStop Failed! 0x%x \n", ret);
	   // return -1;
	}
	else
	printf("audioDecStop ok!\n");
	
	audioDecClose(TALK_CHANNEL);
	if (ret)
	{
		printf("audioDecClose Failed! 0x%x \n", ret);
	   // return -1;
	}
	else
	printf("audioDecClose ok!\n");

	audioOutStop(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutStop Failed! 0x%x \n", ret);
	    //return -1;
	}
	else
	printf("audioOutStop ok!\n");
	
	ret = audioOutClose(TALK_CHANNEL);
	if (ret)
	{
		printf("audioOutClose(%s %d) Failed!\n", __FILE__, __LINE__);
	   // return -1;
	}
	else
		printf("audioOutClose ok!\n");
	close(thdpar.hSock);
	g_server_info.hTalkbackSocket = -1;
	audioDecModuleSemInit();              //zhangjing 2013-05-30
	/*delete by zhangjing 2013-06-09 static open
	if (g_audio_dec_stream)
	{
		audioBufferClose(g_audio_dec_stream);
	}*/
	//HI_MPI_ADEC_DestroyChn(0);       //mbl old:add
	//g_aenc_module_pause_flag[0] = 1;
	//g_adec_module_pause_flag = 1;  //mbl old:worked

	//shutdown(thdpar.hSock, 2);      //mbl old:worked

	#endif
	return 0;
}

