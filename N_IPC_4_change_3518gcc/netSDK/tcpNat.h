#ifndef __TCPNAT_H_
#define __TCPNAT_H_

#define MAX_TCP_NAT_NUM			1
#define TCP_TIME_OUT			1

//mody by lv start add--------------------------------------------
//#define REMOTE_IP				"192.168.1.79"
#define REMOTE_IP				"183.62.98.180"
//#define REMOTE_IP				"192.168.1.251"
//#define REMOTE_IP				"192.168.2.78"
#define REMOTE_PORT				6000
#define TCP_REMOTE_PORT				6001

#define REMOTE_USER				"admin"
#define REMOTE_PASSWD			"admin"
int TcpSendMsgToAll_UDPremote(char *remoteIP, int remotePort, int cmdType);
//mody by lv end add--------------------------------------------

#endif

