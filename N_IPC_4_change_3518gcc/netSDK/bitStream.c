#include "netcomm.h"
#include "netSDK.h"

// 初始化码流通道
int InitChannel(CHANNEL_STREAM *pStream)
{
	int i = 0;
	int j = 0;
    
	if (NULL == pStream)
	{
		return -1;
	}
	
	pStream->nPoolSize = MAX_POOL_SIZE;
	pStream->nWritePos = 0;

	pthread_mutex_init(&pStream->hReadBlockMutex, NULL);
	pthread_cond_init(&pStream->hReadBlockCond, NULL);
	pthread_mutex_init(&pStream->hReadWriteLock, NULL);
	
	if (g_server_info.nSendBufferNum>0 && g_server_info.nSendBufferNum<MAX_POOL_SIZE)
	{
		pStream->nPoolSize = g_server_info.nSendBufferNum;
	}
	else
	{
		return -1;
	}

	//printf("InitChannel: %p %d\n", pStream, pStream->nPoolSize);

	for (i=0; i<pStream->nPoolSize; i++)
	{
		
		pStream->pPackPool[i] = (NET_DATA_PACKET *)malloc(sizeof(NET_DATA_PACKET));
		if (pStream->pPackPool[i] == NULL)
		{
			goto err;
		}
        
		memset(pStream->pPackPool[i], 0, sizeof(NET_DATA_PACKET));
		
	}

	return 0;

err:
	pthread_mutex_destroy(&pStream->hReadBlockMutex);
	pthread_cond_destroy(&pStream->hReadBlockCond);
	pthread_mutex_destroy(&pStream->hReadWriteLock);

	for (j=0; j<i; j++)
	{
		if (pStream->pPackPool[j] != NULL)
		{
			free(pStream->pPackPool[j]);
		}
	}

	return -1;
}

// 销毁码流通道
int DestroyStream(CHANNEL_STREAM *pStream)
{
	int i = 0;
    
	if (NULL == pStream)
	{
		return -1;
	}

	pthread_mutex_destroy(&pStream->hReadBlockMutex);
	pthread_cond_destroy(&pStream->hReadBlockCond);
	pthread_mutex_destroy(&pStream->hReadWriteLock);

	for (i=0; i<pStream->nPoolSize; i++)
	{
		if (pStream->pPackPool[i])
		{
			free(pStream->pPackPool[i]);
			pStream->pPackPool[i] = NULL;
		}
	}
	
	return 0;
}

// 写一帧数据到码流BUFFER
int WriteFrameToStream(unsigned long nChannel, CHANNEL_STREAM *pStream, const char *pFrame)
{
	int nFrameSize = 0;
	int nPacketNum = 0;
	int nSendNo = 0;
	int nWritePos = 0;
	unsigned long nPacketDataSize = 0;
	NET_DATA_PACKET *pPack = NULL;
	AV_FRAME_HEAD *pFrameHead = (AV_FRAME_HEAD *)pFrame;
	char *pFrameData = (char *)(pFrame + sizeof(AV_FRAME_HEAD));

	if (NULL == pStream)
	{
		return -1;
	}
	if (NULL == pFrame)
	{
		return -1;
	}

	//printf("WriteFrameToStream: %d %d %d %d %d %d\n", pFrameHead->nTimeTick, pFrameHead->nVideoSize, pFrameHead->nAudioSize, pFrameHead->nImageWidth, pFrameHead->nImageHeight, pFrameHead->bKeyFrame);

	nFrameSize = pFrameHead->nVideoSize + pFrameHead->nAudioSize;
	nPacketNum = (nFrameSize + PACK_SIZE - 1)/PACK_SIZE;
	if (0 == nPacketNum)
	{
		return -1;
	}

	if (nPacketNum > pStream->nPoolSize)
	{
		return -1;
	}
   
	pthread_mutex_lock(&pStream->hReadWriteLock);

	nWritePos = 0;
	nSendNo = 0;

	while ((nSendNo < nPacketNum) && (nFrameSize > 0))
	{
        pPack = pStream->pPackPool[pStream->nWritePos++];

        if (pStream->nWritePos == pStream->nPoolSize)
        {
            pStream->nWritePos = 0;
		}

		nPacketDataSize = (nFrameSize > PACK_SIZE) ? PACK_SIZE : nFrameSize;

		pPack->packHead.nFlag = HDVS_FLAG;
		pPack->packHead.nSize = sizeof(DATA_PACKET) - PACK_SIZE + nPacketDataSize;
		//pPack->packHead.nSize = sizeof(DATA_PACKET);

		pPack->packData.bIsDataHead = (nWritePos == 0)? TRUE : FALSE;
		pPack->packData.FrameHeader = *pFrameHead;
		pPack->packData.byPacketID = nSendNo++;
		pPack->packData.nChannel = nChannel;
		pPack->packData.nBufSize = nPacketDataSize;

		memcpy(pPack->packData.PackData, pFrameData+nWritePos, nPacketDataSize);

		nWritePos += nPacketDataSize;
		nFrameSize -= nPacketDataSize;
	}
    
	pthread_mutex_unlock(&pStream->hReadWriteLock);
	pthread_cond_broadcast(&pStream->hReadBlockCond);

	//printf("WritePos[%d %d]\n", pStream->nPoolSize, pStream->nWritePos);

	return 0;
}

// 从码流BUFFER里取一个数据包
BOOL GetOnePacket(CHANNEL_STREAM *pStream, NET_DATA_PACKET *pNetPack, READ_POSITION *pReadPos, BOOL bFirstRead)
{
	int ret = 0;
	BOOL bAdjust = FALSE;
	NET_DATA_PACKET *pFindPack;
	int index = 0;
	int nPacketDataSize = 0;
    
	if (NULL == pStream)
	{
		return FALSE;
	}
	if (NULL == pNetPack)
	{
		return FALSE;
	}
	if (NULL == pReadPos)
	{
		return FALSE;
	}

	pthread_mutex_lock(&pStream->hReadWriteLock);
       
	//printf("pReadPos[%d %d %d %d]\n", pReadPos->nBegin, pReadPos->nEnd, pStream->nPoolSize, pStream->nWritePos);
 
	if( ((pReadPos->nBegin <= pStream->nWritePos)&&(pStream->nWritePos < pReadPos->nEnd)) ||
		((pReadPos->nEnd < pReadPos->nBegin)&&(pReadPos->nBegin <= pStream->nWritePos)) ||
		((pStream->nWritePos < pReadPos->nEnd)&&(pReadPos->nEnd < pReadPos->nBegin)) ||
		(pReadPos->bLost) || (bFirstRead) )
	{
		bAdjust = TRUE;
	}
	
	pReadPos->nEnd = pStream->nWritePos;
	
	if (bAdjust)
	{
		pReadPos->bLost = TRUE;
		pReadPos->nBegin = pStream->nWritePos-1;
		if (pReadPos->nBegin < 0)
		{
			pReadPos->nBegin = pStream->nPoolSize - 1;
		}
            
		while (pReadPos->nBegin != pStream->nWritePos)
		{
			pFindPack = pStream->pPackPool[pReadPos->nBegin];

			//printf("Packet: %d %d\n", pFindPack->packData.bIsDataHead, pFindPack->packData.FrameHeader.bKeyFrame);

			if (pFindPack->packData.bIsDataHead && pFindPack->packData.FrameHeader.bKeyFrame)
			{
				pReadPos->bLost = FALSE;
				break;
			}

			pReadPos->nBegin--;
			if (pReadPos->nBegin < 0)
			{                  
				pReadPos->nBegin = pStream->nPoolSize - 1;
			}
		}       
        
		if (pReadPos->bLost)
		{
			pthread_mutex_unlock(&pStream->hReadWriteLock);
			return FALSE;
		}
	}

	memcpy(pNetPack, pStream->pPackPool[pReadPos->nBegin++], sizeof(NET_DATA_PACKET));

	/*
	index = pReadPos->nBegin++;
	nPacketDataSize = sizeof(NET_DATA_HEAD)+pStream->pPackPool[index]->packHead.nSize;
	memcpy(pNetPack, pStream->pPackPool[index], nPacketDataSize);
	*/

	//printf("GetOnePacket(): %d***%d***%d\n", pStream->pPackPool[index]->packHead.nSize, pNetPack->packHead.nSize, nPacketDataSize);

	if (pReadPos->nBegin == pStream->nPoolSize)
	{
		pReadPos->nBegin = 0;
	}
	
	pthread_mutex_unlock(&pStream->hReadWriteLock);
    
	return TRUE;
}

// 等待码流
int WakeupStreamWait()
{
	int i = 0;
    
	for (i=0; i<g_server_info.nChnNum; i++)
	{
		if (g_server_info.pChannelStream1[i])
		{
			pthread_cond_signal(&g_server_info.pChannelStream1[i]->hReadBlockCond);
		}
		if (g_server_info.pChannelStream2[i])
		{
			pthread_cond_signal(&g_server_info.pChannelStream2[i]->hReadBlockCond);
		}
    }

	return 0;
}
