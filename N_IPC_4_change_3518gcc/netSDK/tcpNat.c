#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include "netcomm.h"
#include "netSDK.h"
#include "talkBack.h"
#include "tcpNat.h"
#include "../param.h"  //mbl add
#include "../audioDecAppModule.h"


int g_tcp_nat_run_flag = 0;
int g_tcp_nat_pause_flag = 0;
//mody by lv start add--------------------------------------------
int g_udp_nat_run_flag = 0;
int g_udp_nat_pause_flag = 0;

unsigned long g_p2p_UserID = 0;
//mody by lv end add--------------------------------------------
int g_tcp_nat_keepalive_count = 0;

char g_remote_ip[128];
int g_remote_socket = 0;
int g_remote_port = 0;
int g_intervalTime = 3;

char g_return_data[1024];
int g_return_data_len = 0;

pthread_mutex_t work_mutex;  //mbl add

fd_set g_fset;
struct timeval g_to;

int g_nat_flag = 0;
int g_nat_p2p_flag = 0;
//mody by lv start add--------------------------------------------
int g_nat_p2p_udp_flag = 0;


int remoteRequestProc_remote(int sockfd)
{
	int hConnSock = -1;
	int ret = -1;
	int len = -1;
	int level;
	int nCurNum = 0;

	fd_set fset;
	struct timeval to;

	NET_HEAD netHead;
	TALKTHRD_PARAM talkThrdPar;
	struct sockaddr_in addr;
	char buffer[UPD_RCV_BUFFER_LEN];

	if (sockfd <= 0 || sockfd > 65535)
	{
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	memset(&netHead, 0, sizeof(NET_HEAD));
	bzero(&to, sizeof(to));

	len = sizeof(addr);
	hConnSock = sockfd;

	// Setup attribute of the connected socket
	SetConnSockAttr(hConnSock, SOCK_TIME_OUT);

	FD_ZERO(&fset);
	FD_SET(hConnSock, &fset);
	to.tv_sec = SOCK_TIME_OUT;
	to.tv_usec = 0;

	while(1){
		#if 0
		ret = select(hConnSock+1, &fset, NULL, NULL, &to);
		if ( ret == 0)
		{
			net_debug();
			return 0;
		}
		if (ret<0 && errno==EINTR)
		{
			net_debug();
			return 0;
		}
		if (ret < 0)
		{
			printf("NAT(%d): Select Error!\n", hConnSock);
			net_debug();
			return -1;
		}
		if (!FD_ISSET(hConnSock, &fset))
		{
			printf("NAT(%d): FD_ISSET Error!\n", hConnSock);
			net_debug();
			return -1;
		}
		#endif
		printf("waitting for ....\n");
		// Receive network protocal header
		ret = recv(hConnSock, &buffer, sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_REGISTER_INFO) , 0);
		if (ret <= 0)
		{
			printf("NAT(%d): Recv Error!\n", hConnSock);
			net_debug();
			return -1;
		}
		memcpy(&netHead, &buffer, sizeof(NET_HEAD));	
		
		// Check the flag of network packet
		if (netHead.nFlag != HDVS_FLAG)
		{
			printf("NAT(%d): Flag Error!\n", hConnSock);
			return -1;
		}
		printf("netHead.nCommand = %p\n", netHead.nCommand);
		
		// process command of network
		switch (netHead.nCommand)
		{
		case NETCMD_LOGON:	// logon
			{
				int nCurNum = 0;
				int nRight = 0;		
				int nClientID = 0;                
				NET_USER_INFO userInfo;                

				printf("NETCMD_LOGON_R ... \n");

				memset(&userInfo, 0, sizeof(userInfo));
					
				// check the length of data
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

	                printf("NETCMD_LOGON_R(%d): NETERR_ILLEGAL_PARAM\n", hConnSock);

					return -1;
				}								
					
				// Receive user information
				ret = recv(hConnSock, &userInfo, sizeof(userInfo), 0);
				if (ret <= 0)
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
	                	
					return -1;
				}

				// Check logon number
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

					printf("NETCMD_LOGON_R(%d): NETERR_LOGON_FULL\n", hConnSock);

					return -1;
				}

				// Check user information
				if (g_server_info.funcCheckUserPsw)
				{
					MSG_HEAD msgHead;

					msgHead.nCmd = NETCMD_LOGON;
					msgHead.nSock = hConnSock;
	                    
					ret = g_server_info.funcCheckUserPsw(&msgHead, userInfo.szUserName, userInfo.szUserPsw);
					if (ret < 0)
					{
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
	                	
						printf("NETCMD_LOGON_R(%d): NETERR_USER_PSW\n", hConnSock);

						return -1;
					}
					nRight = ret;

				}

				// logon
				ret = ClientLogon(addr, userInfo, hConnSock, nRight, TCP_FLAG);
				if (ret < 0)
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
					                
					printf("NETCMD_LOGON_R(%d): ClientLogon failed\n", hConnSock);
		        		
					return -1;
				}
	        	
				// Notify the data send thread
				pthread_mutex_lock(&g_server_info.msgQuitThreadMutex);
				g_server_info.msgWaitThreadNum ++;
				pthread_mutex_unlock(&g_server_info.msgQuitThreadMutex);
				pthread_cond_signal(&g_server_info.msgThreadCond);
				
				printf("NETCMD_LOGON_R OK(%d)\n", hConnSock);
				
				return 0;
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
				//ret = recv(hConnSock, &openChannel, sizeof(OPEN_CHANNEL), 0);
				//if (ret<=0 || ret<sizeof(OPEN_CHANNEL))
				// Change the code by lvjh, 2009-04-18
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
	        	
				// 登陆成功,通知数据线程池
				pthread_mutex_lock(&g_server_info.dataQuitThreadMutex);
				g_server_info.dataWaitThreadNum++;
				pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
				pthread_cond_signal(&g_server_info.dataThreadCond); 
			}	
		// Add the code by lvjh, 2010-01-14
		case NETCMD_OPEN_TALK:
			{
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
				printf("NETCMD_OPEN_TALK: open %d (%s %d)\n", hConnSock, __FILE__, __LINE__);
			/*=======================================================================================*/

			   //打开AOUT设备 add code by by liujw

			   ret = audioDecModuleOpen(3);  //add code by liujw
				printf("audioOutOpen\n");
				if (ret)	
				{   
					printf("audioOutOpen(%s %d) Failed!\n", __FILE__, __LINE__);
					audioDecModuleClose();
					//return -1;	
				}

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

				talkParam->nEncType = WAVE_FORMAT_ALAW;  //modify by zhangjing //通知客服端音频编码格式
				talkParam->nAinChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAinBits = 16;
				talkParam->nAinSamples = g_server_info.avInfoStream1[0].nAudioSamples;
				talkParam->nDecType = WAVE_FORMAT_ALAW;  //modify by zhangjing //通知客服端音频编码格式
				talkParam->nAoutChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
				talkParam->nAoutBits = 16;
				talkParam->nAoutSamples = g_server_info.avInfoStream1[0].nAudioSamples;
						
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

		case NETCMD_P2P_CMD:
			printf("CMD: NETCMD_P2P_CMD\n");
			DVSNET_REGISTER_INFO register_return_info;
			
			memcpy(&register_return_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REGISTER_INFO));
			
			g_p2p_UserID = register_return_info.dwUserID;
			printf("g_p2p_UserID = %d\n\n\n", g_p2p_UserID );
			
			#if 1
			// send keep alive packet
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			
			ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", hConnSock);
				break;
			}
			#endif
			break;
			
		case NETCMD_KEEP_ALIVE:
			printf("CMD: NETCMD_KEEP_ALIVE\n");
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			
			ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", hConnSock);
				break;
			}
			break;
			
		//请求反向连接PC---->P2P Server------>IPC
		case NETCMD_P2P_REQCONBACK:
			printf("CMD: NETCMD_P2P_REQCONBACK\n");
	    	#if 1
			DVSNET_REQCONBACK_INFO conback_info;
			NET_HEAD conback_netHead;
			COMMAND_HEAD conback_netComm;
			int conback_send_lenght;
			
			char buffer_return[1000];
			SYS_INFO sysInfo;
			NET_PARAM netParam;

			memcpy(&conback_info, &buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQCONBACK_INFO));

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
			DVSNET_ENDCONBACK_INFO  result_endconback;

			result_endconback.dwReserved = 1;
			result_endconback.dwUserID = g_p2p_UserID;
			
			#if 1
			conback_netHead.nFlag = HDVS_FLAG;
			conback_netHead.nCommand = NETCMD_P2P_CMD;
			conback_netHead.nBufSize = sizeof(DVSNET_ENDCONBACK_INFO) + sizeof(COMMAND_HEAD);


			conback_netComm.nCmdID = NETCMD_P2P_ENDCONBACK;
			conback_netComm.nChannel = 0;
			conback_netComm.nCmdLen = sizeof(DVSNET_ENDCONBACK_INFO);

			memset(buffer, 0, 1024);
			memcpy(buffer, &conback_netHead, sizeof(NET_HEAD));
			memcpy(buffer + sizeof(NET_HEAD), &conback_netComm, sizeof(COMMAND_HEAD));
			memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &g_return_data,  sizeof(DVSNET_ENDCONBACK_INFO));
			conback_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_ENDCONBACK_INFO);
			#endif
			 
			
			ret = tcpSendMsg(hConnSock, &result_endconback, conback_send_lenght);
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", hConnSock);
				break;
			}
			 
			break;

		//PC端请求IPC打洞 PC---->P2P Server----->IPC	
		case NETCMD_P2P_REQHOLE:
			printf("CMD: NETCMD_P2P_REQHOLE\n");

			DVSNET_REQHOLE_INFO hole_info;
			DVSNET_ENDHOLE_INFO hole_end_info;
			NET_HEAD hole_netHead;
			NET_HEAD udp_hole_info;
			COMMAND_HEAD hole_net_comm;
			
			char hole_infor_return[1000];
			int  hole_send_lenght;
			int  clientSock;


			memcpy(&hole_info, &buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQHOLE_INFO));
			
			//打洞
			udp_hole_info.nFlag = HDVS_FLAG;
			udp_hole_info.nCommand = NETCMD_P2P_CMD;
			udp_hole_info.nBufSize = 0;
			udp_hole_info.nReserve = 0;
			printf("udp hole IP = %s	port = %ld\n", hole_info.strIp, hole_info.dwPort);
			clientSock = UdpSockListen(inet_addr(hole_info.strIp), hole_info.dwPort);
			ret = sendto(clientSock, &netHead, sizeof(NET_HEAD), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
			printf("make hole has handled!!\n");
			close(clientSock);
			
			#if 1
			hole_netHead.nFlag = HDVS_FLAG;
			hole_netHead.nCommand = NETCMD_P2P_CMD;
			hole_netHead.nBufSize = sizeof(DVSNET_ENDHOLE_INFO) + sizeof(COMMAND_HEAD);


			hole_net_comm.nCmdID = NETCMD_P2P_ENDHOLE;
			hole_net_comm.nChannel = 0;
			hole_net_comm.nCmdLen = sizeof(DVSNET_ENDHOLE_INFO);

			hole_end_info.dwReserved = 1;
			hole_end_info.dwUserID = g_p2p_UserID;

			memset(buffer, 0, 1024);
			memcpy(buffer, &hole_netHead, sizeof(NET_HEAD));
			memcpy(buffer + sizeof(NET_HEAD), &hole_net_comm, sizeof(COMMAND_HEAD));
			memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &g_return_data,  sizeof(DVSNET_ENDHOLE_INFO));
			hole_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_ENDHOLE_INFO);
			#endif
			
			ret = tcpSendMsg(hConnSock, buffer, hole_send_lenght);
			if (ret <= 0 && errno == EPIPE)
			{
				printf("NAT(%d): Send Error!\n", hConnSock);
				ret = -1;
			}
			printf("tcpSendMsg = %d\n", ret);
			break;

		default:
			printf("NAT(%d): CMD Error!\n", hConnSock);
			ret = -1;
 			break;
		}

	}
	return ret;
}
int remoteRequestProc_udp_remote(int sockfd, struct sockaddr_in sin)
{
	int hConnSock = -1;
	int ret = -1;
	int len = -1;
	int level;
	int nCurNum = 0;
	int sinlen;
	fd_set fset;
	struct timeval to;

	NET_HEAD netHead;
	COMMAND_HEAD p2p_net_comm;
	TALKTHRD_PARAM talkThrdPar;
	struct sockaddr_in addr;
	struct sockaddr_in udp_hole_addr;
	char buffer[UPD_RCV_BUFFER_LEN];

	if (sockfd <= 0 || sockfd > 65535)
	{
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	memset(&udp_hole_addr, 0, sizeof(udp_hole_addr));
	memset(&netHead, 0, sizeof(NET_HEAD));
	bzero(&to, sizeof(to));

	len = sizeof(addr);
	hConnSock = sockfd;

	g_tcp_nat_keepalive_count++;

	// Setup attribute of the connected socket
	SetConnSockAttr(hConnSock, SOCK_TIME_OUT);

	FD_ZERO(&fset);
	FD_SET(hConnSock, &fset);
	to.tv_sec = SOCK_TIME_OUT;
	to.tv_usec = 0;
while(1){
#if 0	
		printf("select = %d ....\n", ret);
		ret = select(hConnSock+1, &fset, NULL, NULL, &to);
		printf("select = %d ....\n", ret);
		if ( ret == 0)
		{
			net_debug();
			return 0;
		}
		if (ret<0 && errno==EINTR)
		{
			net_debug();
			return 0;
		}
		if (ret < 0)
		{
			printf("NAT(%d): Select Error!\n", hConnSock);
			net_debug();
			return -1;
		}
		if (!FD_ISSET(hConnSock, &fset))
		{
			printf("NAT(%d): FD_ISSET Error!\n", hConnSock);
			net_debug();
			return -1;
		}
#endif

		printf("waitting p2p server data\n");
		sinlen = sizeof(sin);
		ret = recvfrom(hConnSock,  &buffer, UPD_RCV_BUFFER_LEN , 0,
					(struct sockaddr*)&sin, &sinlen);
		if (ret <= 0)
		{
			printf("NAT(%d): Recv Error!\n", hConnSock);
			net_debug();
			return -1;
		}
		memcpy(&netHead, &buffer, sizeof(NET_HEAD));	
		if (netHead.nFlag != HDVS_FLAG)
		{
			printf("NAT(%d): Flag Error!\n", hConnSock);
			return -1;
		}
		printf("UDP P2P SERVER command = %p\n", netHead.nCommand);
		// process command of network
		switch (netHead.nCommand)
		{
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
					
					char hole_infor_return[1000];
					int  hole_send_lenght;
					int  clientSock;


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
					printf("make hole has handled!!\n");
					close(clientSock);

					/*tell p2p hole success!*/
					#if 1
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
					#endif

					printf("hole_send_lenght = %d\n %d %d %d %d\n",hole_send_lenght, sizeof(NET_HEAD), sizeof(COMMAND_HEAD),sizeof(DVSNET_ENDHOLE_INFO),hole_info.dwUserID);
					ret = udpSendMsg(sockfd, buffer, hole_send_lenght, sin);
					if (ret <= 0 && errno == EPIPE)
					{
						printf("NAT(%d): Send Error!\n", hConnSock);
						return -1;
					}
					printf("tcpSendMsg = %d\n", ret);
					break;
					
				case NETCMD_P2P_REQCONBACK:
					printf("CMD: NETCMD_P2P_REQCONBACK\n");
			    	//请求反向连接PC---->P2P Server------>IPC
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
					DVSNET_ENDCONBACK_INFO  result_endconback;
					#if 1
					result_endconback.dwReserved = 1;
					result_endconback.dwUserID = g_p2p_UserID;
					
					ret = udpSendMsg(hConnSock, &result_endconback, sizeof(DVSNET_ENDCONBACK_INFO), sin);
					if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
					{
						printf("NAT(%d): Keepalive Error!\n", hConnSock);
						break;
					}
					#endif	
					break;
					
					case NETCMD_P2P_REGDATAPORT:
						printf("CMD: NETCMD_P2P_REGDATAPORT\n");
						DVSNET_REGISTER_INFO p2p_register_return_info;
						memcpy(&p2p_register_return_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REGISTER_INFO));
						g_p2p_UserID = p2p_register_return_info.dwUserID;

						printf("g_p2p_UserID = %ld : %d\n\n\n", g_p2p_UserID, p2p_register_return_info.wLocalPort);
						break;
						
					case NETCMD_KEEP_ALIVE:
						printf("CMD: NETCMD_KEEP_ALIVE\n");
						// send keep alive packet
						netHead.nFlag = HDVS_FLAG;
						netHead.nCommand = NETCMD_KEEP_ALIVE;
						netHead.nBufSize = 0;
						ret = udpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD), sin);
						if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
						{
							printf("NAT(%d): Keepalive Error!\n", hConnSock);
							break;
						}
						printf("NETCMD_P2P_CMD over\n");
						break;					
							
					default:
							printf("undefined command\n");
							break;
								
			}
			
			break;
			
		case NETCMD_KEEP_ALIVE:
			printf("CMD: NETCMD_KEEP_ALIVE\n");
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			ret = udpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD), sin);
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", hConnSock);
				break;
			}
			printf("NETCMD_P2P_CMD over\n");
			break;
			
		default:
			printf("NAT(%d): CMD Error!\n", hConnSock);
			ret = -1;
			break;
		}
		
	}//while(1);

	return ret;
}
//mody by lv end add--------------------------------------------


int remoteRequestProc(int sockfd)
{
	int hConnSock = -1;
	int ret = -1;
	int len = -1;
	int level;
	int nCurNum = 0;

	fd_set fset;
	struct timeval to;

	NET_HEAD netHead;
	TALKTHRD_PARAM talkThrdPar;
	struct sockaddr_in addr;
	//mody by lv start add--------------------------------------------
	char buffer[1024];

	COMMAND_HEAD p2p_net_comm;
	struct sockaddr_in udp_hole_addr;
	int hole_sock;

	if (sockfd <= 0 || sockfd > 65535)
	{
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	memset(&netHead, 0, sizeof(NET_HEAD));
	bzero(&to, sizeof(to));

	len = sizeof(addr);
	hConnSock = sockfd;

	//g_tcp_nat_keepalive_count++;

	// Setup attribute of the connected socket
	SetConnSockAttr(hConnSock, SOCK_TIME_OUT);

	// 
	FD_ZERO(&fset);
	FD_SET(hConnSock, &fset);
	to.tv_sec = SOCK_TIME_OUT;
	to.tv_usec = 0;

while(1){
	#if 0
	ret = select(hConnSock+1, &fset, NULL, NULL, &to);
	if ( ret == 0)
	{
		net_debug();
		return 0;
	}
	if (ret<0 && errno==EINTR)
	{
		net_debug();
		return 0;
	}
	if (ret < 0)
	{
		printf("NAT(%d): Select Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	if (!FD_ISSET(hConnSock, &fset))
	{
		printf("NAT(%d): FD_ISSET Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	#endif
	printf("waitting for ....\n");
	// Receive network protocal header
	ret = recv(hConnSock, &buffer, sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_REGISTER_INFO) , 0);
	if (ret <= 0)
	{
		printf("NAT(%d): Recv Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	memcpy(&netHead, &buffer, sizeof(NET_HEAD));	
	
	// Check the flag of network packet
	if (netHead.nFlag != HDVS_FLAG)
	{
		printf("NAT(%d): Flag Error!\n", hConnSock);
		return -1;
	}
	//printf("netHead.nCommand = %d\n", netHead.nCommand);
	
	// process command of network
	switch (netHead.nCommand){
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
					
					char hole_infor_return[1000];
					int  hole_send_lenght;
					int  clientSock;


					memcpy(&hole_info, &buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQHOLE_INFO));
					
					//打洞
					udp_hole_info.nFlag = HDVS_FLAG;
					udp_hole_info.nCommand = NETCMD_P2P_CMD;
					udp_hole_info.nBufSize = 0;
					udp_hole_info.nReserve = 0;
					printf("udp hole IP = %s	port = %ld\n", hole_info.strIp, hole_info.dwPort);

					#if 0	
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

					ret = natConnect(hole_info.strIp, hole_info.dwPort, &hole_sock);
					printf("natConnect(%s %d %d): %d\n", hole_info.strIp, hole_info.dwPort, hole_sock, ret);
					if (ret < 0)
					{
						return -1;
					}

					tcpSendMsg(hole_sock, &netHead, sizeof(NET_HEAD));
					printf("make hole has handled!!\n");
					close(hole_sock);

					/*tell p2p hole success!*/
					#if 1
					hole_netHead.nFlag = HDVS_FLAG;
					hole_netHead.nCommand = NETCMD_P2P_CMD;
					hole_netHead.nBufSize = sizeof(DVSNET_ENDHOLE_INFO) + sizeof(COMMAND_HEAD);


					hole_net_comm.nCmdID = NETCMD_P2P_ENDHOLE;
					hole_net_comm.nChannel = 0;
					hole_net_comm.nCmdLen = sizeof(DVSNET_ENDHOLE_INFO);

					hole_end_info.dwReserved = 1;
					hole_end_info.dwUserID = g_p2p_UserID;

					memset(buffer, 0, 1024);
					memcpy(buffer, &hole_netHead, sizeof(NET_HEAD));
					memcpy(buffer + sizeof(NET_HEAD), &hole_net_comm, sizeof(COMMAND_HEAD));
					memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &g_return_data,  sizeof(DVSNET_ENDHOLE_INFO));
					hole_send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_ENDHOLE_INFO);
					#endif
					
					ret = tcpSendMsg(hole_sock, buffer, hole_send_lenght);
					if (ret <= 0 && errno == EPIPE)
					{
						printf("NAT(%d): Send Error!\n", hole_sock);
						return -1;
					}
					printf("tcpSendMsg = %d\n", ret);
					break;
					
				case NETCMD_P2P_REQCONBACK:
					printf("CMD: NETCMD_P2P_REQCONBACK\n");
			    	//请求反向连接PC---->P2P Server------>IPC
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
					DVSNET_ENDCONBACK_INFO  result_endconback;
					#if 1
					result_endconback.dwReserved = 1;
					result_endconback.dwUserID = g_p2p_UserID;
					
					ret = tcpSendMsg(hConnSock, &result_endconback, sizeof(DVSNET_ENDCONBACK_INFO));
					if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
					{
						printf("NAT(%d): Keepalive Error!\n", hConnSock);
						break;
					}
					#endif	
					break;
					
					case NETCMD_P2P_REGDATAPORT:
						printf("CMD: NETCMD_P2P_REGDATAPORT\n");
						DVSNET_REGISTER_INFO p2p_register_return_info;
						memcpy(&p2p_register_return_info, buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REGISTER_INFO));
						g_p2p_UserID = p2p_register_return_info.dwUserID;
						
						//printf("CMD: NETCMD_KEEP_ALIVE\n");
						// send keep alive packet
						netHead.nFlag = HDVS_FLAG;
						netHead.nCommand = NETCMD_KEEP_ALIVE;
						netHead.nBufSize = 0;
						ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
						if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
						{
							printf("NAT(%d): Keepalive Error!\n", hConnSock);
							break;
						}
						//printf("NETCMD_P2P_CMD over\n");

						printf("g_p2p_UserID = %ld : %d\n\n\n", g_p2p_UserID, p2p_register_return_info.wLocalPort);
						break;
						
					case NETCMD_KEEP_ALIVE:
					//	printf("CMD: NETCMD_KEEP_ALIVE\n");
						// send keep alive packet
						netHead.nFlag = HDVS_FLAG;
						netHead.nCommand = NETCMD_KEEP_ALIVE;
						netHead.nBufSize = 0;
						ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
						if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
						{
							printf("NAT(%d): Keepalive Error!\n", hConnSock);
							break;
						}
						//printf("NETCMD_P2P_CMD over\n");
						break;					
							
					default:
							printf("undefined command\n");
							break;
								
			}
			
			break;
			
		case NETCMD_KEEP_ALIVE:
			printf("CMD: NETCMD_KEEP_ALIVE\n");
			netHead.nFlag = HDVS_FLAG;
			netHead.nCommand = NETCMD_KEEP_ALIVE;
			netHead.nBufSize = 0;
			ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
			if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
			{
				printf("NAT(%d): Keepalive Error!\n", hConnSock);
				break;
			}
			printf("NETCMD_P2P_CMD over\n");
			break;
			
		default:
			printf("NAT(%d): CMD Error!\n", hConnSock);
			ret = -1;
			break;
		}
	

}//while(1);

	return ret;
	
Error:
	net_debug();
	shutdown(hConnSock, 2);
	close(hConnSock);
	return -1;
}
#if 0
{
	int hConnSock = -1;
	int ret = -1;
	int len = -1;
	int level;
	int nCurNum = 0;

	fd_set fset;
	struct timeval to;

	NET_HEAD netHead;
	TALKTHRD_PARAM talkThrdPar;
	struct sockaddr_in addr;
	char buffer[1024];
//mody by lv end add--------------------------------------------


	if (sockfd <= 0 || sockfd > 65535)
	{
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	memset(&netHead, 0, sizeof(NET_HEAD));
	bzero(&to, sizeof(to));

	len = sizeof(addr);
	hConnSock = sockfd;

	g_tcp_nat_keepalive_count++;
	
	// Setup attribute of the connected socket
	SetConnSockAttr(hConnSock, SOCK_TIME_OUT);

	// 
	FD_ZERO(&fset);
	FD_SET(hConnSock, &fset);
	to.tv_sec = SOCK_TIME_OUT;
	to.tv_usec = 0;

while(1){    //mbl add
	ret = select(hConnSock+1, &fset, NULL, NULL, &to);
	if ( ret == 0)
	{
		net_debug();
		return 0;
	}
	if (ret<0 && errno==EINTR)
	{
		net_debug();
		return 0;
	}
	if (ret < 0)
	{
		printf("NAT(%d): Select Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	if (!FD_ISSET(hConnSock, &fset))
	{
		printf("NAT(%d): FD_ISSET Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	
	// Receive network protocal header
	ret = recv(hConnSock, &buffer, sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_REGISTER_INFO) , 0);
	if (ret <= 0)
	{
		printf("NAT(%d): Recv Error!\n", hConnSock);
		net_debug();
		return -1;
	}
	memcpy(&netHead, &buffer, sizeof(NET_HEAD));	
	// Check the flag of network packet
	if (netHead.nFlag != HDVS_FLAG)
	{
		printf("NAT(%d): Flag Error!\n", hConnSock);
		return -1;
	}

	//printf("netHead.nCommand = %d\n", netHead.nCommand);
	
	// process command of network
	switch (netHead.nCommand)
	{
	case NETCMD_LOGON:	// logon
		{
			int nCurNum = 0;
			int nRight = 0;		
			int nClientID = 0;                
			NET_USER_INFO userInfo;                

			printf("NETCMD_LOGON_R ... \n");

			memset(&userInfo, 0, sizeof(userInfo));
				
			// check the length of data
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

                printf("NETCMD_LOGON_R(%d): NETERR_ILLEGAL_PARAM\n", hConnSock);

				return -1;
			}								
				
			// Receive user information
			ret = recv(hConnSock, &userInfo, sizeof(userInfo), 0);
			if (ret <= 0)
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
                	
				return -1;
			}

			// Check logon number
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

				printf("NETCMD_LOGON_R(%d): NETERR_LOGON_FULL\n", hConnSock);

				return -1;
			}

			// Check user information
			if (g_server_info.funcCheckUserPsw)
			{
				MSG_HEAD msgHead;

				msgHead.nCmd = NETCMD_LOGON;
				msgHead.nSock = hConnSock;
                    
				ret = g_server_info.funcCheckUserPsw(&msgHead, userInfo.szUserName, userInfo.szUserPsw);
				if (ret < 0)
				{
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
                	
					printf("NETCMD_LOGON_R(%d): NETERR_USER_PSW\n", hConnSock);

					return -1;
				}
				nRight = ret;

			}

			// logon
			ret = ClientLogon(addr, userInfo, hConnSock, nRight);
			if (ret < 0)
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
				                
				printf("NETCMD_LOGON_R(%d): ClientLogon failed\n", hConnSock);
	        		
				return -1;
			}
        	
			// Notify the data send thread
			pthread_mutex_lock(&g_server_info.msgQuitThreadMutex);
			g_server_info.msgWaitThreadNum ++;
			pthread_mutex_unlock(&g_server_info.msgQuitThreadMutex);
			pthread_cond_signal(&g_server_info.msgThreadCond);
			
			printf("NETCMD_LOGON_R OK(%d)\n", hConnSock);
			
			return 0;
		}
		break;
		
	// Add the code by lvjh, 2010-01-14
	case NETCMD_OPEN_TALK:
		{
			pthread_t thrdID;
			TALKTHRD_PARAM talkThrdParam;
			NET_DATA netData;
			TALK_PARAM *talkParam=NULL;
				
			printf("NETCMD_OPEN_TALK: open %d (%s %d)\n", hConnSock, __FILE__, __LINE__);

		   //打开AOUT设备 add code by by liujw

		   ret = audioDecModuleOpen(3);  //add code by liujw
			printf("audioOutOpen\n");
			if (ret)	
			{   
				printf("audioOutOpen(%s %d) Failed!\n", __FILE__, __LINE__);
				audioDecModuleClose();
				//return -1;	
			}

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

			talkParam->nEncType = g_server_info.avInfoStream1[0].nAudioEncType;
			talkParam->nAinChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
			talkParam->nAinBits = 16;
			talkParam->nAinSamples = g_server_info.avInfoStream1[0].nAudioSamples;
			talkParam->nDecType = g_server_info.avInfoStream1[0].nAudioEncType;
			talkParam->nAoutChnNum = g_server_info.avInfoStream1[0].nAudioChannels;
			talkParam->nAoutBits = 16;
			talkParam->nAoutSamples = g_server_info.avInfoStream1[0].nAudioSamples;
					
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
//mody by lv start add--------------------------------------------
	case NETCMD_P2P_CMD:  
		printf("CMD: NETCMD_P2P_CMD\n");
		#if 1
		// send keep alive packet
		netHead.nFlag = HDVS_FLAG;
		netHead.nCommand = NETCMD_KEEP_ALIVE;
		netHead.nBufSize = 0;
		
		ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
		if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
		{
			printf("NAT(%d): Keepalive Error!\n", hConnSock);
			break;
		}
		#endif

		break;
	case NETCMD_KEEP_ALIVE:
		printf("CMD: NETCMD_KEEP_ALIVE\n");
	#if 1
		// send keep alive packet
		netHead.nFlag = HDVS_FLAG;
		netHead.nCommand = NETCMD_KEEP_ALIVE;
		netHead.nBufSize = 0;
		
		ret = tcpSendMsg(hConnSock, &netHead, sizeof(NET_HEAD));
		if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
		{
			printf("NAT(%d): Keepalive Error!\n", hConnSock);
			break;
		}
	#endif
		break;
		
	case NETCMD_P2P_REQCONBACK:
		printf("CMD: NETCMD_P2P_REQCONBACK\n");
	   //请求反向连接PC---->P2P Server------>IPC
		#if 1
		//反向连接的代码
		DVSNET_REQCONBACK_INFO conback_info;
		char buffer_return[1000];
		SYS_INFO sysInfo;
		NET_PARAM netParam;
		memcpy(&conback_info, &buffer+sizeof(NET_HEAD)+sizeof(COMMAND_HEAD),sizeof(DVSNET_REQCONBACK_INFO));
	
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
		DVSNET_ENDCONBACK_INFO  result_endconback;
		#if 1
		result_endconback.dwReserved = 1;
		result_endconback.dwUserID = 201206011030;
		
		ret = tcpSendMsg(hConnSock, &result_endconback, sizeof(DVSNET_ENDCONBACK_INFO));
		if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
		{
			printf("NAT(%d): Keepalive Error!\n", hConnSock);
			break;
		}
		#endif	
		break;
//mody by lv end add--------------------------------------------
		
	default:
		printf("NAT(%d): CMD Error!\n", hConnSock);
		ret = -1;
		break;
	}

}//while(1);

	return ret;
}
#endif


int natOpenChannel(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int hConnSock = -1; 
	int sock = -1;
	int nCurNum = 0;
	OPEN_CHANNEL openChannel;
	CLIENT_INFO clientInfo;

	NET_HEAD netHead;

	memset(&clientInfo, 0, sizeof(CLIENT_INFO));
	memset(&openChannel, 0, sizeof(OPEN_CHANNEL));

	if (pMsgHead==NULL || pRecvBuf==NULL)
	{
		return -1;
	}

	hConnSock = pMsgHead->nSock;
	if (hConnSock <= 0)
	{
		return -1;
	}
	memcpy(&openChannel, pRecvBuf, sizeof(OPEN_CHANNEL));

	printf("NETCMD_OPEN_CHANNEL_R ...\n");

	// Check whether the correct channel parameter
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

		printf("NETCMD_OPEN_CHANNEL_R: NETERR_ILLEGAL_PARAM!\n");

		return -1;
	}

	// Connect with reverse
	ret = natConnect(g_remote_ip, g_remote_port, &sock);
	if (ret < 0)
	{
		printf("NETCMD_OPEN_CHANNEL_R: natConnect failed!\n");
		return -1;
	}
	else
	{
		char buf[1024];
		netHead.nFlag = HDVS_FLAG;
		netHead.nCommand = NETCMD_OPEN_CHANNEL_R;
		netHead.nErrorCode = 0;
		netHead.nBufSize = g_return_data_len+sizeof(OPEN_CHANNEL);
		netHead.nReserve = 0;
		memcpy(buf, &netHead, sizeof(NET_HEAD));
		memcpy(buf+sizeof(NET_HEAD), g_return_data, g_return_data_len);
		memcpy(buf+sizeof(NET_HEAD)+g_return_data_len, &openChannel, sizeof(OPEN_CHANNEL));

		ret = send(sock, buf, sizeof(NET_HEAD)+g_return_data_len+sizeof(OPEN_CHANNEL), 0);
		if (ret < 0)
		{
			net_debug();
			
			printf("NETCMD_OPEN_CHANNEL_R(%d): Send Error!\n", sock); 
		
			goto Error;
		}
	}

	// Check whether the specific user exists
	if (!GetClient(hConnSock, &clientInfo))
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
			
		printf("NETCMD_OPEN_CHANNEL_R(%d): NETERR_NOT_LOGON!!\n", sock);      	
			
		goto Error;
	}

	// Check whether the user number exceed the max number
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
		
		printf("NETCMD_OPEN_CHANNEL_R(%d): NETERR_CHANNEL_FULL!!!\n", sock);    	
		
		goto Error;

	}             

	// Request send data
	ret = RequestTcpPlay(openChannel, sock);
	if (ret < 0)
	{
		printf("NETCMD_OPEN_CHANNEL_R(%d): RequestTcpPlay Failed\n", sock);
			
		goto Error;
	}

	// Notify the send data thread
	pthread_mutex_lock(&g_server_info.dataQuitThreadMutex);
	g_server_info.dataWaitThreadNum++;
	pthread_mutex_unlock(&g_server_info.dataQuitThreadMutex);
	pthread_cond_signal(&g_server_info.dataThreadCond); 

	printf("NETCMD_OPEN_CHANNEL_R OK(%d)\n", sock);

	return 0;
	
Error:
	shutdown(sock, 2);
	close(sock);
	
	return -1;
}


//mody by lv start add--------------------------------------------
int p2p_udp_Connect(const char*pSerIP, int inPort, int *sock, struct sockaddr_in *toAddr)
{
	struct sockaddr_in addr;
	int hSocket = 0;

	if (NULL == pSerIP)
	{
		return -1;
	}

	if ((inPort < 1000) || (inPort > 65535))
	{
		return -1;
	}

	hSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(hSocket < 0)
	{
		 printf("create sock erroe!!\r\n");
		 return -1;
	}
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(pSerIP);
	addr.sin_port = htons(inPort);

	*sock = hSocket;
	*toAddr = addr;

	return 0;
}
//mody by lv end add--------------------------------------------

	//mody by lv start add--------------------------------------------
int nat_udp_Connect(const char*pSerIP, int inPort, int *sock, struct sockaddr_in *addr)
{
	int nRet = 0;
	int hSocket = 0;
	int nOpt = 1;	
	int error;
	struct hostent *host = NULL;

	//struct sockaddr_in addr;
	unsigned long ulServer;

	unsigned long non_blocking = 1;
	unsigned long blocking = 0;
	
	if (NULL == pSerIP)
	{
		return -1;
	}

	if ((inPort < 1000) || (inPort > 65535))
	{
		return -1;
	}
#if 1
//	struct sockaddr_in toAddr;
	*sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(*sock < 0)
	{
	 printf("create socket error.\r\n");
	 exit(1);
	}
	memset(addr,0,sizeof(struct sockaddr_in));
	addr->sin_family=AF_INET;
	addr->sin_addr.s_addr=inet_addr(pSerIP);
	addr->sin_port = htons(inPort);

	printf("create socket successs!\n");
#endif


#if 0	

	nOpt = 1;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOpt, sizeof(nOpt));
	
	nOpt = 1;
	nRet = setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&nOpt, sizeof(nOpt));	
	
	/*
	nOpt = 2000;  
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&nOpt, sizeof(nOpt));	
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOpt, sizeof(nOpt));
	*/


	nOpt = SOCK_SNDRCV_LEN;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nOpt, sizeof(nOpt));

	nOpt = SOCK_SNDRCV_LEN;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nOpt, sizeof(nOpt));
#endif

#if 0	
	memset(&addr, 0 ,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)inPort);
	//addr.sin_addr.s_addr = ulServer;
	addr.sin_addr = *((struct in_addr *)host->h_addr);
#endif



#if 0
	//nRet = connect(hSocket, (struct sockaddr*)&addr, sizeof(addr));
	nRet = bind(hSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)); 
	if (nRet == -1 && errno == EINPROGRESS)
	{
		struct timeval tv; 
		fd_set writefds;

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&writefds); 
		FD_SET(hSocket, &writefds); 

		if (select(hSocket+1, NULL, &writefds, NULL, &tv) != 0)
		{ 
			if (FD_ISSET(hSocket,&writefds))
			{
				int len=sizeof(error); 
				if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0)
				{
					goto error_ret; 
				}

				if (error != 0) 
				{
					goto error_ret; 
				}
			}
			else
			{
				goto error_ret;
			}
		}
		else 
		{
			goto error_ret;
		}

		blocking = 0;
		ioctl(hSocket, FIONBIO, &blocking);
		nRet = 0;
	}

#endif

	if (nRet != 0)
	{
		goto error_ret;
	}
	blocking = 0;
	ioctl(hSocket, FIONBIO, &blocking);

	*sock = hSocket;

	return 0;

error_ret:
	printf("NAT: Close Socket %d\n", hSocket);
	net_debug();
	shutdown(hSocket, 2);
	close(hSocket);
	
	return -1;
}
	//mody by lv end add--------------------------------------------
int udp_natConnect(const char*pSerIP, int inPort, int *sock, struct sockaddr_in *addr)
{
	int nRet = 0;
	int hSocket = 0;
	int nOpt = 1;	
	int error;
	struct hostent *host = NULL;
	//struct sockaddr_in addr;
	unsigned long ulServer;

	unsigned long non_blocking = 1;
	unsigned long blocking = 0;
	
	if (NULL == pSerIP)
	{
		return -1;
	}

	if ((inPort < 1000) || (inPort > 65535))
	{
		return -1;
	}

	//ulServer = inet_addr(pSerIP);
	if ((host=gethostbyname(pSerIP)) == NULL) 
	{
		return -1;
	}
		
	hSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hSocket <= 0)
	{
		return -1;
	}	

	nOpt = 1;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOpt, sizeof(nOpt));
	
	nOpt = 1;
	nRet = setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&nOpt, sizeof(nOpt));	
	
	/*
	nOpt = 2000;  
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&nOpt, sizeof(nOpt));	
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOpt, sizeof(nOpt));
	*/
	
	nOpt = SOCK_SNDRCV_LEN;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nOpt, sizeof(nOpt));

	nOpt = SOCK_SNDRCV_LEN;
	nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nOpt, sizeof(nOpt));
	
	memset(&addr, 0 ,sizeof(addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons((u_short)inPort);
	//addr.sin_addr.s_addr = ulServer;
	addr->sin_addr = *((struct in_addr *)host->h_addr);

	non_blocking = 1;
	ioctl(hSocket, FIONBIO, &non_blocking);

	nRet = connect(hSocket, (struct sockaddr*)&addr, sizeof(addr));
	if (nRet == -1 && errno == EINPROGRESS)
	{
		struct timeval tv; 
		fd_set writefds;

		tv.tv_sec = 10;
		tv.tv_usec = 0;

		FD_ZERO(&writefds); 
		FD_SET(hSocket, &writefds); 

		if (select(hSocket+1, NULL, &writefds, NULL, &tv) != 0)
		{ 
			if (FD_ISSET(hSocket,&writefds))
			{
				int len=sizeof(error); 
				if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0)
				{
					goto error_ret; 
				}

				if (error != 0) 
				{
					goto error_ret; 
				}
			}
			else
			{
				goto error_ret;
			}
		}
		else 
		{
			goto error_ret;
		}

		blocking = 0;
		ioctl(hSocket, FIONBIO, &blocking);
		nRet = 0;
	}

	if (nRet != 0)
	{
		goto error_ret;
	}
	blocking = 0;
	ioctl(hSocket, FIONBIO, &blocking);

	*sock = hSocket;

	return 0;

error_ret:
	printf("NAT: Close Socket %d\n", hSocket);
	net_debug();
	shutdown(hSocket, 2);
	close(hSocket);
	
	return -1;
}


//mody by lv start add--------------------------------------------
int natConnect(const char*pSerIP, int inPort, int *sock)
{
		int nRet = 0;
		int hSocket = 0;
		int nOpt = 1;	
		int error;
		struct hostent *host = NULL;
		struct sockaddr_in addr;
		unsigned long ulServer;
	
		unsigned long non_blocking = 1;
		unsigned long blocking = 0;
	
#if 1
		if (NULL == pSerIP)
		{
			return -1;
		}
	
		if ((inPort < 1000) || (inPort > 65535))
		{
			return -1;
		}
#endif
	
	
		//ulServer = inet_addr(pSerIP);
		if ((host=gethostbyname(pSerIP)) == NULL) 
		{
			return -1;
		}
			
		hSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (hSocket <= 0)
		{
			return -1;
		}	
	
#if 1
		nOpt = 1;
		nRet = setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&nOpt, sizeof(nOpt));
		
		nOpt = 1;
		nRet = setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&nOpt, sizeof(nOpt));	
		
		/*
		nOpt = 2000;  
		nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&nOpt, sizeof(nOpt));	
		nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&nOpt, sizeof(nOpt));
		*/
		
		nOpt = SOCK_SNDRCV_LEN;
		nRet = setsockopt(hSocket, SOL_SOCKET, SO_SNDBUF, (char *)&nOpt, sizeof(nOpt));
	
		nOpt = SOCK_SNDRCV_LEN;
		nRet = setsockopt(hSocket, SOL_SOCKET, SO_RCVBUF, (char *)&nOpt, sizeof(nOpt));
#endif
	
		memset(&addr, 0 ,sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons((u_short)inPort);
		//addr.sin_addr.s_addr = ulServer;
		addr.sin_addr = *((struct in_addr *)host->h_addr);
	
		non_blocking = 1;
		ioctl(hSocket, FIONBIO, &non_blocking);
	//	printf("connecting ...........\n");
		nRet = connect(hSocket, (struct sockaddr*)&addr, sizeof(addr));
		if (nRet == -1)
		{
			struct timeval tv; 
			fd_set writefds;
	
			tv.tv_sec = 10;
			tv.tv_usec = 0;
	
			FD_ZERO(&writefds); 
			FD_SET(hSocket, &writefds); 
	
			if (select(hSocket+1, NULL, &writefds, NULL, &tv) != 0)
			{ 
				if (FD_ISSET(hSocket,&writefds))
				{
					int len=sizeof(error); 
					if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0)
					{
						goto error_ret; 
					}
	
					if (error != 0) 
					{
						goto error_ret; 
					}
				}
				else
				{
					goto error_ret;
				}
			}
			else 
			{
				goto error_ret;
			}
	
			blocking = 0;
			ioctl(hSocket, FIONBIO, &blocking);
			nRet = 0;
		}
	
		if (nRet != 0)
		{
			goto error_ret;
		}
		blocking = 0;
		ioctl(hSocket, FIONBIO, &blocking);
	
		*sock = hSocket;
	
		return 0;
	
	error_ret:
		printf("NAT: Close Socket %d\n", hSocket);
		net_debug();
		shutdown(hSocket, 2);
		close(hSocket);
		
		return -1;
}
//mody by lv end add--------------------------------------------


int Send(int hSer, char *pBuf, int nLen)
{
	int ret = 0;
	int sendsize = 0;
	
	while (sendsize < nLen)
	{
		ret = send(hSer, pBuf+sendsize, nLen-sendsize, 0);
		if (ret < 1)
		{
			return ret;
		}
		sendsize = sendsize + ret;
	}
	
	return sendsize;
}
//mody by lv start add--------------------------------------------
int Sendto(int hSer, char *pBuf, int nLen, struct sockaddr_in addr)
{
	int ret = 0;
	int sendsize = 0;
	//struct sockaddr_in receiver_addr;
	
	while (sendsize < nLen)
	{
		ret = sendto(hSer, pBuf+sendsize, nLen-sendsize, 0,(struct sockaddr*)&addr,sizeof(addr));
		if (ret < 1)
		{
			return ret;
		}
		sendsize = sendsize + ret;
	}
	
	return sendsize;
}
//mody by lv end add--------------------------------------------

int tcpSendMsg(int hSock, void *data, int len)
{
	int ret = -1;

	if (hSock <= 0)
	{
		return -1;
	}
	if (NULL == data || len <= 0)
	{
		return -1;
	}
	//printf("send data = %s\n", data);
	ret = Send(hSock, data, len);
	if (ret <= 0)
	{
		return -1;
	}
	return ret;

}


//mody by lv start add--------------------------------------------
int udpSendMsg(int hSock, void *data, int len, struct sockaddr_in addr)
{
	int ret = -1;

	if (hSock <= 0)
	{
		return -1;
	}
	if (NULL == data || len <= 0)
	{
		return -1;
	}
	//printf("---track  udp printf---5==\n");
	ret = sendto(hSock, data, len, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
	if (ret <= 0)
	{
		printf("send to error = %d!\n", len);
		return -1;
	}
	//printf("---track  udp printf---6==\n");

	return ret;
}

int TcpSendMsgToAll_UDPremote(char *remoteIP, int remotePort, int cmdType)
{
	int ret = 0;
	int sock = 0;
	char buffer[1024];
	char upnpCmd[256];
	struct sockaddr_in toAddr;

	DVSNET_REGISTER_INFO  p2p_register_info;
	NET_HEAD netHead;
	COMMAND_HEAD p2p_net_comm;
	int send_lenght;

	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}
	
	printf("TcpSendMsgToAll_UDPremote\n");
#if 1
	
	sock = g_server_info.hUdpListenSock;
	

#if 0
	sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock < 0)
	{
		 printf("create sock erroe!!\r\n");
		 goto Error;
	}
#endif

	memset(&toAddr,0,sizeof(toAddr));
	toAddr.sin_family=AF_INET;
	toAddr.sin_addr.s_addr=inet_addr(remoteIP);
	toAddr.sin_port = htons(remotePort);
#endif

	#if 1
	NET_PARAM net_param;
	SYS_INFO     P2P_sys_info;
	DVSNET_UPNP_PARAM upnp_param;
	int upnp_flags  = 0 ;
	int upnp_port = 0;
	int g_upnp_flag = 0;

	getNewUpnpParam(&upnp_param);
	if(g_upnp_flag == 0){
		getNetParam(&net_param);
		memset(upnpCmd, 0, 256);
		g_upnp_flag = 1;
		#if 0
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, upnp_port);
		if(system(upnpCmd) == -1){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_flags = 0;
			upnp_param.nReserved = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, upnp_port+1);
		if(system(upnpCmd) == -1){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_flags = 0;
			upnp_param.nReserved = 0;
		}
		#endif
		upnp_port  = cmdType;
	//printf("---track  udp printf---3==\n");
	#if 0
		printf("upnp_port  = %d\n", upnp_port );
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wWebPort, upnp_port);
		if(system(upnpCmd) !=  0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_flags = 0;
			g_upnp_flag = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wServerPort, upnp_port+1);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_flags = 0;
			g_upnp_flag = 0;
		}
	#endif
	//printf("---track  udp printf---4==\n");
	}
	getNetParam(&net_param);
	getSysInfoParam(&P2P_sys_info);

	memset(&p2p_register_info, 0, sizeof(DVSNET_REGISTER_INFO));
	strcpy(p2p_register_info.strLocalIp, net_param.byServerIp);
	strcpy(p2p_register_info.strRemoteIp, REMOTE_IP);
	p2p_register_info.wLocalPort = net_param.wServerPort;
	p2p_register_info.wRemotePort = REMOTE_PORT;
	p2p_register_info.dwCount = 1;
	p2p_register_info.dwDeviceType = 5;
	p2p_register_info.dwUpnp = g_upnp_flag;
	p2p_register_info.dwUpnpWebPort =  upnp_port;
	p2p_register_info.dwUpnpDataPort =  upnp_port+1;
	p2p_register_info.dwUserID = 0;
	
	strcpy(p2p_register_info.strDeviceName, P2P_sys_info.strDeviceName);
	getIDConfigure(p2p_register_info.strSerialNo);
	#if 0
	printf("P2P_sys_info.strDeviceName = %s\n", P2P_sys_info.strDeviceName);
	printf("p2p_register_info.wRemotePort = %d\n", p2p_register_info.wRemotePort);
	printf("p2p_register_info.strRemoteIp = %s\n", p2p_register_info.strRemoteIp);
	#endif
	strcpy(p2p_register_info.strUser, REMOTE_USER);
	strcpy(p2p_register_info.strPasswd, REMOTE_PASSWD);
#endif

	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_P2P_CMD;
	netHead.nBufSize = sizeof(DVSNET_REGISTER_INFO) + sizeof(COMMAND_HEAD);

	p2p_net_comm.nCmdID = NETCMD_P2P_REGDATAPORT;
	p2p_net_comm.nChannel = 0;
	p2p_net_comm.nCmdLen = sizeof(DVSNET_REGISTER_INFO);

	memset(buffer, 0, 1024);
	memcpy(buffer, &netHead, sizeof(NET_HEAD));
	memcpy(buffer + sizeof(NET_HEAD), &p2p_net_comm, sizeof(COMMAND_HEAD));
	memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &p2p_register_info,  sizeof(DVSNET_REGISTER_INFO));
	send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_REGISTER_INFO);

	ret = udpSendMsg(sock, buffer, send_lenght, toAddr);
#if 0
	if (remoteRequestProc_udp_remote(sock, toAddr) < 0)
	{
		printf("NAT(%d): RemoteRequestProc Error!\n", sock);
		goto Error;
	}
#endif
	return ret;
Error:
	net_debug();
	shutdown(sock, 2);
	close(sock);
	return -1;
}


int TcpSendMsgToAll_remote(char *remoteIP, int remotePort, int cmdType)
{
	int ret = 0;
	int sock = 0;
	char buffer[1024];
	NET_HEAD *netHead;
	char *returnData;

	SYS_INFO sysInfo;
	NET_PARAM netParam;

	printf("TcpSendMsgToAll_remote(%s %d %d): ...\n", remoteIP, remotePort, sock);
	
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}

	NETSDK_NatPause();
	getSysInfoParam(&sysInfo);
	getNetParam(&netParam);
	netHead->nFlag = HDVS_FLAG;
	netHead->nCommand = cmdType;
	netHead->nBufSize = sizeof(SYS_INFO)+sizeof(NET_PARAM);

	// Connect with reverse
	ret = natConnect(remoteIP, remotePort, &sock);
	printf("natConnect(%s %d %d): %d\n", remoteIP, remotePort, sock, ret);
	if (ret < 0)
	{
		return -1;
	}
	memcpy(returnData, netHead, sizeof(NET_HEAD));
	memcpy(returnData+sizeof(NET_HEAD), &sysInfo, sizeof(SYS_INFO));
	memcpy(returnData+sizeof(NET_HEAD)+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
	
	ret = tcpSendMsg(sock, returnData, sizeof(NET_HEAD)+sizeof(SYS_INFO)+sizeof(NET_PARAM));
	if (ret <= 0 && errno == EPIPE)
	{
		printf("NAT(%d): Send Error!\n", sock);
		goto Error;
	}
	printf("SEND = %d\n", ret);
	
	if (remoteRequestProc_remote(sock) < 0)
	{
		printf("NAT(%d): RemoteRequestProc Error!\n", sock);
		goto Error;
	}
	return 0;
	
Error:
	net_debug();
	shutdown(sock, 2);
	close(sock);
	return -1;
}
//mody by lv end add--------------------------------------------

int TcpSendMsgToAll(char *remoteIP, int remotePort, int cmdType)
#if 1
{
	int ret = 0;
	int sock = 0;
	char buffer[1024];
	char upnpCmd[256];
	NET_HEAD netHead;
	char *returnData;
	struct sockaddr_in addr;

	#if 1
	DVSNET_REGISTER_INFO  p2p_register_info;
	DVSNET_P2P_PARAM p2p_IP_param;
	COMMAND_HEAD p2p_net_comm;
	SYS_INFO     P2P_sys_info;
	int send_lenght;
	#endif
	

	printf("natConnect(%s %d %d): ...\n", remoteIP, remotePort, sock);
	
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}

	
	// Connect with reverse
	ret = natConnect(remoteIP, remotePort, &sock);
	printf("natConnect(%s %d %d): %d\n", remoteIP, remotePort, sock, ret);
	if (ret < 0)
	{
		return -1;
	}

	//mody by lv start add--------------------------------------------	
	#if 1
		NET_PARAM net_param;
		DVSNET_UPNP_PARAM upnp_param;
		int upnp_flags	= 0 ;
	
		getNewUpnpParam(&upnp_param);
		if(upnp_param.nReserved == 0){
			getNetParam(&net_param);
			memset(upnpCmd, 0, 256);
			upnp_param.nReserved = 1;
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, UPNP_MAPPING_PROT);
			if(system(upnpCmd) == -1){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_flags = 0;
				upnp_param.nReserved = 0;
			}
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, UPNP_MAPPING_PROT+1);
			if(system(upnpCmd) == -1){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_flags = 0;
				upnp_param.nReserved = 0;
			}
		}
		getNetParam(&net_param);
		getSysInfoParam(&P2P_sys_info);
		
		strcpy(p2p_register_info.strLocalIp, net_param.byServerIp);
		strcpy(p2p_register_info.strRemoteIp, REMOTE_IP);
		p2p_register_info.wLocalPort = net_param.wServerPort;
		p2p_register_info.wRemotePort = REMOTE_PORT;
		p2p_register_info.dwCount = 1;
		p2p_register_info.dwDeviceType = 5;
		p2p_register_info.dwUpnp = upnp_param.nReserved;
		p2p_register_info.dwUpnpWebPort =  UPNP_MAPPING_PROT;
		p2p_register_info.dwUpnpDataPort =	UPNP_MAPPING_PROT+1;
		
		strcpy(p2p_register_info.strDeviceName, P2P_sys_info.strDeviceName);
		getIDConfigure(p2p_register_info.strSerialNo);
	#if 0
		printf("P2P_sys_info.strDeviceName = %s\n", P2P_sys_info.strDeviceName);
		printf("p2p_register_info.wRemotePort = %d\n", p2p_register_info.wRemotePort);
		printf("p2p_register_info.strRemoteIp = %s\n", p2p_register_info.strRemoteIp);
	#endif
		strcpy(p2p_register_info.strUser, REMOTE_USER);
		strcpy(p2p_register_info.strPasswd, REMOTE_PASSWD);
#endif

#if 1
	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_P2P_CMD;
	netHead.nBufSize = sizeof(DVSNET_REGISTER_INFO) + sizeof(COMMAND_HEAD);

	#if 1
	p2p_net_comm.nCmdID = NETCMD_P2P_REGDATAPORT;
	p2p_net_comm.nChannel = 0;
	p2p_net_comm.nCmdLen = sizeof(DVSNET_REGISTER_INFO);
	#endif

	getP2PParam(&p2p_IP_param);
	memset(buffer, 0, 1024);
	memcpy(buffer, &netHead, sizeof(NET_HEAD));
	memcpy(buffer + sizeof(NET_HEAD), &p2p_net_comm, sizeof(COMMAND_HEAD));
	memcpy(buffer + sizeof(NET_HEAD) + sizeof(COMMAND_HEAD), &p2p_register_info,  sizeof(DVSNET_REGISTER_INFO));
	send_lenght = sizeof(NET_HEAD)+ sizeof(COMMAND_HEAD) + sizeof(DVSNET_REGISTER_INFO);
#endif


	ret = tcpSendMsg(sock, buffer, send_lenght);
	if (ret <= 0)
	{
		p2p_IP_param.dwReserved = 0;
		setP2PParam(&p2p_IP_param);
		printf("NAT(%d): Send Error!\n", sock);
		goto Error;
	}

	p2p_IP_param.dwReserved = 1;
	setP2PParam(&p2p_IP_param);
	printf("success send data to p2p server tcpSendMsg = %d\n", ret);
	

	#if 0
	//向p2p服务器udp注册
	ret = udpSendMsg(sock, buffer, send_lenght, addr);
	if (ret <= 0 && errno == EPIPE)
	{
		printf("NAT(%d): Send Error!\n", sock);
		goto Error;
	}
	#endif

   #if 1
	if (remoteRequestProc(sock) < 0)
	{
		printf("NAT(%d): RemoteRequestProc Error!\n", sock);
		goto Error;
	}
   #endif

	if (cmdType != NETCMD_OPEN_TALK_R)	// Add the code by lvjh, 2010-01-14
	{
		g_remote_socket = sock;
	}
	
	
	return 0;
	
Error:
	net_debug();
	shutdown(sock, 2);
	close(sock);
	return -1;
}
#endif
#if 0
{
	int ret = 0;
	int sock = 0;
	char buffer[1024];
	NET_HEAD *netHead;
	char *returnData;
	DVSNET_REGISTER_INFO  p2p_register_info;
	NET_PARAM net_param;
	DVSNET_P2P_PARAM p2p_IP_param;
	int send_lenght;

	#if 1
	MSG_HEAD returnMsg;
	#endif

	printf("natConnect(%s %d %d): ...\n", remoteIP, remotePort, sock);
	
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}

	// Connect with reverse
	ret = natConnect(remoteIP, remotePort, &sock);
	if (ret < 0)
	{
		return -1;
	}
	printf(":test\n");
	printf("natConnect(%s %d %d): %d\n", remoteIP, remotePort, sock, ret);
	printf("stop.......\n");
	
	//netHead = (NET_HEAD *)buffer;
	//returnData = buffer+sizeof(NET_HEAD);

	netHead->nFlag = HDVS_FLAG;
	netHead->nCommand = cmdType;
	netHead->nBufSize = sizeof(MSG_HEAD) +  sizeof(DVSNET_REGISTER_INFO);
	printf("stop.......\n");
	//printf(" netHead->nFlag = %d\n", netHead->nFlag);
	//memcpy(returnData, g_return_data, g_return_data_len);

	
	getNetParam(&net_param);
	getP2PParam(&p2p_IP_param);
	printf("stop.......\n");

	strcpy(p2p_register_info.strLocalIp, net_param.byServerIp);
	strcpy(p2p_register_info.strRemoteIp, p2p_IP_param.sServerUrl);
	p2p_register_info.wLocalPort = net_param.wWebPort;
	p2p_register_info.wRemotePort = p2p_IP_param.nPort;
	strcpy(p2p_register_info.strUser, "admin");
	strcpy(p2p_register_info.strPasswd, "admin");
	
	

	#if 1
	returnMsg.nCmd = NETCMD_P2P_CMD;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DVSNET_REGISTER_INFO);
	#endif

	
	printf("stop.......\n");
	memcpy(buffer, netHead, sizeof(NET_HEAD));
	memcpy(buffer + sizeof(netHead), &returnMsg, sizeof(MSG_HEAD));
	memcpy(buffer + sizeof(netHead)+sizeof(returnMsg), &p2p_register_info, returnMsg.nBufSize);
	send_lenght = sizeof(NET_HEAD)+ sizeof(MSG_HEAD) + sizeof(DVSNET_REGISTER_INFO);
	
	printf("send_lenght = %d\n", send_lenght);
	printf("sizeof(NET_HEAD) = %d\n", sizeof(NET_HEAD));
	printf("sizeof(DVSNET_REGISTER_INFO) = %d\n", sizeof(DVSNET_REGISTER_INFO));
//mody by lv end add--------------------------------------------	
	ret = tcpSendMsg(sock, buffer, send_lenght);
	if (ret <= 0 && errno == EPIPE)
	{
		printf("NAT(%d): Send Error!\n", sock);
		goto Error;
	}
	printf("send p2p server data size = %d\n", ret);
	

	// Process reverse logon
	if (remoteRequestProc(sock) < 0)
	{
		printf("NAT(%d): RemoteRequestProc Error!\n", sock);
		goto Error;
	}
	
	// send keep alive packet
	netHead->nFlag = HDVS_FLAG;
	netHead->nCommand = NETCMD_KEEP_ALIVE;
	netHead->nBufSize = 0;
	
	ret = tcpSendMsg(sock, netHead, sizeof(NET_HEAD));
	if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
	{
		printf("NAT(%d): Keepalive Error!\n", sock);
		goto Error;	
	}
	
	if (cmdType != NETCMD_OPEN_TALK_R)	// Add the code by lvjh, 2010-01-14
	{
		g_remote_socket = sock;
	}
	
	return 0;
	
Error:
	net_debug();
	shutdown(sock, 2);
	close(sock);
	return -1;
}
#endif


int tcpNatConnect(int cmdType)
{
	int ret = -1;
	
	if (g_tcp_nat_run_flag)
	{
		ret = TcpSendMsgToAll(g_remote_ip, g_remote_port, cmdType);
		if (ret < 0)
		{
			return -1;
		}

		return 0;
	}

	return -1;
}


//mody by lv start add--------------------------------------------
void udpNatListenFun(void)
{
	int ret = -1;

	while (!g_server_info.bServerExit)
	{
		if (g_udp_nat_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		if (g_udp_nat_run_flag)
		{
			  if(g_nat_p2p_udp_flag == 1){
				  	printf("TcpSendMsgToAll_UDPremote......\n");
					strcpy(g_remote_ip, REMOTE_IP);
					g_remote_port = REMOTE_PORT;
					ret = TcpSendMsgToAll_UDPremote(g_remote_ip, g_remote_port, NETCMD_NAT_CONNECT);
					if (ret < 0)
					{
						sleep(g_intervalTime);
						continue;
					}
					else
					{
						CLIENT_INFO clientInfo;
						g_nat_p2p_udp_flag = 0;

						printf("g_udp_remote_socket: %d\n", g_remote_socket);

						while (GetClient(g_remote_socket, &clientInfo))
						{
							if (g_nat_flag==1)
							{
								g_remote_socket = -1;
								break;
							}
							
							else
							{
								sleep(g_intervalTime);
								printf("sleep: g_intervalTime = %d\n", g_intervalTime);
							}
						}
						
						printf("GetClient(%d): NULL!\n", g_remote_socket);
						
						g_remote_socket = -1;
					}
				}
		}
	}
}
//mody by lv end add--------------------------------------------

void tcpNatListenFun(void)
{
	int ret = -1;

	while (!g_server_info.bServerExit)
	{
		if (g_tcp_nat_pause_flag)
		{
			sleep(1);
			continue;
		}
		
		if (g_tcp_nat_run_flag)
		{
			//printf("tcpNatListenFun .....\n");
			if(g_nat_p2p_flag == 1){
					printf("TcpSendMsgToAll\n");
					strcpy(g_remote_ip, REMOTE_IP);
					g_remote_port = TCP_REMOTE_PORT;
					ret = TcpSendMsgToAll(g_remote_ip, g_remote_port, NETCMD_NAT_CONNECT);
					if (ret < 0)
					{
						sleep(g_intervalTime);
						continue;
					}
					else
					{
						CLIENT_INFO clientInfo;
						
						g_nat_p2p_flag = 0;

						printf("g_remote_socket: %d\n", g_remote_socket);

						while (GetClient(g_remote_socket, &clientInfo))
						{
							if (g_nat_p2p_flag==1)
							{
								g_remote_socket = -1;
								break;
							}
							
							else
							{
								sleep(g_intervalTime);
								printf("sleep: g_intervalTime = %d\n", g_intervalTime);
							}
						}
						
						printf("GetClient(%d): NULL!\n", g_remote_socket);
						
						g_remote_socket = -1;
					}
				}
			else if(g_nat_flag == 1){
					printf("TcpSendMsgToAll_remote\n");
					ret = TcpSendMsgToAll_remote(g_remote_ip, g_remote_port, NETCMD_NAT_CONNECT);
					if (ret < 0)
					{
						sleep(g_intervalTime);
						continue;
					}
					else
					{
						CLIENT_INFO clientInfo;
						
						g_nat_flag = 0;

						printf("g_remote_socket: %d\n", g_remote_socket);

						while (GetClient(g_remote_socket, &clientInfo))
						{
							if (g_nat_flag==1)
							{
								g_remote_socket = -1;
								break;
							}
							
							else
							{
								sleep(g_intervalTime);
								printf("sleep: g_intervalTime = %d\n", g_intervalTime);
							}
						}
						
						printf("GetClient(%d): NULL!\n", g_remote_socket);
						
						g_remote_socket = -1;
					}
				}
		}
	}
}

// API of module
int NETSDK_NatSetup(char *remoteIP, int remotePort, int interval, char *data, int len)
{
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}
	if (data == NULL || len>1000)
	{
		return -1;
	}
	
	strcpy(g_remote_ip, remoteIP);
	g_remote_port = remotePort;
	g_intervalTime = interval;
	memcpy(g_return_data, data, len);
	g_return_data_len = len;

	g_nat_flag = 1;
	g_nat_p2p_flag = 0;
	
	return 0;
}

int NETSDK_NatSetup_P2P(char *remoteIP, int remotePort, int interval, char *data, int len)
{
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}
	if (data == NULL || len>1000)
	{
		return -1;
	}
	
	strcpy(g_remote_ip, remoteIP);
	g_remote_port = remotePort;
	g_intervalTime = interval;
	memcpy(g_return_data, data, len);
	g_return_data_len = len;

	g_nat_flag = 0;
	g_nat_p2p_flag = 1;
	g_nat_p2p_udp_flag = 0;
		
	return 0;
}
//mody by lv start add--------------------------------------------
int NETSDK_NatSetup_udp_P2P(char *remoteIP, int remotePort, int interval, char *data, int len)
{
	if (remoteIP==NULL || remotePort<=0)
	{
		return -1;
	}
	if (data == NULL || len>1000)
	{
		return -1;
	}
	
	strcpy(g_remote_ip, remoteIP);
	g_remote_port = remotePort;
	g_intervalTime = interval;
	memcpy(g_return_data, data, len);
	g_return_data_len = len;

	g_nat_flag = 0;
	g_nat_p2p_flag = 0;
	g_nat_p2p_udp_flag = 1;
		
	return 0;
}
//mody by lv end add--------------------------------------------

int NETSDK_NatStart()
{
	int ret = -1;
	int res = 0;
	pthread_t tcp_threadID;
	pthread_t udp_threadID;

	g_tcp_nat_run_flag = 1;
	g_udp_nat_run_flag = 1;
	
	g_nat_p2p_udp_flag = 0;
	g_nat_p2p_flag = 0;
	g_nat_flag = 1;

	ret = pthread_create(&tcp_threadID, NULL, (void *)tcpNatListenFun, NULL);
	if (ret)
	{
		g_tcp_nat_run_flag = 0;
		
		return -1;
	}

#if 0	
	ret = pthread_create(&udp_threadID, NULL, (void *)udpNatListenFun, NULL);
	if (ret)
	{
		g_udp_nat_run_flag = 0;
		
		return -1;
	}
#endif	
	
	usleep(10);
	
	return 0;
}

int NETSDK_NatStop()
{
	g_tcp_nat_run_flag = 0;
	
	return 0;
}

int NETSDK_NatPause()
{
	g_tcp_nat_pause_flag = 1;
	
	return 0;
}

int NETSDK_NatResume()
{
	g_tcp_nat_pause_flag = 0;
	
	return 0;
}

