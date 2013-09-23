#include "sendList.h"

int lan_ip(unsigned int nIP)
{
	// filter LAN IP's
	// -------------------------------------------
	// 0.*
	// 10.0.0.0 - 10.255.255.255  class A
	// 172.16.0.0 - 172.31.255.255  class B
	// 192.168.0.0 - 192.168.255.255 class C
	
	unsigned char nFirst = (unsigned char)nIP;
	unsigned char nSecond = (unsigned char)(nIP >> 8);
	
	if (nFirst==192 && nSecond==168) // check this 1st, because those LANs IPs are mostly spreaded
		return 1;
	
	if (nFirst==172 && nSecond>=16 && nSecond<=31)
		return 1;
	
	if (nFirst==0 || nFirst==10)
		return 1;
	
	return 0; 
}

//#define FULL_PACKET		1

int PrintMemory(char *buffer, int size)
{
	int i = 0;

	if (buffer==NULL || size<=0)
	{
		return -1;
	}

	printf("Memory Start: %p\n", buffer);
	for (i=0; i<size; i++)
	{
		printf("%02x ", buffer[i]);
	}
	printf("\n");
	printf("\nMemory End: %p\n", buffer+size);

	return 0;
}

// TCP数据发送线程
int TcpDataSendThread()
{
	CHANNEL_STREAM *pStream = NULL;
	SEND_NODE *pNode = NULL;
	READ_POSITION *pReadPos = NULL;

	NET_DATA_PACKET netDataPacket;
	BOOL bRet = 0;
	BOOL bFirstRead = TRUE;
	int ret = 0;
	int loop_send = 0;
	int send_sleep = 0;
	int nLen = 0;

#ifdef FULL_PACKET
	int nPos = 0;
	char *av_buffer = NULL;
	NET_DATA_PACKET *pNetDataPacket = NULL;
	NET_DATA_HEAD netDataHead;
#endif

	int nLanFlag = 1;

	struct timeval tv;
	unsigned long long nStartTimeCount = 0;
	unsigned long long nCurTimeCount = 0;
	unsigned long nCount = 0;
   
	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	pReadPos = (READ_POSITION *)malloc(sizeof(READ_POSITION));
	if (pReadPos == NULL)
	{
		return -1;
	}
	memset(pReadPos, 0, sizeof(READ_POSITION));
	pthread_setspecific(g_server_info.hReadPosKey, pReadPos);

#ifdef FULL_PACKET
	av_buffer = (char *)malloc(5000);
	if (av_buffer == NULL)
	{
		return -1;
	}
	memset(av_buffer, 0, 5000);

	pNetDataPacket = av_buffer;
	
	//printf("av_buffer: %p\n", av_buffer);
#endif

	//send_sleep = get_sleep_time();
	//printf("SLEEP: %d\n", send_sleep);

	while (!g_server_info.bServerExit)
	{
		// 等待用户连接数据通道
		pthread_mutex_lock(&g_server_info.dataThreadMutex);
		
		while (g_server_info.dataWaitThreadNum == 0) //
		{
			pthread_cond_wait(&g_server_info.dataThreadCond, &g_server_info.dataThreadMutex);
			if (g_server_info.bServerExit)
			{
				break;
			}
		}

		if (g_server_info.bServerExit)
		{
			pthread_mutex_unlock(&g_server_info.dataThreadMutex);
			break;
		}

		// 查找等待队列中的用户
		pNode = FindWaitProcessNode(READY);
		if (NULL == pNode)
		{
			pthread_mutex_unlock(&g_server_info.dataThreadMutex);
			continue;
		}
        
		pNode->status = RUN;
		g_server_info.dataProcessThreadNum ++;        
		g_server_info.dataWaitThreadNum --;
		if (g_server_info.dataWaitThreadNum < 0)
		{
			g_server_info.dataWaitThreadNum = 0;
		}

		pthread_mutex_unlock(&g_server_info.dataThreadMutex);

		if (pNode->nSerChn < 0 || pNode->nSerChn>=SERVER_MAX_CHANNEL)	// Add by zhb, 2007-11-06
		{
			continue;
		}
		if (0 == pNode->nStreamType)
		{
			pStream = g_server_info.pChannelStream1[pNode->nSerChn];
		}
		else
		{
			pStream = g_server_info.pChannelStream2[pNode->nSerChn];
		}
		if (pStream == NULL)	// Add by zhb, 2007-10-27
		{
			continue;
		}

		pReadPos->nBegin = 0;
		pReadPos->nEnd = 1;
		pReadPos->bLost = FALSE;

		bFirstRead = TRUE;

		nLanFlag = lan_ip(pNode->nTcpAddr);
		//printf("Lan: %d %x\n", nLanFlag, pNode->nTcpAddr);

		// Add the code by lvjh, 2008-02-28
		//gettimeofday (&tv, NULL);
		//nStartTimeCount = tv.tv_sec*1000000+tv.tv_usec;
		
#ifdef FULL_PACKET
		nPos = 0;
#endif

		// 处理用户的数据传输
		while (STOP != pNode->status)
		{
			bRet = TRUE;

			pthread_mutex_lock(&pStream->hReadBlockMutex);
			while ((pReadPos->nBegin == pReadPos->nEnd) && (pReadPos->nBegin == pStream->nWritePos))
			{
				pthread_cond_wait(&pStream->hReadBlockCond, &pStream->hReadBlockMutex);
			}            
			pthread_mutex_unlock(&pStream->hReadBlockMutex);

			if (g_server_info.bServerExit)
			{
				break;
			}
			if (STOP == pNode->status)
			{
				break;
			}

			// Add the code by lvjh, 2008-03-22
			//nStartTimeCount = nCurTimeCount;
			
			// 从BUFFER里提取数据包
			//bRet = GetOnePacket(pStream, &netDataPacket, pReadPos, bFirstRead);		

#ifdef FULL_PACKET
			//printf("nPos: %d\n", nPos);
			pNetDataPacket = (NET_DATA_PACKET *)(av_buffer+nPos);
			bRet = GetOnePacket(pStream, pNetDataPacket, pReadPos, bFirstRead);

			//PrintMemory(pNetDataPacket, 32);
#else
			bRet = GetOnePacket(pStream, &netDataPacket, pReadPos, bFirstRead);	
#endif

			bFirstRead = FALSE;
            
			//printf("1111111111111111111111111111111\n");

			if (STOP == pNode->status)
			{
				break;
			}
			if (!bRet)
			{
				printf("GetOnePacket(%d): Failed!\n", pNode->hTcpDataSock);
				//usleep(1000);	// Add the code by lvjh, 2008-03-27
				sleep(1);
				continue;
			}
		
#ifdef FULL_PACKET	
			// Add the code by lvjh, 2008-02-27
			//nPos = nPos+sizeof(NET_DATA_HEAD)+pNetDataPacket->packHead.nSize;

			memcpy(&netDataHead, pNetDataPacket, sizeof(NET_DATA_HEAD));
			nPos = nPos+sizeof(NET_DATA_HEAD)+netDataHead.nSize;
			//printf("Packet(%p): %x %x\n", pNetDataPacket, netDataHead.nSize, nPos);

			if (nPos < sizeof(NET_DATA_PACKET))
			{
				continue;
			}
#endif
			
			// Add the code by lvjh, 2008-04-05
			nLen = netDataPacket.packHead.nSize+sizeof(netDataPacket.packHead);
			if (nLen > sizeof(NET_DATA_PACKET))
			{
				printf("TCP_DATA_ERROR: data packet too large(%d)!\n", nLen);
				continue;
			}
			
			/*
			// 发送数据包
			for (loop_send=0; loop_send<1; loop_send++)
			{
#ifdef FULL_PACKET
				ret = send(pNode->hTcpDataSock, av_buffer, sizeof(NET_DATA_PACKET), 0);
#else
				ret = send(pNode->hTcpDataSock, &netDataPacket, netDataPacket.packHead.nSize+sizeof(netDataPacket.packHead), 0);
#endif
		
				// Add the code by lvjh, 2008-04-05
				if (ret < 0)
				{
					printf("TCP_DATA_ERROR(%d): send error(%s)!\n", pNode->hTcpDataSock, strerror(errno));
					goto out;
				}
				if (ret < netDataPacket.packHead.nSize+sizeof(netDataPacket.packHead))
				{
					printf("TCP_DATA_ERROR(%d): send (%d %d)!\n", pNode->hTcpDataSock, ret, netDataPacket.packHead.nSize+sizeof(netDataPacket.packHead));
				}
			}
			*/

			
			// Add the code by lvjh, 2008-05-23
			nLen = 0;
			for (loop_send=0; loop_send<10; loop_send++)
			{
				int nSendLen = 0;

#ifdef FULL_PACKET
				ret = send(pNode->hTcpDataSock, av_buffer, sizeof(NET_DATA_PACKET), 0);
#else
				nSendLen = netDataPacket.packHead.nSize+sizeof(netDataPacket.packHead)-nLen;
				ret = send(pNode->hTcpDataSock, &netDataPacket+nLen, nSendLen, 0);
				
				//printf("send: socket: %d, len: %d\n", pNode->hTcpDataSock, nSendLen);
#endif
				//printf("22222222222222222222\n");
				// Add the code by lvjh, 2008-04-05
				if (ret < 0)
				{
					printf("TCP_DATA_ERROR(%d): %d send error(%s)!\n", pNode->hTcpDataSock, nSendLen, strerror(errno));
					goto out;
				}
				if (ret < nSendLen)
				{
					printf("TCP_DATA_ERROR(%d): send (%d %d)!\n", pNode->hTcpDataSock, ret, nSendLen);
					nLen += ret;
				}
				else
				{
					break;
				}
			}
			
			
#ifdef FULL_PACKET
			nPos = nPos - sizeof(NET_DATA_PACKET);
			if (nPos > 0)
			{
				memcpy(av_buffer, av_buffer+sizeof(NET_DATA_PACKET), nPos);
			}
#endif	
			/*
			gettimeofday(&tv, NULL);
			nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
			nCount = nCurTimeCount-nStartTimeCount;
			printf("nCount: %d *** %d\n", nCount, send_sleep);

			if (nCount < send_sleep)
			{
				//usleep(send_sleep-nCount);
			}
			//printf("nCount: %d *** %d\n", nCount, g_send_sleep);
			*/
		}

out:
		printf("TCP_DATA_ERROR: EXIT(%d)\n", pNode->hTcpDataSock);

		pNode->status = STOP;

		pthread_mutex_lock(&g_server_info.dataQuitThreadMutex);
		g_server_info.dataQuitThreadNum ++;
		g_server_info.dataProcessThreadNum --;
        
		if (g_server_info.dataProcessThreadNum < 0)
		{
			g_server_info.dataProcessThreadNum = 0;
		}
		pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
        
	//	pthread_cond_signal(&g_server_info.dataQuitThreadCond);
	}
	
	g_server_info.nDataThreadCount--;
	
	printf("TcpDataSendThread(%d): Exit!\n", g_server_info.nDataThreadCount);

	return 0;
}

// 处理TCP连接的用户退出
int TcpDataQuitThread()
{
	SEND_NODE *pFind = NULL;
    
	pthread_detach(pthread_self());

	while (!g_server_info.bServerExit)
	{
		// 等待退出用户发送的信号
		pthread_mutex_lock(&g_server_info.dataQuitThreadMutex);
		while (g_server_info.dataQuitThreadNum == 0)
		{
			pthread_cond_wait(&g_server_info.dataQuitThreadCond, &g_server_info.dataQuitThreadMutex);
		}

		if (g_server_info.bServerExit)
		{
			pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
			break;
		}
        
		// 查找要退出的用户
		while ((pFind = FindWaitProcessNode(STOP)) != NULL)
		{
			FreeTcpSendNode(pFind);
		}

		g_server_info.dataQuitThreadNum = 0;
		pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
	}

	return 0;
}

// 请求TCP数据
int RequestTcpPlay(OPEN_CHANNEL openHead, int hDataSock, int nDataIP)
{
	int ret = -1;
	SEND_NODE *pNew = NULL;
	
	if (hDataSock < 0)
	{
		return -1;
	}
             
	pNew = (SEND_NODE *)malloc(sizeof(SEND_NODE));
	
	if (pNew != NULL)
	{
		memset(pNew, 0, sizeof(SEND_NODE));
        
		pNew->status = READY;
		pNew->pNext = NULL;
		pNew->nTcpAddr = nDataIP;		// Add the code by lvjh, 2008-02-28
		pNew->hTcpDataSock = hDataSock;
        
		pNew->nType = PROTOCOL_TCP;
		pNew->nSerChn = openHead.nSerChn;
		pNew->nStreamType = openHead.nStreamType;
		pNew->nTcpCmdSock = openHead.nID;
      
		pNew->nUdpDataIP = 0;		//udp set,not use in tcp
		pNew->nUdpDataPort = 0;		//udp set,not use in tcp

		ret = InsertPlayNode(g_server_info.pTcpSendList, g_server_info.hTcpSendListMutex, pNew);
		if (ret < 0)
		{
			return -1;
		}
		
		return 0;
	}
	else
	{
		return -1;
	}
}

// 插入TCP数据请求队列
int InsertPlayNode(SEND_NODE *pNodeHead, pthread_mutex_t hMutex, SEND_NODE *pNew)
{
	if (NULL == pNodeHead)
	{
		return -1;
	}
	if (NULL == pNew)
	{
		return -1;
	}
   
	pthread_mutex_lock(&hMutex);
    
	while (pNodeHead->pNext)
	{
		pNodeHead = pNodeHead->pNext;
	}
        
	pNodeHead->pNext = pNew;
    
	pthread_mutex_unlock(&hMutex);
	
	return 0;
}

// 在等待队列中查找
SEND_NODE *FindWaitProcessNode(NODE_STATUS status)
{
	SEND_NODE *pHead = g_server_info.pTcpSendList->pNext;
    
	pthread_mutex_lock(&g_server_info.hTcpSendListMutex);
    
	while (pHead)
	{
		if (status == pHead->status)
		{
			pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);
			return pHead;
		}
		pHead = pHead->pNext;
	}
	pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);

	return NULL;
}


SEND_NODE *GetWaitProcessHeadNode()
{
	return g_server_info.pTcpSendList->pNext;
}

// 释放TCP数据传输的节点
int FreeTcpSendNode(SEND_NODE *pNode)
{
	SEND_NODE *pPre = g_server_info.pTcpSendList;
	SEND_NODE *pCur = NULL;
    
	if (NULL == pPre)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_server_info.hTcpSendListMutex);
    
	pCur = pPre->pNext;
    
	while (pCur)
	{
		if (pCur == pNode)
		{
			printf("Close TCP Data Socket: %d\n", pNode->hTcpDataSock);

			shutdown(pNode->hTcpDataSock, 2);
			close(pNode->hTcpDataSock);
			pNode->hTcpDataSock = -1;
			pPre->pNext = pNode->pNext;
			free(pNode);
			pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);
          
			return 0;
		}
        
		pPre = pCur;
		pCur = pCur->pNext;
	}
    
	pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);

	return -1;
}

// 停止TCP数据传输的节点
int StopTcpNode(int id)
{
	int nNum = 0;
	SEND_NODE *pNode = g_server_info.pTcpSendList->pNext;
    
	pthread_mutex_lock(&g_server_info.hTcpSendListMutex);
    
	while (pNode)
	{
		if (pNode->nTcpCmdSock == id)
		{
			pNode->status = STOP;
			nNum ++;
		}
        
		pNode = pNode->pNext;
	}
    
	pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);

	return nNum;
}

// 停止所有的TCP数据传输的节点
int StopAllTcpNode()
{
    int nNum = 0;
    SEND_NODE *pNode = g_server_info.pTcpSendList;

    pthread_mutex_lock(&g_server_info.hTcpSendListMutex);

    if (pNode)
    {
        pNode = pNode->pNext;
	}
        
    while (pNode)
    {
        pNode->status = STOP;
        nNum ++;

        pNode = pNode->pNext;
    }
    
    pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);

    return nNum;
}

// 释放所有的TCP数据传输的节点
int FreeAllTcpNode()
{
    int nFreeNum = 0;
    SEND_NODE *pTmp = NULL;

    pthread_mutex_lock(&g_server_info.hTcpSendListMutex);

    while (g_server_info.pTcpSendList)
    {
        pTmp = g_server_info.pTcpSendList;
        g_server_info.pTcpSendList = pTmp->pNext;
        free(pTmp);
        nFreeNum ++;
    }

    pthread_mutex_unlock(&g_server_info.hTcpSendListMutex);
    
    return nFreeNum;
}

// UDP数据发送线程
int UdpSendThread(void *pPar)
{
	unsigned long lPar = 0;
	int channelNo = 0;
	int streamNo = 0;
	int opt;
	int ret;
	BOOL bRet;
	BOOL bMultiSend = FALSE;
	BOOL bFirstRead = TRUE;
	int hTcpDataSock = -1;

	int send_sample_size = 0;

	SEND_NODE *pHeadNode = NULL;
	SEND_NODE *pSendNode = NULL;

	pthread_mutex_t hNodeMutex;
    
	READ_POSITION	*pReadPos = NULL;
	CHANNEL_STREAM	*pStream = NULL;
	NET_DATA_PACKET  netDataPack;

	struct sockaddr_in addr;

	// Add the code by lvjh, 2008-02-29
	struct timeval tv;
	unsigned long long nStartTimeCount = 0;
	unsigned long long nCurTimeCount = 0;
	unsigned long nCount = 0;

	OPEN_CHANNEL closeHead;

	lPar = (unsigned long *)pPar;

	channelNo = LowWord(lPar);
	streamNo = HeightWord(lPar);

	pthread_detach(pthread_self());
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

	bzero(&addr,sizeof(addr));
	bzero(&netDataPack,sizeof(NET_DATA_PACKET));

	// 判断是哪一种码流
	addr.sin_family = AF_INET; 
	if (0 == streamNo)
	{
		pHeadNode = g_server_info.udpStreamSend1.pUdpSendList[channelNo];
		hNodeMutex = g_server_info.udpStreamSend1.hUdpSendListMutex[channelNo];
	}
	else
	{
		pHeadNode = g_server_info.udpStreamSend2.pUdpSendList[channelNo];
		hNodeMutex = g_server_info.udpStreamSend2.hUdpSendListMutex[channelNo];
	}
    
	hTcpDataSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == hTcpDataSock)
	{
		net_debug();
		return -1;
	}
    
	opt = SOCK_SNDRCV_LEN;
	setsockopt(hTcpDataSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
    
	if (0 == streamNo)
	{
		pStream = g_server_info.pChannelStream1[channelNo];
	}
	else
	{
		pStream = g_server_info.pChannelStream2[channelNo];
	}

	pReadPos = (READ_POSITION *)malloc(sizeof(READ_POSITION));
	bzero(pReadPos,sizeof(READ_POSITION));
	pthread_setspecific(g_server_info.hReadPosKey,pReadPos);
	pReadPos->nBegin = 0;
	pReadPos->nEnd = 1;
	pReadPos->bLost = FALSE;

	// Add the code by lvjh, 2008-02-29
	gettimeofday (&tv, NULL);

	while (!g_server_info.bServerExit)
	{
		// 等待用户连接数据通道
		pthread_mutex_lock(&pStream->hReadBlockMutex);

		while ((pReadPos->nBegin == pReadPos->nEnd) && (pReadPos->nBegin == pStream->nWritePos))
		{
			pthread_cond_wait(&pStream->hReadBlockCond,&pStream->hReadBlockMutex);
		}

		pthread_mutex_unlock(&pStream->hReadBlockMutex);

		if (g_server_info.bServerExit)
		{
			break;
		}
		
		if (0 == GetUdpRunNode(channelNo,streamNo))
		{
			usleep(100000);
			pReadPos->nBegin = pStream->nWritePos;
			pReadPos->nEnd = pStream->nWritePos;
			bFirstRead = TRUE;

			continue;
		}
        
		bMultiSend = FALSE;

		// 从码流BUFFER取数据包
		bRet = GetOnePacket(pStream, &netDataPack,pReadPos, bFirstRead);
		if (!bRet)
		{
			net_debug();
			usleep(40000);	// Add the code by lvjh, 2008-03-27
			printf("GetOnePacket(UDP): %d\n", bRet);
			continue;
		}

		bFirstRead = FALSE;

		// 开始处理码流传输
		pthread_mutex_lock(&hNodeMutex);
		pSendNode = pHeadNode->pNext;
        
		while (pSendNode)
		{
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = pSendNode->nUdpDataIP;
			addr.sin_port = pSendNode->nUdpDataPort;

			if (pSendNode->nType == PROTOCOL_UDP)							// UDP传输
			{
			//	printf("g_server_info.hUdpListenSock  = %d :%s : %d\n",g_server_info.hUdpListenSock, pSendNode->nUdpDataIP, pSendNode->nUdpDataPort); 
				ret = sendto(g_server_info.hUdpListenSock, &netDataPack, netDataPack.packHead.nSize+sizeof(netDataPack.packHead), 0, (struct sockaddr *)&addr, sizeof(addr));
			//		ret = sendto(hTcpDataSock, &netDataPack, netDataPack.packHead.nSize+sizeof(netDataPack.packHead), 0, (struct sockaddr *)&addr, sizeof(addr));
			}
			else if (!bMultiSend &&(pSendNode->nType == PROTOCOL_MULTI))	// MULTI传输
			{
				ret = sendto(hTcpDataSock, &netDataPack, netDataPack.packHead.nSize+sizeof(netDataPack.packHead), 0, (struct sockaddr *)&addr, sizeof(addr));
			
				/*
				// Add the code by lvjh, 2008-05-12
				opt = inet_addr(g_wire_ip);
				ret = setsockopt(hTcpDataSock, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
				ret = sendto(hTcpDataSock, &netDataPack, netDataPack.packHead.nSize+sizeof(netDataPack.packHead), 0, (struct sockaddr *)&addr, sizeof(addr));
				
				opt = inet_addr(g_wireless_ip);
				ret = setsockopt(hTcpDataSock, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
				ret = sendto(hTcpDataSock, &netDataPack, netDataPack.packHead.nSize+sizeof(netDataPack.packHead), 0, (struct sockaddr *)&addr, sizeof(addr));
				*/
				bMultiSend = TRUE;
			}
			if (ret < 1)
			{
				net_debug();
			}   
             
			pSendNode = pSendNode->pNext;
		}
		pthread_mutex_unlock(&hNodeMutex);

		//usleep(10000);

		/*
		// Add the code by lvjh, 2008-02-29
		gettimeofday (&tv, NULL);
		nCurTimeCount = tv.tv_sec*1000000+tv.tv_usec;
		nCount = nCurTimeCount-nStartTimeCount;
		if (nCount < g_send_sleep)
		{
			usleep(g_send_sleep-nCount);
		}
		*/

	}
    
	return 0;
}

// UDP数据传输请求
int RequestUdpPlay(OPEN_CHANNEL openHead, unsigned long ip, unsigned long port)
{
	int i = 0;
	SEND_NODE *pHeadNode = NULL;
	SEND_NODE *pSendNode = NULL;
	SEND_NODE *pNew = NULL;

	pNew = (SEND_NODE *)malloc(sizeof(SEND_NODE));
	bzero(pNew,sizeof(SEND_NODE));

	pNew->status = RUN;
	pNew->pNext = NULL;
	pNew->hTcpDataSock = 0;

	pNew->nType = PROTOCOL_UDP;
	pNew->nSerChn = openHead.nSerChn;
	pNew->nStreamType = openHead.nStreamType;
	pNew->nTcpCmdSock = openHead.nID;

	pNew->nUdpDataIP = ip;
	pNew->nUdpDataPort = port;

	pNew->nClientID = openHead.nClientID;	// Add the code by lvjh, 2008-03-28
    
	for (i=0; i<g_server_info.nChnNum; i++)
	{
		pHeadNode = g_server_info.udpStreamSend1.pUdpSendList[i];
		pSendNode = pHeadNode->pNext;
       
		while (pSendNode)
		{
			if ((pSendNode->nUdpDataIP == pNew->nUdpDataIP) 
				&& (pSendNode->nUdpDataPort == pNew->nUdpDataPort) 
				&& (pSendNode->nType == pNew->nType) 
				&& (pSendNode->nSerChn == pNew->nSerChn))
			{
				return -1;
			}
			
			pSendNode = pSendNode->pNext;
		}
	}

	if (0 == openHead.nStreamType)
	{
		InsertPlayNode(g_server_info.udpStreamSend1.pUdpSendList[openHead.nSerChn],
					g_server_info.udpStreamSend1.hUdpSendListMutex[openHead.nSerChn], pNew);
	}
	else
	{
		InsertPlayNode(g_server_info.udpStreamSend2.pUdpSendList[openHead.nSerChn],
					g_server_info.udpStreamSend2.hUdpSendListMutex[openHead.nSerChn], pNew);
	}

	return 0;
}

// 退出UDP数据传输
BOOL ExistUdpUser(OPEN_CHANNEL openHead, unsigned long ip, unsigned long port)
{
	SEND_NODE *pNode = NULL;
	pthread_mutex_t hMutex;
    
	if (0 == openHead.nStreamType)
	{
		pNode = g_server_info.udpStreamSend1.pUdpSendList[openHead.nSerChn];
		hMutex = g_server_info.udpStreamSend1.hUdpSendListMutex[openHead.nSerChn];
	}
	else
	{
		pNode = g_server_info.udpStreamSend2.pUdpSendList[openHead.nSerChn];
		hMutex = g_server_info.udpStreamSend2.hUdpSendListMutex[openHead.nSerChn];
	}
    
	//net_debug();
    
	if (NULL == pNode)
	{
		return FALSE;
	}
        
	pthread_mutex_lock(&hMutex);
	pNode = pNode->pNext;
    
	while (pNode)
	{
		if ((pNode->nTcpCmdSock == openHead.nID) && (pNode->nUdpDataIP == ip) && (pNode->nUdpDataPort == port))
		{
			pthread_mutex_unlock(&hMutex);
			net_debug();
			return TRUE;
		}

		// Add the code by lvjh, 2008-04-17
		pNode = pNode->pNext;
	}
    
	pthread_mutex_unlock(&hMutex);

	//net_debug();
    
	return FALSE;
}

// 关闭UDP数据传输
BOOL CloseUdpPlay(OPEN_CHANNEL closeHead)
{
	SEND_NODE *pNode = NULL;
	SEND_NODE *pPre = NULL;
	pthread_mutex_t hMutex;

	if (0 == closeHead.nStreamType)
	{
		pNode = g_server_info.udpStreamSend1.pUdpSendList[closeHead.nSerChn];
		hMutex = g_server_info.udpStreamSend1.hUdpSendListMutex[closeHead.nSerChn];
	}
	else
	{
		pNode = g_server_info.udpStreamSend2.pUdpSendList[closeHead.nSerChn];
		hMutex = g_server_info.udpStreamSend2.hUdpSendListMutex[closeHead.nSerChn];
	}

	if (NULL == pNode)
	{
		return FALSE;
	}

	pthread_mutex_lock(&hMutex);

	pPre = pNode;
	pNode = pNode->pNext;
    
	while (pNode)
	{
		//if (pNode->nTcpCmdSock==closeHead.nID && pNode->nType==closeHead.nProtocolType)
		if (pNode->nTcpCmdSock==closeHead.nID && pNode->nType==closeHead.nProtocolType && pNode->nClientID==closeHead.nClientID)	// Add the code by lvjh, 2008-03-28
		{
			pPre->pNext = pNode->pNext;
			free(pNode);
			//net_debug();
			break;
		}
		pPre = pNode;
		pNode = pNode->pNext;
	}

	pthread_mutex_unlock(&hMutex);

	return TRUE;
}

// 退出多播数据传输
BOOL ExistMultiUser(OPEN_CHANNEL openHead)
{
	SEND_NODE *pNode = NULL;
	pthread_mutex_t hMutex;

	if (0 == openHead.nStreamType)
	{
		pNode = g_server_info.udpStreamSend1.pUdpSendList[openHead.nSerChn];
		hMutex = g_server_info.udpStreamSend1.hUdpSendListMutex[openHead.nSerChn];
	}
	else
	{
		printf("ExistMultiUser: 1\n");
		pNode = g_server_info.udpStreamSend2.pUdpSendList[openHead.nSerChn];
		hMutex = g_server_info.udpStreamSend2.hUdpSendListMutex[openHead.nSerChn];
	}

	if (NULL == pNode)
	{
		return FALSE;
	}

	pthread_mutex_lock(&hMutex);

	pNode = pNode->pNext;
    
	while (pNode)
	{
		printf("ExistMultiUser: ...\n");
		if ((pNode->nTcpCmdSock == openHead.nID) && (pNode->nUdpDataIP == htonl(g_server_info.multiAddr)) && (pNode->nUdpDataPort == htonl(g_server_info.nBasePort+openHead.nSerChn)))
		//if (pNode->nTcpCmdSock==openHead.nID && pNode->nType==openHead.nProtocolType && pNode->nClientID==openHead.nClientID)	// Add the code by lvjh, 2008-03-28
		{
			pthread_mutex_unlock(&hMutex);
			return TRUE;
		}
		
		// Add the code by lvjh, 2008-04-17
		pNode = pNode->pNext;
	}

	pthread_mutex_unlock(&hMutex);
    
	return FALSE;
}

// 关闭多播数据传输
BOOL CloseMultiPlay(OPEN_CHANNEL closeHead)
{
	return CloseUdpPlay(closeHead);
}

// 多播数据传输请求
int RequestMultiPlay(OPEN_CHANNEL openHead)
{
	SEND_NODE *pNew = NULL;

	printf("RequestMultiPlay\n");

	pNew = (SEND_NODE *)malloc(sizeof(SEND_NODE));
	bzero(pNew,sizeof(SEND_NODE));

	pNew->status = RUN;
	pNew->pNext = NULL;
	pNew->hTcpDataSock = 0;					//tcp use,not use in udp

	pNew->nType = PROTOCOL_MULTI;
	pNew->nSerChn = openHead.nSerChn;
	pNew->nStreamType = openHead.nStreamType;
	pNew->nTcpCmdSock = openHead.nID;

	pNew->nUdpDataIP = g_server_info.multiAddr;
	pNew->nUdpDataPort = htons(g_server_info.nBasePort+openHead.nSerChn);
	
	pNew->nClientID = openHead.nClientID;	// Add the code by lvjh, 2008-03-28

	if (0 == openHead.nStreamType)
	{
		InsertPlayNode(g_server_info.udpStreamSend1.pUdpSendList[openHead.nSerChn], 
					g_server_info.udpStreamSend1.hUdpSendListMutex[openHead.nSerChn], pNew);
	}
	else
	{
		InsertPlayNode(g_server_info.udpStreamSend2.pUdpSendList[openHead.nSerChn],
					g_server_info.udpStreamSend2.hUdpSendListMutex[openHead.nSerChn], pNew);
	}

	return 0;
}

// 获取UDP数据传输的用户数
int GetUdpRunNode(int nChannel,int nStream)
{
	int nCount = 0;
	SEND_NODE *pPre = NULL;
	SEND_NODE *pNode = NULL;
    
	if (0 == nStream)
	{
		pthread_mutex_lock(&g_server_info.udpStreamSend1.hUdpSendListMutex[nChannel]);

		pNode = g_server_info.udpStreamSend1.pUdpSendList[nChannel];
		pPre = pNode;
		pNode = pNode->pNext;
        
		while (pNode)
		{
			if (STOP == pNode->status)
			{
				pPre->pNext = pNode->pNext;
				free(pNode);
				pNode = pPre->pNext;
			}
			else
			{
				nCount++;
				pPre = pNode;
				pNode=pNode->pNext;
			}
		}
		pthread_mutex_unlock(&g_server_info.udpStreamSend1.hUdpSendListMutex[nChannel]);
	}
	else
	{
		pthread_mutex_lock(&g_server_info.udpStreamSend2.hUdpSendListMutex[nChannel]);
		pNode = g_server_info.udpStreamSend2.pUdpSendList[nChannel];
		pPre = pNode;
		pNode = pNode->pNext;
		while(pNode)
		{
			if(STOP ==pNode->status)
			{
				pPre->pNext = pNode->pNext;
				free(pNode);
				pNode = pPre->pNext;
			}
			else
			{
				nCount++;
				pPre = pNode;
				pNode=pNode->pNext;
			}
		}
		pthread_mutex_unlock(&g_server_info.udpStreamSend2.hUdpSendListMutex[nChannel]);
	}

	return nCount;
}

// 停止指定的UDP数据传输
int StopUdpNode(int id)
{
	int i = 0;
	SEND_NODE *pNode = NULL;
    
	for (i=0; i<g_server_info.nChnNum; i++)
	{
		pthread_mutex_lock(&g_server_info.udpStreamSend1.hUdpSendListMutex[i]);

		if (g_server_info.udpStreamSend1.pUdpSendList[i])
		{
			pNode = g_server_info.udpStreamSend1.pUdpSendList[i]->pNext;
		}
		
		while (pNode)
		{
			if (pNode->nTcpCmdSock == id)
			{
				pNode->status = STOP;
			}
			pNode = pNode->pNext;
		}
        
		pthread_mutex_unlock(&g_server_info.udpStreamSend1.hUdpSendListMutex[i]);
		
		pthread_mutex_lock(&g_server_info.udpStreamSend2.hUdpSendListMutex[i]);
        
		if (g_server_info.udpStreamSend2.pUdpSendList[i])
		{
			pNode = g_server_info.udpStreamSend2.pUdpSendList[i]->pNext;
		}
        
		while (pNode)
		{
			if (pNode->nTcpCmdSock == id)
			{
				pNode->status = STOP;
			}
			pNode = pNode->pNext;
		}
		pthread_mutex_unlock(&g_server_info.udpStreamSend2.hUdpSendListMutex[i]);
	}

	return 0;
}

// 释放所有的UDP数据传输节点
int FreeAllUdpNode()
{
	int i = 0;
	SEND_NODE *pTmp = NULL;
	SEND_NODE *pNode = NULL;

	for (i=0; i<g_server_info.nChnNum; i++)
	{
		pthread_mutex_lock(&g_server_info.udpStreamSend1.hUdpSendListMutex[i]);
        
		if (g_server_info.udpStreamSend1.pUdpSendList[i])
		{
			pNode = g_server_info.udpStreamSend1.pUdpSendList[i];
		}
		
		while (pNode)
		{
			pTmp = pNode->pNext;
			free(pNode);
			pNode = pTmp;
		}
        
		pthread_mutex_unlock(&g_server_info.udpStreamSend1.hUdpSendListMutex[i]);

		pthread_mutex_lock(&g_server_info.udpStreamSend2.hUdpSendListMutex[i]);

		if (g_server_info.udpStreamSend2.pUdpSendList[i])
		{
			pNode = g_server_info.udpStreamSend2.pUdpSendList[i];
		}
        
		while (pNode)
		{
			pTmp = pNode->pNext;
			free(pNode);
			pNode = pTmp;
		}
        
		pthread_mutex_unlock(&g_server_info.udpStreamSend2.hUdpSendListMutex[i]);
	}

	return 0;
}

