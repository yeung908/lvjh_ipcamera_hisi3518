#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"
#include "param.h"
#include "session.h"

#include "searchDevice.h"

int g_wire_search_run_flag = 0;
int g_wire_search_pause_flag = 0;
int g_wireless_search_run_flag = 0;
int g_wireless_search_pause_flag = 0;

int wireSearchFun(void *param)
{
	int ret = -1;
	socklen_t len;

	fd_set fset;
	struct timeval to;
    
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	WIFI_PARAM wifiParam;
	NET_PARAM *pNetParam = NULL;
	USER_INFO_PARAM userInfo;
	USER_INFO *pUserInfo = NULL;
	char *pDeviceName = NULL;
	NET_HEAD *netHead;
	DEV_CMD_HEADER *devCmdHeader;
	char buffer[1024];

	char ipAddr[16];
	char maskAddr[16];
	char gateAddr[16];
	
	int listenBroadfd = -1;
    
	int opt;
	struct sockaddr_in serverAddr;	
	struct sockaddr_in multiAddr;
	struct ip_mreq ipMreq;

	int wirelessFlag = 0;

	//printf("Search Device(Wire): %s\n", ipAddr);

	listenBroadfd = socket(AF_INET,SOCK_DGRAM, 0);	
	if (-1 == listenBroadfd)
	{
		printf("socket() Failed!\n");
		return -1;
	}
	
	opt = 1;
	ret = setsockopt(listenBroadfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	//SO_BROADCAST
	if (ret < 0)
	{
		printf("setsockopt() Failed!\n");
		return -1;
	}
    
	serverAddr.sin_family = AF_INET;       
	serverAddr.sin_port = htons(54321);
	serverAddr.sin_addr.s_addr = 0;   
	//serverAddr.sin_addr.s_addr = inet_addr(ipAddr);  
	

	bind(listenBroadfd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in)); 

	getWifiParam(&wifiParam);

	if (wifiParam.nOnFlag == 1)
	{
		// 加入多播组
		get_ip_addr(ETH_WIRE_DEV, ipAddr);
		ipMreq.imr_multiaddr.s_addr = inet_addr("224.99.99.99");
		ipMreq.imr_interface.s_addr = inet_addr(ipAddr);
		//ipMreq.imr_interface.s_addr = 0;
		ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipMreq, sizeof(ipMreq));
		if (ret < 0)
		{
			printf("setsockopt(IP_ADD_MEMBERSHIP) Failed!\n");
			printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
			//return -1;
		}	

		get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
		ipMreq.imr_multiaddr.s_addr = inet_addr("224.99.99.99");
		ipMreq.imr_interface.s_addr = inet_addr(ipAddr);
		//ipMreq.imr_interface.s_addr = 0;
		ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipMreq, sizeof(ipMreq));
		if (ret < 0)
		{
			printf("setsockopt(IP_ADD_MEMBERSHIP) Failed!\n");
			printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
			//return -1;
		}
		else
		{
			wirelessFlag = 1;
		}
	}
	else
	{
		// 加入多播组
		ipMreq.imr_multiaddr.s_addr = inet_addr("224.99.99.99");
		ipMreq.imr_interface.s_addr = 0;
		ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipMreq, sizeof(ipMreq));
		if (ret < 0)
		{
			printf("setsockopt(IP_ADD_MEMBERSHIP) Failed!\n");
			printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
			return -1;
		}	
	}

	opt = 0;
	ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt));
	if (ret < 0)
	{
		printf("setsockopt(IP_MULTICAST_LOOP) Failed!\n");
		printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	
	// 有线多播
	get_ip_addr(ETH_WIRE_DEV, ipAddr);
	opt = inet_addr(ipAddr);
	ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
	if (ret < 0)
	{
		printf("setsockopt(IP_MULTICAST_IF: %s) Failed!\n", ipAddr);
		printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	memset(&to, 0, sizeof(to));

	while (g_wire_search_run_flag)
	{
		if (g_wire_search_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		FD_ZERO(&fset);
		FD_SET(listenBroadfd, &fset);
		to.tv_sec = 60;
		to.tv_usec = 0;

		// SOCKET接收超时或错误,则退出
		ret = select(listenBroadfd+1, &fset, NULL, NULL, &to);
		if ( ret == 0)
		{
			continue;
		}
		if (ret<0 && errno==EINTR)
		{
			continue;
		}
		if (ret < 0)
		{
			return -1;
		}

		if (!FD_ISSET(listenBroadfd, &fset))
		{
			return -1;
		}

		len = sizeof(multiAddr);
		ret = recvfrom(listenBroadfd, buffer, 1024, 0, (struct sockaddr*)&multiAddr, &len);

		printf("SearchDevice Client: %s\n", inet_ntoa(multiAddr.sin_addr));

		
		if (ret >= sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER))
		{
			netHead = (NET_HEAD *)buffer;
			devCmdHeader = (DEV_CMD_HEADER *)(buffer+sizeof(NET_HEAD));
				
			// 搜索IP
			if (netHead->nFlag==HDVS_FLAG && !(netHead->nReserve&0x0F) && netHead->nCommand==NETCMD_USER_CMD && devCmdHeader->nCmdID==GET_BROADCAST_INFO)
			{
				struct in_addr mAddr;
				
				// 有线
				memset(ipAddr, 0, 16);
				memset(maskAddr, 0, 16);
				memset(gateAddr, 0, 16);

				getSysInfoParam(&sysInfo);
				getNetParam(&netParam);
				get_ip_addr(ETH_WIRE_DEV, ipAddr);
				get_mask_addr(ETH_WIRE_DEV, maskAddr);
				//get_gateway_addr(gateAddr);
				get_gateway_addr_ext(ETH_WIRE_DEV, gateAddr);

				strcpy(netParam.byServerIp, ipAddr);
				strcpy(netParam.byServerMask, maskAddr);
				strcpy(netParam.byGateway, gateAddr);

				netHead->nReserve = 0x01;
				netHead->nBufSize = sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM);
				devCmdHeader->nCmdLen = sizeof(SYS_INFO)+sizeof(NET_PARAM);
			
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER), &sysInfo, sizeof(SYS_INFO));
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
				
				// 有线多播
				get_ip_addr(ETH_WIRE_DEV, ipAddr);
				//opt = inet_addr(ipAddr);
				//ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
				// Add the code by lvjh, 2009-04-17
				memset(&mAddr, 0, sizeof(mAddr));
				mAddr.s_addr = inet_addr(ipAddr);
				ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &mAddr, sizeof(opt));
				if (ret < 0)
				{
					printf("setsockopt(IP_MULTICAST_IF: %s) Failed!\n", ipAddr);
					printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
					//return -1;
				}

				multiAddr.sin_family = AF_INET;
				multiAddr.sin_addr.s_addr = inet_addr("224.99.99.99");
				multiAddr.sin_port = htons(54321);

				ret = sendto(listenBroadfd, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM), 0, (struct sockaddr*)&multiAddr, len); 

				printf("SearchDevice(WIRE: %s %s %s %s): %d\n", netParam.byServerIp, netParam.byServerMask, netParam.byGateway, netParam.byDnsAddr, ret);
				
				
				if (wirelessFlag != 1)
				{
					continue;
				}

				// 无线多播
				memset(ipAddr, 0, 16);
				memset(maskAddr, 0, 16);
				memset(gateAddr, 0, 16);

				getSysInfoParam(&sysInfo);
				getNetParam(&netParam);
				getWifiParam(&wifiParam);
				get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
				get_mask_addr(ETH_WIRELESS_DEV, maskAddr);
				//get_gateway_addr(gateAddr);
				get_gateway_addr_ext(ETH_WIRELESS_DEV, gateAddr);

				netParam.nDhcpOnFlag = wifiParam.nDhcpOnFlag;
				strcpy(netParam.byServerIp, ipAddr);
				strcpy(netParam.byServerMask, maskAddr);
				strcpy(netParam.byGateway, gateAddr);
				strcpy(netParam.byDnsAddr, wifiParam.byDnsAddr);

				netHead->nReserve = 0x01;
				netHead->nBufSize = sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM);
				devCmdHeader->nCmdLen = sizeof(SYS_INFO)+sizeof(NET_PARAM);
			
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER), &sysInfo, sizeof(SYS_INFO));
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
				
				// 线多播
				get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
				//printf("Wireless: %s\n", ipAddr);
				//opt = inet_addr(ipAddr);
				//rt = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
				// Add the code by lvjh, 2009-04-17
				memset(&mAddr, 0, sizeof(mAddr));
				mAddr.s_addr = inet_addr(ipAddr);
				ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &mAddr, sizeof(opt));
				if (ret < 0)
				{
					printf("setsockopt(IP_MULTICAST_IF: %s) Failed!\n", ipAddr);
					printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
					//return -1;
				}

				multiAddr.sin_family = AF_INET;
				multiAddr.sin_addr.s_addr = inet_addr("224.99.99.99");
				multiAddr.sin_port = htons(54321);

                ret = sendto(listenBroadfd, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM), 0, (struct sockaddr*)&multiAddr, len); 

				printf("SearchDevice(WIRELESS: %s %s %s %s): %d\n", netParam.byServerIp, netParam.byServerMask, netParam.byGateway, netParam.byDnsAddr, ret);
				
			}
			
			// 修改IP
			if (netHead->nFlag==HDVS_FLAG && !(netHead->nReserve&0x0F) && netHead->nCommand==NETCMD_USER_CMD && devCmdHeader->nCmdID==CONFIG_SERVER_INFO 
				&& ret>=sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO)+32+sizeof(NET_PARAM))
			{
				pUserInfo = (USER_INFO *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER));
				pDeviceName = (char *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO));
				pNetParam = (NET_PARAM *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO)+32);
			
				getUserInfoParam(&userInfo);
				getNetParam(&netParam);
				getSysInfoParam(&sysInfo);

				get_ip_addr(ETH_WIRE_DEV, ipAddr);
				get_mask_addr(ETH_WIRE_DEV, maskAddr);
				//get_gateway_addr(gateAddr);
				get_gateway_addr_ext(ETH_WIRE_DEV, gateAddr);

				strcpy(netParam.byServerIp, ipAddr);
				strcpy(netParam.byServerMask, maskAddr);
				strcpy(netParam.byGateway, gateAddr);
				
				//if (!strcmp(netParam.strPhyAddr, pNetParam->strPhyAddr))	// Add the code by lvjh, 2008-11-1
				if (netParam.strPhyAddr[0] == pNetParam->strPhyAddr[0] 
					&& netParam.strPhyAddr[1] == pNetParam->strPhyAddr[1]
					&& netParam.strPhyAddr[2] == pNetParam->strPhyAddr[2]
					&& netParam.strPhyAddr[3] == pNetParam->strPhyAddr[3]
					&& netParam.strPhyAddr[4] == pNetParam->strPhyAddr[4]
					&& netParam.strPhyAddr[5] == pNetParam->strPhyAddr[5]) 
				{
					//printf("MAC: %s %s\n", netParam.strPhyAddr, pNetParam->strPhyAddr);
					int rebootFlag = 0;

					if (!strcmp(ADMIN_USER, pUserInfo->strName) && !strcmp(userInfo.Admin.strPsw, pUserInfo->strPsw))
					{
						// System Info
						strncpy(sysInfo.strDeviceName, pDeviceName, 32);
						setSysInfoParam(&sysInfo);
					
						// Net Parameter
						setNetParam(pNetParam);
					
						// Save Parameter
						saveParamToFile();
						
						ret = 0;
						rebootFlag = 1;
					}
					else
					{
						ret = -1;
						rebootFlag = 0;
					}
					
					// Return 
					netHead->nReserve = 0x01;
					netHead->nBufSize = sizeof(DEV_CMD_HEADER)+32+sizeof(NET_PARAM);
					devCmdHeader->nCmdLen = 32+sizeof(NET_PARAM);
					if (ret == 0)
					{
						netHead->nErrorCode = 0;
						devCmdHeader->nChannel = 0;
					}
					else
					{
						netHead->nErrorCode = NETERR_USER_PSW;
						devCmdHeader->nChannel = NETERR_USER_PSW;
					}
					memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER), sysInfo.strDeviceName, 32);
					memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+32, &netParam, sizeof(NET_PARAM));
					
					// 有线多播
					get_ip_addr(ETH_WIRE_DEV, ipAddr);
					opt = inet_addr(ipAddr);
					ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
					if (ret < 0)
					{
						printf("setsockopt(IP_MULTICAST_IF: %s) Failed!\n", ipAddr);
						printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
						//return -1;
					}

					multiAddr.sin_family = AF_INET;
					multiAddr.sin_addr.s_addr = inet_addr("224.99.99.99");
					multiAddr.sin_port = htons(54321);

					sendto(listenBroadfd, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+32+sizeof(NET_PARAM), 0, (struct sockaddr*)&multiAddr, len); 
					
					if (rebootFlag == 1)
					{
						RebootSystem();
					}
				}
			}
		}
		
	}
	
	return 0;
}

int wirelessSearchFun(void *param)
{
	int ret = -1;
	socklen_t len;

	fd_set fset;
	struct timeval to;
    
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	WIFI_PARAM wifiParam;
	NET_PARAM *pNetParam = NULL;
	USER_INFO_PARAM userInfo;
	USER_INFO *pUserInfo = NULL;
	char *pDeviceName = NULL;
	NET_HEAD *netHead;
	DEV_CMD_HEADER *devCmdHeader;
	char buffer[1024];

	char ipAddr[16];
	char maskAddr[16];
	char gateAddr[16];
	
	int listenBroadfd = -1;
    
	int opt;
	struct sockaddr_in serverAddr;	
	struct sockaddr_in multiAddr;
	struct ip_mreq ipMreq;
	
	//printf("Search Device(Wireless): %s\n", ipAddr);

	listenBroadfd = socket(AF_INET, SOCK_DGRAM, 0);	
	if (-1 == listenBroadfd)
	{
		printf("socket() Failed!\n");
		return -1;
	}
	
	opt = 1; 
	ret = setsockopt(listenBroadfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));	//SO_BROADCAST
	if (ret < 0)
	{
		printf("setsockopt() Failed!\n");
		return -1;
	}
    
	serverAddr.sin_family = AF_INET;       
	serverAddr.sin_port = htons(54321);
	serverAddr.sin_addr.s_addr = 0;

	bind(listenBroadfd, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in)); 

	// 加入多播组
	get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
	ipMreq.imr_multiaddr.s_addr = inet_addr("224.99.99.99");
	ipMreq.imr_interface.s_addr = inet_addr(ipAddr);

	ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipMreq, sizeof(ipMreq));
	if (ret < 0)
	{
		printf("setsockopt(IP_ADD_MEMBERSHIP) Failed!\n");
		printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}	

	opt = 0;
	ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt));
	if (ret < 0)
	{
		printf("setsockopt(IP_MULTICAST_LOOP) Failed!\n");
		printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
		return -1;
	}

	printf("wirelessSearchFun(%s): ... \n", ipAddr);

	memset(&to, 0, sizeof(to));

	while (g_wireless_search_run_flag)
	{
		if (g_wireless_search_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		FD_ZERO(&fset);
		FD_SET(listenBroadfd, &fset);
		to.tv_sec = 60;
		to.tv_usec = 0;

		// SOCKET接收超时或错误,则退出
		ret = select(listenBroadfd+1, &fset, NULL, NULL, &to);
		if ( ret == 0)
		{
			continue;
		}
		if (ret<0 && errno==EINTR)
		{
			continue;
		}
		if (ret < 0)
		{
			return -1;
		}

		if (!FD_ISSET(listenBroadfd, &fset))
		{
			return -1;
		}

		len = sizeof(multiAddr);
		ret = recvfrom(listenBroadfd, buffer, 1024, 0, (struct sockaddr*)&multiAddr, &len);
		if (ret >= sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER))
		{
			netHead = (NET_HEAD *)buffer;
			devCmdHeader = (DEV_CMD_HEADER *)(buffer+sizeof(NET_HEAD));
			

			// 无线多播
			get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
			
			opt = inet_addr(ipAddr);
			ret = setsockopt(listenBroadfd, IPPROTO_IP, IP_MULTICAST_IF, &opt, sizeof(opt));
			if (ret < 0)
			{
				printf("setsockopt(IP_MULTICAST_IF: %s) Failed!\n", ipAddr);
				printf(" %s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno));
				return -1;
			}

			// 搜索IP
			if (netHead->nFlag==HDVS_FLAG && !(netHead->nReserve&0x0F) && netHead->nCommand==NETCMD_USER_CMD && devCmdHeader->nCmdID==GET_BROADCAST_INFO)
			{
				// 无线
				memset(ipAddr, 0, 16);
				memset(maskAddr, 0, 16);
				memset(gateAddr, 0, 16);

				getSysInfoParam(&sysInfo);
				getNetParam(&netParam);
				getWifiParam(&wifiParam);
				get_ip_addr(ETH_WIRELESS_DEV, ipAddr);
				get_mask_addr(ETH_WIRELESS_DEV, maskAddr);
				//get_gateway_addr(gateAddr);
				get_gateway_addr_ext(ETH_WIRELESS_DEV, gateAddr);

				netParam.nDhcpOnFlag = wifiParam.nDhcpOnFlag;
				strcpy(netParam.byServerIp, ipAddr);
				strcpy(netParam.byServerMask, maskAddr);
				strcpy(netParam.byGateway, gateAddr);
				strcpy(netParam.byDnsAddr, wifiParam.byDnsAddr);

				netHead->nReserve = 0x01;
				netHead->nBufSize = sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM);
				devCmdHeader->nCmdLen = sizeof(SYS_INFO)+sizeof(NET_PARAM);
			
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER), &sysInfo, sizeof(SYS_INFO));
				memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
				
				multiAddr.sin_family = AF_INET;
				multiAddr.sin_addr.s_addr = inet_addr("224.99.99.99");
				multiAddr.sin_port = htons(54321);

                ret = sendto(listenBroadfd, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(SYS_INFO)+sizeof(NET_PARAM), 0, (struct sockaddr*)&multiAddr, len); 

				printf("SearchDevice(WIRELESS: %s %s %s %s): %d\n", netParam.byServerIp, netParam.byServerMask, netParam.byGateway, netParam.byDnsAddr, ret);
			}
			
			// 修改IP
			if (netHead->nFlag==HDVS_FLAG && !(netHead->nReserve&0x0F) && netHead->nCommand==NETCMD_USER_CMD && devCmdHeader->nCmdID==CONFIG_SERVER_INFO 
				&& ret>=sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO)+32+sizeof(NET_PARAM))
			{
				pUserInfo = (USER_INFO *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER));
				pDeviceName = (char *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO));
				pNetParam = (NET_PARAM *)(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(USER_INFO)+32);
			
				getUserInfoParam(&userInfo);
				getNetParam(&netParam);
				getSysInfoParam(&sysInfo);

				get_ip_addr(ETH_WIRE_DEV, ipAddr);
				get_mask_addr(ETH_WIRE_DEV, maskAddr);
				//get_gateway_addr(gateAddr);
				get_gateway_addr_ext(ETH_WIRELESS_DEV, gateAddr);

				strcpy(netParam.byServerIp, ipAddr);
				strcpy(netParam.byServerMask, maskAddr);
				strcpy(netParam.byGateway, gateAddr);
				
				if (!strcmp(netParam.strPhyAddr, pNetParam->strPhyAddr)) 
				{
					if (!strcmp(ADMIN_USER, pUserInfo->strName) && !strcmp(userInfo.Admin.strPsw, pUserInfo->strPsw))
					{
						// System Info
						strncpy(sysInfo.strDeviceName, pDeviceName, 32);
						setSysInfoParam(&sysInfo);
					
						// Net Parameter
						setNetParam(pNetParam);
					
						// Save Parameter
						saveParamToFile();
						
						ret = 0;
					}
					else
					{
						ret = -1;
					}
					
					// Return 
					netHead->nReserve = 0x01;
					netHead->nBufSize = sizeof(DEV_CMD_HEADER)+32+sizeof(NET_PARAM);
					devCmdHeader->nCmdLen = 32+sizeof(NET_PARAM);
					if (ret == 0)
					{
						netHead->nErrorCode = 0;
						devCmdHeader->nChannel = 0;
					}
					else
					{
						netHead->nErrorCode = NETERR_USER_PSW;
						devCmdHeader->nChannel = NETERR_USER_PSW;
					}
					memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER), sysInfo.strDeviceName, 32);
					memcpy(buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+32, &netParam, sizeof(NET_PARAM));
					
					multiAddr.sin_family = AF_INET;
					multiAddr.sin_addr.s_addr = inet_addr("224.99.99.99");
					multiAddr.sin_port = htons(54321);

					sendto(listenBroadfd, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+32+sizeof(NET_PARAM), 0, (struct sockaddr*)&multiAddr, len); 
					
					if (ret == 0)
					{
						RebootSystem();
					}
				}
			}
		}

		//sleep(1);
	}
	
	return 0;
}

int serInfoBroadcastStart()
{
	int ret = -1;
	pthread_t threadID;
	pthread_t wifi_threadID;
	
	g_wire_search_run_flag = 1;

	//printf("#########################################\n");
	//printf("func(%s) %s %d\n", __func__, __FILE__, __LINE__);
	//sleep(30);
	ret = pthread_create(&threadID, NULL, (void *)wireSearchFun, NULL);
	if (ret < 0)
	{
		g_wire_search_run_flag = 0;
		return -1;
	}
	

	#if 0
	g_wireless_search_run_flag = 1;
	ret = pthread_create(&wifi_threadID, NULL, (void *)wirelessSearchFun, NULL);
	if (ret < 0)
	{
		g_wireless_search_run_flag = 0;
		return -1;
	}
	#endif
	
	return 0;
}

int serInfoBroadcastStop()
{
	g_wire_search_run_flag = 0;
	//g_wireless_search_run_flag = 0;
	
	return 0;
}

int serInfoBroadcastPause()
{
	g_wire_search_pause_flag = 1;
	//g_wireless_search_pause_flag = 1;
	
	return 0;
}

int serInfoBroadcastResume()
{
	g_wire_search_pause_flag = 0;
	//g_wireless_search_pause_flag = 0;

	return 0;
}

