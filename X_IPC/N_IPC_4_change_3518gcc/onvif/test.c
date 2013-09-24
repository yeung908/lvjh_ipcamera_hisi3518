#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#else 
#include <WINSOCK2.H>
#endif
#include <string.h>
#include "libonvif.h"
#include "../param.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "onvif.h"

SYS_PARAM g_sys_param;

struct RTSP_ConnThread
{
	pthread_t hthread;
	int		  sock;
	struct sockaddr_in remote_addr;
};


#ifndef _WIN32
int get_ip_addr(char *name,char *net_ip)
{
	struct ifreq ifr;
	int ret = 0;
	int fd;	
	
	strcpy(ifr.ifr_name, name);
	ifr.ifr_addr.sa_family = AF_INET;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		return -1;
	}
		
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) 
	{
		close(fd);
		return -1;
	}
	
	strcpy(net_ip,inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

	close(fd);

	return	0;
}
#else
int get_ip_addr(char *name,char *net_ip)
{
	strcpy(net_ip,"192.168.1.149");
}
#endif

extern unsigned int g_OnvifServiceRunning;

int ComparseCommand(const char *sBuf,const char *sCommand)
{
	if(0 == strncmp(sBuf,sCommand,strlen(sCommand))
	{
		return 1;
	}
	return 0;
}


int RTSP_NewConnThread(void *pPara)
{
	struct RTSP_ConnThread *pConnThread = (struct RTSP_ConnThread *)pPara;
	int sock = pConnThread->sock;
	int ret = 0;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	struct Http_Buffer RecvBuf;
	int   nNoDataCount = 0;
	memset(&RecvBuf,0,sizeof(RecvBuf));
	while(1)
	{
		if(nNoDataCount >= 5)
			break;
		FD_ZERO(&fset);
		FD_SET(sock,&fset);
		memset(&to,0,sizeof(to));
		to.tv_sec = 5;
		to.tv_usec = 0;
		ret = select(sock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0 && errno == EINTR)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0)
			break;
		if(!FD_ISSET(sock,&fset))
			break;
		ret = recv(sock,RecvBuf.Buffer + RecvBuf.nBufLen,65535 - RecvBuf.nBufLen,0);
//		printf("%s(%d) rtsp tcp recved %d bytes\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),ret);
		if(ret <= 0)
			break;
		nNoDataCount = 0;
		RecvBuf.nBufLen += ret;
		if(HttpParse(&RecvBuf) >= 0)
		{
			printf("HttpParse OK\n");
			printf("%s(%d) rtsp tcp recved total %d bytes:%s\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),RecvBuf.nBufLen,RecvBuf.Buffer);
			
			if(ComparseCommand(RecvBuf.Buffer,"OPTIONS",))
			{
				
			}
			else if(ComparseCommand(RecvBuf.Buffer,"DESCRIBE"))
			{
			}
			else if(ComparseCommand(RecvBuf.Buffer,"SETUP"))
			{
			}
			else if(ComparseCommand(RecvBuf.Buffer,"PLAY"))
			{
			}
			else
			{
				printf("unknow command...\n");
			}
		}
	}
	printf("rtsp connect thread exit\n");
	close(sock);
	free(pConnThread);
	pthread_exit(NULL);
	return 0;
}

int RTSPNewConnect(int hsock,struct sockaddr_in *pAddr)
{
	int ret;
	struct RTSP_ConnThread *pConnThread = (struct RTSP_ConnThread *)malloc(sizeof(struct RTSP_ConnThread));
	if(pConnThread == NULL)
	{
		printf("rtsp new connect malloc memeory error\n");
		return -1;
	}
	memset(pConnThread,0,sizeof(struct RTSP_ConnThread));
	pConnThread->sock = hsock;
	memcpy(&pConnThread->remote_addr,pAddr,sizeof(struct sockaddr_in));
	ret = pthread_create(&pConnThread->hthread,NULL,(void *)&RTSP_NewConnThread,pConnThread);
	if(ret < 0)
	{
		printf("create rtsp new connect thread error\n");
		close(hsock);
		free(pConnThread);
		return -1;
	}
	return 0;
}

int RTSP_ServiceThread()
{
	int ret = 0;
	int len = 0;
	int opt;
	int hConnSock = -1;
	int hListenSock = -1;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	FD_ZERO(&fset);
	memset(&to,0,sizeof(to));
	memset(&addr,0,sizeof(addr));
	len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(g_sys_param.rtsp.nRtspPort);
	hListenSock = socket(AF_INET,SOCK_STREAM,0);
	if(hListenSock < 0)
	{
		printf("create rtsp service listen sock error\n");
		pthread_exit(NULL);
		return -1;
	}
	ret = bind(hListenSock,(struct sockaddr *)&addr,sizeof(addr));
	if(ret < 0)
	{
		printf("bind rtsp service listen sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return ret;
	}
	ret = listen(hListenSock,200);
	if(ret < 0)
	{
		printf("listen rtsp sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return ret;
	}
	while(g_OnvifServiceRunning)
	{
		hConnSock = -1;
		to.tv_sec = 3;
		to.tv_usec = 0;
		FD_SET(hListenSock,&fset);
		ret = select(hListenSock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
			continue;
		if(ret < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				printf("rtsp listen sock select error\n");
				break;
			}
		}
		if(!FD_ISSET(hListenSock,&fset))
			continue;
		if(!g_OnvifServiceRunning)
			break;
		hConnSock = accept(hListenSock,(struct sockaddr *)&addr,&len);
		if(hConnSock < 0)
		{
			if(errno == 1000)
			{
				printf("too many file opened\n");
				continue;
			}
		}
		printf("client connected:%s(%d)\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		opt = 1;
		ret = setsockopt(hConnSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
		if (ret < 0)
		{
			printf("set rtsp tcp sockopt error\n");
			close(hConnSock);
			continue;
		}
		RTSPNewConnect(hConnSock,&addr);
	}
	close(hListenSock);
	pthread_exit(NULL);
	return 0;
}

int RTSP_ServiceStart()
{
	int ret;
	pthread_t hRtspThread;
	ret = pthread_create(&hRtspThread,NULL,(void *)&RTSP_ServiceThread,NULL);
	if(ret < 0)
	{
		printf("create rtsp service thread error\n");
		return -1;
	}
	return 0;
}

int main()
{
  memset(&g_sys_param,0,sizeof(g_sys_param));
  strcpy(g_sys_param.sysInfo.strDeviceID,"20030722000001");
  g_sys_param.sysInfo.nHardwareVersion = HARDWARE_VERSION;
  g_sys_param.sysInfo.nSoftwareVersion = SOFTWARE_VERSION;
  g_sys_param.videoEnc[0][0].nEncodeWidth = 1600;
  g_sys_param.videoEnc[0][0].nEncodeHeight = 1200;
  g_sys_param.videoEnc[0][1].nEncodeWidth = 800;
  g_sys_param.videoEnc[0][1].nEncodeHeight = 600;
  strcpy(g_sys_param.userInfo.Admin.strName,"admin");
  strcpy(g_sys_param.userInfo.Admin.strPsw,"admin");
  g_sys_param.videoEnc[0][0].nFramerate = g_sys_param.videoEnc[0][1].nFramerate = 30;
  g_sys_param.videoEnc[0][0].nKeyInterval = g_sys_param.videoEnc[0][1].nKeyInterval = 100;
  g_sys_param.rtsp.nRtspPort = 1554;
  ONVIFStart();
  RTSP_ServiceStart();
  while(1)
  {
    sleep(1);
  }
  return 0;
}
