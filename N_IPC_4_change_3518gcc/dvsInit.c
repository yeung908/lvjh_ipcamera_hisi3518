#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "videoEncAppModule.h"
#include "audioEncAppModule.h"
#include "audioDecAppModule.h"
#include "searchDevice.h"
#include "param.h"
#include "netTools/netTools.h"
#include "talkModule.h"
#include "rtc.h"
#include "ptz.h"
#include "session.h"
#include "dvsInit.h"
#include "snapshot.h"
#include "acodecDrv.h"
#include "ircut.h"
#include "upnp.h"
#include "include/recordSDK.h"
#include "onvif/libonvif.h"

//#define RECORD_VIDEO_BITRATE
	
#define MAX_AV_BUFF_SIZE	512000
#define MAX_SECOND_AV_BUFF_SIZE	65536


#ifdef RECORD_VIDEO_BITRATE
int g_count = 0;
FILE *g_video_fd = NULL;
#endif

//FILE *g_pdecFile ;


// 全局变量
char *g_av_buffer[MAX_CHANNEL][MAX_CHANNEL_ENC_NUM];
static int g_video_width[MAX_CHANNEL][MAX_CHANNEL_ENC_NUM];
static int g_video_height[MAX_CHANNEL][MAX_CHANNEL_ENC_NUM];
static int g_video_enc_type[MAX_CHANNEL][MAX_CHANNEL_ENC_NUM];

int sendRecordFile(int sock);
//int sendUdpRecordFile(void *param);
int sendUdpRecordFile(int sock, struct sockaddr_in addr, char *buffer);

int setVideoFormat(int nChannel, int nStreamType, int nWidth, int nHeight, int nVencType)
{
	if (nChannel<0 || nChannel>MAX_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>MAX_CHANNEL_ENC_NUM)
	{
		return -1;
	}

	g_video_width[nChannel][nStreamType] = nWidth;
	g_video_height[nChannel][nStreamType] = nHeight;
	g_video_enc_type[nChannel][nStreamType] = nVencType;

	#if 0
	// Add the code by lvjh, 2009-12-10
	if (nWidth == 720)
	{
		g_video_width[nChannel][nStreamType] = 700;
		g_video_width[nChannel][nStreamType] = 1100;
	}
	
	printf("setVideoFormat(%d %d %d %d) \n", nChannel, nStreamType, g_video_width[nChannel][nStreamType], g_video_height[nChannel][nStreamType]);
	#endif
	

	return 0;
}

int getVideoFormat(int nChannel, int nStreamType, int *nWidth, int *nHeight, int *nVencType)
{
	if (nChannel<0 || nChannel>MAX_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>MAX_CHANNEL_ENC_NUM)
	{
		return -1;
	}

	(*nWidth) = g_video_width[nChannel][nStreamType];
	(*nHeight) = g_video_height[nChannel][nStreamType];
	(*nVencType) = g_video_enc_type[nChannel][nStreamType];

	//printf("getVideoFormat(%d %d %d %d) \n", nChannel, nStreamType, *nWidth, *nHeight);

	return 0;
}

int setServerMediaPort(int wServerPort)
{
	FILE *fp = NULL;
	char buffer[1025];
	char *temp = NULL;
	char port[16];
	int ret = -1;
	int position = 0;

	fp = fopen("/mnt/mtd/dvs/www/index.html", "r+b");
	if (fp == NULL)
	{
		printf("setServerMediaPort(): Failed!\n");
		return -1;
	}

	memset(buffer, 0, 1025);

	while (!feof(fp))
	{
		ret = fread(buffer, 1, 1024, fp);
		if (ret <= 0)
		{
			break;
		}

		temp = strstr(buffer, "vPort");
		if (temp != NULL)
		{
			sprintf(port, "%5d", wServerPort);
			memcpy(temp, port, 5);
			fseek(fp, position, SEEK_SET);
			fwrite(buffer, 1, ret, fp);
			break;
		}

		if (ret > 16)
		{
			position += (ret-16);
			fseek(fp, position, SEEK_SET);
		}
	}

	fflush(fp);
	
	fclose(fp);

	return 0;
}

int WireNetworkInit()
{
	int ret = -1;
	int nDhcpFlag = -1;
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	char ip[16];
	char mac_addr[32] = {0};
	char buffer_cache[128] ={0};
	
	ret = getNetParam(&netParam);
	if (ret < 0)
	{
		return -1;
	}
	
	// 自动生成多播地址
	generateMultiAddr(netParam.byMultiAddr);
	
	ifconfig_up_down(ETH_WIRE_DEV, "down");
	
	memset(mac_addr, 0, 32);
	if (get_net_phyaddr_ext(ETH_WIRE_DEV, mac_addr) == 0)
	{
		if (strcmp(netParam.strPhyAddr, mac_addr) != 0)
		{
			sprintf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x", netParam.strPhyAddr[0], netParam.strPhyAddr[1], netParam.strPhyAddr[2], netParam.strPhyAddr[3], netParam.strPhyAddr[4], netParam.strPhyAddr[5]);
			ret = set_net_phyaddr(ETH_WIRE_DEV, mac_addr);
			printf("set_net_phyaddr: (WIRE: %s) %d\n", mac_addr, ret);
		}
	}
	
	ifconfig_up_down(ETH_WIRE_DEV, "up");

	if (netParam.nDhcpOnFlag == 1)
	{
		getSysInfoParam(&sysInfo);
		nDhcpFlag = dhcp_setup(ETH_WIRE_DEV, sysInfo.strDeviceName);
	}
	else
	{
		nDhcpFlag = -1;
	}

	memset(ip, 0, 16);
	ret = get_ip_addr(ETH_WIRE_DEV, ip);

	printf("dhcp_setup(WIRE): %d %s\n", nDhcpFlag, ip);

	if (nDhcpFlag != 0 || strlen(ip)<=0 || netParam.nDhcpOnFlag==0)
	{
		// Add the code by lvjh, 2008-03-22
		unsigned int add1 = 0;
		unsigned int add2 = 0;
		unsigned int add3 = 0;
		unsigned int add4 = 0;

		unsigned int add5 = 0;
		unsigned int add6 = 0;
		unsigned int add7 = 0;
		unsigned int add8 = 0;
			
		printf("DVS IP(WIRE): %s\n", netParam.byServerIp);


	#if 0
		if (set_ip_addr(ETH_WIRE_DEV, netParam.byServerIp) < 0)
		{
			printf("set_ip_addr(%s) Failed!\n", netParam.byServerIp);

			set_ip_addr(ETH_WIRE_DEV, DEFAULT_WIRE_IP);
			set_mask_addr(ETH_WIRE_DEV, DEFAULT_WIRE_MASK);
			//set_gateway_addr(DEFAULT_WIRE_GATEWAY);
			set_gateway_addr_ext(ETH_WIRE_DEV, DEFAULT_WIRE_GATEWAY);
			//return -1;
		}
	#endif
	

		sprintf(buffer_cache, "%s %s %s", "ifconfig",  ETH_WIRE_DEV, netParam.byServerIp);
		system(buffer_cache);

		if (set_mask_addr(ETH_WIRE_DEV, netParam.byServerMask) < 0)
		{
    	   	printf("set_mask_addr(%s) Failed!\n", netParam.byServerMask);
	
			set_mask_addr(ETH_WIRE_DEV, DEFAULT_WIRE_MASK);
			//set_gateway_addr(DEFAULT_WIRE_GATEWAY);
			set_gateway_addr_ext(ETH_WIRE_DEV, DEFAULT_WIRE_GATEWAY);
			//return -1;
		}

		//printf("Old GateWay: %s\n", netParam.byGateway);
		//sscanf(netParam.byServerIp, "%d.%d.%d.%d", &add1, &add2, &add3, &add4);
		//sscanf(netParam.byGateway, "%d.%d.%d.%d", &add5, &add6, &add7, &add8);
		//sprintf(netParam.byGateway, "%d.%d.%d.%d", add1, add2, add3, add8);
		//printf("New GateWay: %s\n", netParam.byGateway);
			
		//if (set_gateway_addr(netParam.byGateway) < 0)
		if (set_gateway_addr_ext(ETH_WIRE_DEV, netParam.byGateway) < 0)
		{
			printf("set_gateway_addr(%s) Failed!\n", netParam.byGateway);
			//set_gateway_addr(DEFAULT_WIRE_GATEWAY);
			set_gateway_addr_ext(ETH_WIRE_DEV, DEFAULT_WIRE_GATEWAY);
			//return -1;
		}
	}

	// DNS
	if (netParam.nDhcpOnFlag == 1)
	{
		char localIP[16];
		char gateway[16];
		
		memset(localIP, 0, 16);
		memset(gateway, 0, 16);

		//ret = get_gateway_addr(gateway);
		ret = get_gateway_addr_ext(ETH_WIRE_DEV, gateway);
		if (ret == 0)
		{
			printf("GateWay(WIRE): %s\n", gateway);

			set_dns_addr(gateway);
			
			strcpy(netParam.byGateway, gateway);
			strcpy(netParam.byDnsAddr, gateway);
		}
		ret = get_ip_addr(ETH_WIRE_DEV, localIP);
		if (ret == 0)
		{
			printf("DHCP: get_ip_addr: %s\n", localIP);
			strcpy(netParam.byServerIp, localIP);
		}
		printf("DHCP: %s %s\n", ETH_WIRE_DEV, localIP);
	
		setNetParam(&netParam);
	}
	else
	{
		set_dns_addr(netParam.byGateway);
	}
	
	return 0;
}

int WirelessNetworkInit()
{
	int ret = -1;
	int nDhcpFlag = -1;
	SYS_INFO sysInfo;
	WIFI_PARAM wifiParam;
	NET_PARAM netParam;
	char ip[16];
	char mac_addr[32] = {0};
	char command_buffer[256]= {0};
	char buffer_cache[128]= {0};
	
	ret = getWifiParam(&wifiParam);
	if (ret < 0)
	{
		return -1;
	}
			
	ret = getNetParam(&netParam);
	if (ret < 0)
	{
		return -1;
	}

	system("ifconfig ra0 192.168.5.5 up");
	memset(mac_addr, 0, 32);
	if (get_net_phyaddr_ext(ETH_WIRELESS_DEV, mac_addr) == 0)
	{
		memcpy(wifiParam.strPhyAddr, mac_addr, 6);
	}
	ret = setWifiParam(&wifiParam);
	if (ret < 0)
	{
		return -1;
	}
	
	if (wifiParam.nOnFlag == 1) 	// 启用无线
	{
		//增加wifi 状态指示
		if(wifi_setup(wifiParam) == 0)
		{
			wifiParam.Reserve = 1;
			ret = setWifiParam(&wifiParam);
			if (ret < 0)
			{
				return -1;
			}
		}
		
#if 1		
		// DHCP设置
		if (wifiParam.nDhcpOnFlag == 1)
		{
			getSysInfoParam(&sysInfo);
			nDhcpFlag = dhcp_setup(ETH_WIRELESS_DEV, sysInfo.strDeviceName);
		}
		else
		{
			nDhcpFlag = -1;
		}
		
		// 固定IP设置
		memset(ip, 0, 16);
		get_ip_addr(ETH_WIRELESS_DEV, ip);
		
		printf("dhcp_setup(WIRELESS): %d %s\n", nDhcpFlag, ip);

		if (nDhcpFlag != 0 || strlen(ip)<=0 || wifiParam.nDhcpOnFlag==0)
		{
			// Add the code by lvjh, 2008-03-22
			unsigned int add1 = 0;
			unsigned int add2 = 0;
			unsigned int add3 = 0;
			unsigned int add4 = 0;
	
			unsigned int add5 = 0;
			unsigned int add6 = 0;
			unsigned int add7 = 0;
			unsigned int add8 = 0;
				
			//判断是不是用户输入的wifi
			sleep(10);
			printf("wifiParam.pReserve[0]  = %d\n", wifiParam.pReserve[0]);
			if(wifiParam.pReserve[0] == 1){
				strcpy(wifiParam.byServerIp, netParam.byServerIp);
				strcpy(wifiParam.byDnsAddr, netParam.byDnsAddr);
				strcpy(wifiParam.byGateway, netParam.byGateway);
				strcpy(wifiParam.byServerMask, netParam.byServerMask);
				
				ret = setWifiParam(&wifiParam);
				if (ret < 0)
				{
					return -1;
				}
			}
			
			printf("DVS IP(WIRELESS): %s\n", wifiParam.byServerIp);
			

			#if 0
			if (set_ip_addr(ETH_WIRELESS_DEV, wifiParam.byServerIp) < 0)
			{
				printf("set_ip_addr(%s) Failed!\n", wifiParam.byServerIp);
	
				set_ip_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_IP);
				set_mask_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_MASK);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
			#endif
			
			sprintf(buffer_cache, "%s %s %s", "ifconfig",  ETH_WIRELESS_DEV, wifiParam.byServerIp);
			system(buffer_cache);
			
		
			if (set_mask_addr(ETH_WIRELESS_DEV, wifiParam.byServerMask) < 0)
			{
				printf("set_mask_addr(%s) Failed!\n", wifiParam.byServerMask);
		
				set_mask_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_MASK);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
	
			//printf("Old GateWay: %s\n", wifiParam.byGateway);
			//sscanf(wifiParam.byServerIp, "%d.%d.%d.%d", &add1, &add2, &add3, &add4);
			//sscanf(wifiParam.byGateway, "%d.%d.%d.%d", &add5, &add6, &add7, &add8);
			//sprintf(wifiParam.byGateway, "%d.%d.%d.%d", add1, add2, add3, add8);
			//printf("New GateWay: %s\n", wifiParam.byGateway);
				
			//if (set_gateway_addr(wifiParam.byGateway) < 0)
			if (set_gateway_addr_ext(ETH_WIRELESS_DEV, wifiParam.byGateway) < 0)
			{
				printf("set_gateway_addr(%s) Failed!\n", wifiParam.byGateway);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
		}

		// DNS设置
		if (wifiParam.nDhcpOnFlag == 1)
		{
			char localIP[16];
			char gateway[16];
			
			memset(localIP, 0, 16);
			memset(gateway, 0, 16);
	
			//ret = get_gateway_addr(gateway);
			ret = get_gateway_addr_ext(ETH_WIRELESS_DEV, gateway);
			if (ret == 0)
			{
				printf("GateWay(WIRELESS): %s\n", gateway);

				set_dns_addr(gateway);
				//add_dns_addr(gateway);
				
				strcpy(wifiParam.byGateway, gateway);
				strcpy(wifiParam.byDnsAddr, gateway);
			}
			ret = get_ip_addr(ETH_WIRELESS_DEV, localIP);
			if (ret == 0)
			{
				printf("DHCP: get_ip_addr: %s\n", localIP);
				strcpy(wifiParam.byServerIp, localIP);
			}
			printf("DHCP: %s %s\n", ETH_WIRELESS_DEV, localIP);
			
		
			wifiParam.Reserve = 1;
			setWifiParam(&wifiParam);
		}
		else
		{
			wifiParam.Reserve = 1;
			set_dns_addr(wifiParam.byGateway);
			
			
			//add_dns_addr(wifiParam.byGateway);
		}
#endif	
	}
	else
	{
		wifiParam.Reserve = 1;
		setWifiParam(&wifiParam);
	}
	return 0;
}

#if 0	
{
	int ret = -1;
	int nDhcpFlag = -1;
	SYS_INFO sysInfo;
	WIFI_PARAM wifiParam;
	NET_PARAM netParam;
	char ip[16];
	char mac_addr[32];
	char command_buffer[256];
	char buffer_cache[128];
	
	ret = getWifiParam(&wifiParam);
	if (ret < 0)
	{
		return -1;
	}
			
	ret = getNetParam(&netParam);
	if (ret < 0)
	{
		return -1;
	}

	system("ifconfig ra0 192.168.5.5 ");
	memset(mac_addr, 0, 32);
	if (get_net_phyaddr_ext(ETH_WIRELESS_DEV, mac_addr) == 0)
	{
		memcpy(wifiParam.strPhyAddr, mac_addr, 6);
	}
	ret = setWifiParam(&wifiParam);
	if (ret < 0)
	{
		return -1;
	}
	
	if (wifiParam.nOnFlag == 1) 	// 启用无线
	{
		//增加wifi 状态指示
		if(wifi_setup(wifiParam) == 0)
		{
			wifiParam.Reserve = 1;
			ret = setWifiParam(&wifiParam);
			if (ret < 0)
			{
				return -1;
			}
		};
		
#if 1		
		// DHCP设置
		if (wifiParam.nDhcpOnFlag == 1)
		{
			getSysInfoParam(&sysInfo);
			nDhcpFlag = dhcp_setup(ETH_WIRELESS_DEV, sysInfo.strDeviceName);
		}
		else
		{
			nDhcpFlag = -1;
		}
		
		// 固定IP设置
		memset(ip, 0, 16);
		get_ip_addr(ETH_WIRELESS_DEV, ip);
		
		printf("dhcp_setup(WIRELESS): %d %s\n", nDhcpFlag, ip);

		if (nDhcpFlag != 0 || strlen(ip)<=0 || wifiParam.nDhcpOnFlag==0)
		{
			// Add the code by lvjh, 2008-03-22
			unsigned int add1 = 0;
			unsigned int add2 = 0;
			unsigned int add3 = 0;
			unsigned int add4 = 0;
	
			unsigned int add5 = 0;
			unsigned int add6 = 0;
			unsigned int add7 = 0;
			unsigned int add8 = 0;
				
			//判断是不是用户输入的wifi
			//sleep(10);
			printf("wifiParam.pReserve[0]  = %d\n", wifiParam.pReserve[0]);
			if(wifiParam.pReserve[0] == 1){
				strcpy(wifiParam.byServerIp, netParam.byServerIp);
				}
				
			printf("DVS IP(WIRELESS): %s\n", wifiParam.byServerIp);
			
		
			if (set_ip_addr(ETH_WIRELESS_DEV, wifiParam.byServerIp) < 0)
			{
				printf("set_ip_addr(%s) Failed!\n", wifiParam.byServerIp);
	
				set_ip_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_IP);
				set_mask_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_MASK);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
		
			if (set_mask_addr(ETH_WIRELESS_DEV, wifiParam.byServerMask) < 0)
			{
				printf("set_mask_addr(%s) Failed!\n", wifiParam.byServerMask);
		
				set_mask_addr(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_MASK);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
	
			printf("Old GateWay: %s\n", wifiParam.byGateway);
			sscanf(wifiParam.byServerIp, "%d.%d.%d.%d", &add1, &add2, &add3, &add4);
			sscanf(wifiParam.byGateway, "%d.%d.%d.%d", &add5, &add6, &add7, &add8);
			sprintf(wifiParam.byGateway, "%d.%d.%d.%d", add1, add2, add3, add8);
			printf("New GateWay: %s\n", wifiParam.byGateway);
				
			//if (set_gateway_addr(wifiParam.byGateway) < 0)
			if (set_gateway_addr_ext(ETH_WIRELESS_DEV, wifiParam.byGateway) < 0)
			{
				printf("set_gateway_addr(%s) Failed!\n", wifiParam.byGateway);
				//set_gateway_addr(DEFAULT_WIRELESS_GATEWAY);
				set_gateway_addr_ext(ETH_WIRELESS_DEV, DEFAULT_WIRELESS_GATEWAY);
				//return -1;
			}
		}

		// DNS设置
		if (wifiParam.nDhcpOnFlag == 1)
		{
			char localIP[16];
			char gateway[16];
			
			memset(localIP, 0, 16);
			memset(gateway, 0, 16);
	
			//ret = get_gateway_addr(gateway);
			ret = get_gateway_addr_ext(ETH_WIRELESS_DEV, gateway);
			if (ret == 0)
			{
				printf("GateWay(WIRELESS): %s\n", gateway);

				set_dns_addr(gateway);
				//add_dns_addr(gateway);
				
				strcpy(wifiParam.byGateway, gateway);
				strcpy(wifiParam.byDnsAddr, gateway);
			}
			ret = get_ip_addr(ETH_WIRELESS_DEV, localIP);
			if (ret == 0)
			{
				printf("DHCP: get_ip_addr: %s\n", localIP);
				strcpy(wifiParam.byServerIp, localIP);
			}
			printf("DHCP: %s %s\n", ETH_WIRELESS_DEV, localIP);
			
		
			wifiParam.Reserve = 1;
			setWifiParam(&wifiParam);
		}
		else
		{
			wifiParam.Reserve = 1;
			set_dns_addr(wifiParam.byGateway);
			
			
			//add_dns_addr(wifiParam.byGateway);
		}
#endif	
	}
	else
	{
		wifiParam.Reserve = 1;
		setWifiParam(&wifiParam);
	}
	return 0;
}
#endif


int getWiFIStatusConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	char tmp_buffer[128];

	if (param == NULL)
	{
		return -1;
	}

	memset(tmp_buffer, 0, 128);

	fp = fopen(WIFI_STATUS_CONF_PATH, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: MAC_CONFIGURE_FILE!\n");
		return -1;
	}
	
	ret = fread(tmp_buffer, 1, 128, fp);
	if(ret < 0){
			fclose(fp);
			return -1;
	}
	memcpy(param, tmp_buffer, 128);
	fclose(fp);		
	return 0;
}


int getHTTPDStatusConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	char tmp_buffer[128];

	if (param == NULL)
	{
		return -1;
	}

	memset(tmp_buffer, 0, 128);

	fp = fopen(HTTPD_STATUS_CONF_PATH, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: HTTPD_STATUS_CONF_PATH!\n");
		return -1;
	}
	
	ret = fread(tmp_buffer, 1, 128, fp);
	if(ret < 0){
			fclose(fp);
			return -1;
	}
	memcpy(param, tmp_buffer, 128);
	fclose(fp);		
	return 0;
}

int check_wifi_status(void)
{
	WIFI_PARAM wifiParam;
	char buffer_cache[128];
	int ret = 0;
	static int flags = 1;
	ret = getWifiParam(&wifiParam);
	if (ret < 0)
	{
		return -1;
	}
	system("/mnt/mtd/dvs/wlan/iwpriv ra0 connStatus  > /param/wifi_status.conf");
	ret = getWiFIStatusConfigure(buffer_cache);
	if(ret < 0)
	{
		printf("getWiFIStatusConfigure error!!!\n");
		return -1;
	}
	if(wifiParam.nOnFlag){
		if(strstr(buffer_cache,"Disconnected")){
			WirelessNetworkInit();
			sleep(30);
			system("/mnt/mtd/dvs/wlan/iwpriv ra0 connStatus  > /param/wifi_status.conf");
			getWiFIStatusConfigure(buffer_cache);
			if(ret < 0)
			{
				printf("getWiFIStatusConfigure error!!!\n");
				return -1;
			}
			if(strstr(buffer_cache,"Connected")){
				system("ifconfig eth0 down");	
				system("ifconfig eth0 up");
				wifiParam.Reserve = 0;
				ret = setWifiParam(&wifiParam);
				if (ret < 0)
				{
					return -1;
				}
			}
			else{
				wifiParam.Reserve = 1;
				ret = setWifiParam(&wifiParam);
				if (ret < 0)
				{
					return -1;
				}
				set_wifi_status_pause();
			}
			ret = 0;
		}
		else{
				system("ifconfig eth0 down");
				set_wifi_status_resume();
				wifiParam.Reserve = 0;
				ret = setWifiParam(&wifiParam);
				if (ret < 0)
				{
					return -1;
				}
				ret = 0;
		}
	}
	return ret;
}

int MakeMobileConf()
{
	int nRet = -1;
	FILE *fp = NULL;
	USER_INFO_PARAM param;
	char buf[128];
	int len = 0;
	int i = 0;
	
	fp = fopen("/mnt/mtd/dvs/mobile/tmpfs/httpd.conf", "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: httpd.conf.\n");

		return -1;
	}
	
	memset(buf, 0, 128);
	getUserInfoParam(&param);
	
	len = sprintf(buf,"/:%s:%s\n", param.Admin.strName, param.Admin.strPsw);
	fwrite(buf, 1, len, fp);
	
	for (i=0; i<MAX_USER_COUNT; i++)
	{
		if (strlen(param.Users[i].strName) > 0)
		{
			memset(buf, 0, 128);
			len = sprintf(buf,"/:%s:%s\n", param.Users[i].strName, param.Users[i].strPsw);
			fwrite(buf, 1, len, fp);
		}
	}
	
	memset(buf, 0, 128);
	len = sprintf(buf,"/:%s:%s\n", GUEST_USER, param.Guest.strPsw);
	fwrite(buf, 1, len, fp);
	
	fclose(fp);
	
	return 0;
}

int MakeWebsConf()
{
	int nRet = -1;
	FILE *fp = NULL;
	USER_INFO_PARAM param;
	char buf[128];
	int len = 0;
	int i = 0;
	
	fp = fopen("/mnt/mtd/dvs/app/webServer/webServer.conf", "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: webServer.conf.\n");

		return -1;
	}
	
	getUserInfoParam(&param);
	memset(buf, 0, 128);
	len = sprintf(buf,"TABLE=users\n\nROW=0\n\nname=%s\n\npassword=%s\n\ngroup=Administrator\n\nprot=0\n\ndisable=0\n\n", param.Admin.strName, param.Admin.strPsw);
	fwrite(buf, 1, len, fp);
	
	for (i=0; i<MAX_USER_COUNT; i++)
	{
		if (strlen(param.Users[i].strName) > 0)
		{
			memset(buf, 0, 128);
			len = sprintf(buf,"TABLE=users\n\nROW=0\n\nname=%s\n\npassword=%s\n\ngroup=Administrator\n\nprot=0\n\ndisable=0\n\n", param.Users[i].strName, param.Users[i].strPsw);
			fwrite(buf, 1, len, fp);
		}
	}
	
	memset(buf, 0, 128);
	len = sprintf(buf,"TABLE=users\n\nROW=0\n\nname=%s\n\npassword=%s\n\ngroup=Administrator\n\nprot=0\n\ndisable=0\n\n",GUEST_USER, param.Guest.strPsw);
	fwrite(buf, 1, len, fp);


	memset(buf, 0, 128);
	len = sprintf(buf,"TABLE=groups\n\nROW=0\n\nname=Administrator\n\npriv=4\n\nmethod=2\n\nprot=1\n\ndisable=0\n\n");
	fwrite(buf, 1, len, fp);
	


	memset(buf, 0, 128);
	len = sprintf(buf,"TABLE=access\n\nROW=0\n\nname=/\n\nmethod=2\n\nsecure=0\n\ngroup=Administrator\n\n");
	fwrite(buf, 1, len, fp);

	fclose(fp);
	fp = NULL;
	
	return 0;
}


int AllNetworkInit()
{
	int ret = -1;
	int webServerPort = 0;
	int serverMediaPort = 0;
	char httpdCmd[256];
	FILE *fp = NULL;
	NET_PARAM netParam;
	WIFI_PARAM wifiParam;
	SYS_INFO sysInfo;

	NETSDK_ServerInit(5);
	WirelessNetworkInit();
	WireNetworkInit();
	getNetParam(&netParam);

 	getWifiParam(&wifiParam);
	add_dns_addr(netParam.byDnsAddr, wifiParam.byDnsAddr);
	serverMediaPort = netParam.wServerPort;    
	if (serverMediaPort < 0 || serverMediaPort > 65535)
	{
		serverMediaPort = 4000;
	}

	webServerPort = netParam.wWebPort;
	if (webServerPort < 0 || webServerPort > 65535)
	{
		webServerPort = 80;
	}
 
	if (webServerPort == serverMediaPort)
	{
		serverMediaPort = 4000;
		webServerPort = 80;
	}

	MakeMobileConf();
	memset(httpdCmd, 0, 256);
	#ifdef WEBSERVER
		sprintf(httpdCmd, "/mnt/mtd/dvs/app/webServer/web.sh %d &", netParam.wWebPort);
		MakeWebsConf();
	#else 
		
		sprintf(httpdCmd, "busybox httpd -p %d -h /mnt/mtd/dvs/www -c /mnt/mtd/dvs/mobile/tmpfs/httpd.conf -r \"Web Server Authentication\"", webServerPort);
	#endif
	
	system(httpdCmd);

	fp = fopen("/mnt/mtd/dvs/www/a.js", "w");
	if (fp != NULL)
	{
		fprintf(fp, "<!--\nvar vPort = %d;\n-->", serverMediaPort);
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
	else
	{
		printf("Can not open the file(/mnt/mtd/dvs/www/a.js)!\n");
	}
	
	getSysInfoParam(&sysInfo);
	
	fp = fopen("/mnt/mtd/dvs/www/b.js", "w+");
	if (fp != NULL)
	{
		fprintf(fp, "<!--\nvar vLanguage = %d;\n-->", sysInfo.nLanguage);
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
	else
	{
		printf("Can not open the file：/mnt/mtd/dvs/www/b.js\n");
	}
	printf("Network initializing OK!\n");

    return 0; 
}

int avSendBufferDeInit()
{
	int i = 0;
	int j = 0;
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		for (j=0; j<MAX_CHANNEL_ENC_NUM; j++)
		{
			if (g_av_buffer[i][j])
			{
				free(g_av_buffer[i][j]);
			}
		}
	}
	
	return 0;
}

int avSendBufferInit()
{
	int i = 0;
	int j = 0;
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		for (j=0; j<MAX_CHANNEL_ENC_NUM; j++)
		{	
			if(j == 1)g_av_buffer[i][j] = (char *)malloc(MAX_SECOND_AV_BUFF_SIZE);
			g_av_buffer[i][j] = (char *)malloc(MAX_AV_BUFF_SIZE);
			if (g_av_buffer[i][j] == NULL)
			{
				avSendBufferDeInit();
				return -1;
			}
		}
	}
	
	return 0;
}

int videoSendBitRateFun(int nChannel, int nStreamType, void *stream, int size, int type)
{
	int ret = -1;
	int audioSize = 0;
	char *temp = NULL;
	int nWidth = 0;
	int nHeight = 0;
	int nVencType = 0;

	VIDOE_FRAME_HEADER *videoHead;
	AV_FRAME_HEAD *frameHead;

#ifdef RECORD
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	if (stream == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>=2)
	{
		return -1;
	}

	if (g_av_buffer[nChannel][nStreamType] == NULL)
	{
		return -1;
	}
	
	if (size<=0 || size>MAX_AV_BUFF_SIZE)
	{
		return -1;
	}

	videoHead = (VIDOE_FRAME_HEADER *)stream;
	frameHead = (AV_FRAME_HEAD *)g_av_buffer[nChannel][nStreamType];
	
	getVideoFormat(nChannel, nStreamType, &nWidth, &nHeight, &nVencType);


	frameHead->nImageWidth = nWidth;	
	frameHead->nImageHeight = nHeight;	
	
	frameHead->nReserve = (unsigned char)(nVencType);
	
	frameHead->nVideoSize = size-sizeof(VIDOE_FRAME_HEADER);
	frameHead->nTimeTick = (unsigned int)(videoHead->timestamp);
	frameHead->bKeyFrame = (unsigned char)(videoHead->IFrameFlag);
	
	frameHead->nAudioSize = 0;
	
	memcpy(g_av_buffer[nChannel][nStreamType]+sizeof(AV_FRAME_HEAD), stream+sizeof(VIDOE_FRAME_HEADER), size-sizeof(VIDOE_FRAME_HEADER));
	
	getAudioBitRateBuffer(nChannel, nStreamType, g_av_buffer[nChannel][nStreamType]+sizeof(AV_FRAME_HEAD)+size-sizeof(VIDOE_FRAME_HEADER), &audioSize);
	if (audioSize > 0)
	{
		frameHead->nAudioSize = audioSize;
	}
	if (nStreamType == 0)
	{
		ret = NETSDK_WriteOneFrame1(nChannel, g_av_buffer[nChannel][0]);
	}
	else
	{
		ret = NETSDK_WriteOneFrame2(nChannel, g_av_buffer[nChannel][1]);
	}

#ifdef RECORD
	
	// record SDK
	cmdParam.nChannel = nChannel;
	cmdParam.nOpt = RSDKCMD_SEND_FRAME;
	cmdParam.param.frameBuffer = NULL;
	if (nStreamType == 0)	// Change the code by lvjh, 2009-05-27
	{
		cmdParam.param.frameBuffer = g_av_buffer[nChannel][0];
		RECORDSDK_Operate(&cmdParam, NULL, NULL);
	}	
#endif
	usleep(10);
		
	return 0;
}

int netSdkStart()
{
	int i = 0;
	int ret = -1;
	AV_INFO avInfo;
	VENC_PARAM param;
	REMOTE_CONNECT_PARAM remoteConnectParam;
	UPNP_PARAM upnp;

	char *ip = NULL;
	char buffer[1024];
	struct hostent *host = NULL;
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	NET_HEAD netHead;
	
	getSysInfoParam(&sysInfo);
	getNetParam(&netParam);
	memcpy(buffer, &sysInfo, sizeof(SYS_INFO));
	memcpy(buffer+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));

	getRemoteConnectParam(&remoteConnectParam);
	
	ret = NETSDK_ServerOpen();
	if (ret < 0)
	{
		printf("NETSDK_ServerOpen() Failed!\n");		
		return -1;
	}
	
	ret = NETSDK_ServerSetup(MAX_CHANNEL, netParam.wServerPort, netParam.byMultiAddr, 512);
	if (ret < 0)
	{
		printf("NETSDK_ServerSetup(%d %d %s) Failed!\n", MAX_CHANNEL, netParam.wServerPort, netParam.byMultiAddr);		
		return -1;
	}
	else
	{
		printf("NETSDK_ServerSetup(%d %d %s) OK!\n", MAX_CHANNEL, netParam.wServerPort, netParam.byMultiAddr);
	}

	NETSDK_SetUserCheckFunc(checkUser);
	NETSDK_SetUdpUserCheckFunc(checkUdpUser);
	
	NETSDK_SetServerRecvFunc(recvCmdProcFun);

#ifdef RECORD
	NETSDK_SetFileTransferFunc(sendRecordFile);
	NETSDK_SetFileUdpTransferFunc(sendUdpRecordFile);
#endif
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		getVideoEncParam(i, 0, &param);

		if (param.reserve)
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_MJPEG;
		}
		else
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_HISI;
		}


		avInfo.nImageHeight = param.nEncodeHeight;
		avInfo.nImageWidth = param.nEncodeWidth;
		avInfo.nVideoBitRate = param.nBitRate;
		avInfo.nFrameRate = param.nFramerate;

#ifdef G726
		avInfo.nAudioEncType = ENCODE_AUDIO_G726;
#else 
		avInfo.nAudioEncType = ENCODE_AUDIO_G711;
#endif

		avInfo.nAudioChannels = 0x01;
		avInfo.nAudioSamples = AUDIOSAMPLES;
		avInfo.nAudioBitRate = AUDIOSAMPLES;
		avInfo.nReserve = 0;
	
		NETSDK_SetAVInfo1(i, &avInfo);
#ifdef DEBUG		
		printf("avInfo.nImageWidth = %d \n avInfo.nImageHeight = %d\n", avInfo.nImageWidth, avInfo.nImageHeight);
#endif
		getVideoEncParam(i, 1, &param);

		if (param.reserve)
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_MJPEG;
		}
		else
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_HISI;
		}

		avInfo.nImageHeight = param.nEncodeHeight;
		avInfo.nImageWidth = param.nEncodeWidth;
		avInfo.nVideoBitRate = param.nBitRate;
		avInfo.nFrameRate = param.nFramerate;

#ifdef G726
		avInfo.nAudioEncType = ENCODE_AUDIO_G726;
#else 
		avInfo.nAudioEncType = ENCODE_AUDIO_G711;
#endif
		avInfo.nAudioChannels = 0x01;
		avInfo.nAudioSamples = AUDIOSAMPLES;
		avInfo.nAudioBitRate = AUDIOSAMPLES;
		
		NETSDK_SetAVInfo2(i, &avInfo);
	}
	NETSDK_SetCallback(talkStart, talkFun);
	UDPNETSDK_SetCallback(UdpTalkStart, talkFun);
	
	ret = NETSDK_ServerStart();
	if (ret < 0)
	{
		printf("NETSDK_ServerStart() Failed!\n");		
		return -1;
	}
	else
	{
		printf("NETSDK_ServerStart() OK!\n");
	}
	
	if (!remoteConnectParam.nOnFlag)
	{
		NETSDK_NatPause();
	}

	return 0;
}

int setVideoInAttr(int nChannel)
{
	int i = 0;
	VIDEO_IN_ATTR param;
	VIDEO_FLIP_PARAM flip;
	VIDEO_MIRROR_PARAM mirror;
	VIDEO_HZ_PARAM hz;

	for (i=0; i<MAX_CHANNEL; i++)
	{
		getVideoInAttrParam(i, &param);
#ifdef CCD
		printf("CCD driver initializing...\n");
		vadcDrv_SetBrightness(i, param.nBrightness);
		vadcDrv_SetContrast(i, param.nContrast);
		vadcDrv_SetSaturation(i, param.nSaturation);
		vadcDrv_SetHue(i, param.nHue);
#endif
#ifdef HD_CMOS
		vadcDrv_SetBrightness(i, param.nBrightness);
		vadcDrv_SetContrast(i, param.nContrast);
		vadcDrv_SetSaturation(i, param.nSaturation);
		vadcDrv_SetHue(i, param.nHue);

		getVideoFlipParam(i, &flip);
		vadcDrv_SetImageFlip(i, flip.nFlip);
		getVideoMirrorParam(i, &mirror);
		vadcDrv_SetImageMirror(i, mirror.nMirror);
		getVideoHzParam(i, &hz);
		vadcDrv_SetImageHz(i, hz.nHz);
#endif
	}

	return 0;
}

int g3ModuleStartUp()
{
	G3_PARAM param;
	
	memset(&param, 0, sizeof(G3_PARAM));
	get3gParam(&param);
	
	G3_Open();
	G3_Close();
	G3_Setup(param);
	G3_Start();
	
	return 0;
}

#define MAX_SEND_SIZE 64 
struct mymsgbuf 
{ 
	long mtype; 
	char mtext[MAX_SEND_SIZE]; 
}; 

int CGI_Proc_Func()
{
	int nRet = -1;
	
	key_t key; 
	int msgqueue_id = 0; 
	struct mymsgbuf qbuf; 
	
	key = 888888;
	
	msgqueue_id = msgget(key, IPC_CREAT|0660);
	if (msgqueue_id < 0)
	{
		return -1;
	}
	
	while (1)
	{
		qbuf.mtype = CTRL_PTZ; 
		nRet = msgrcv(msgqueue_id, &qbuf, MAX_SEND_SIZE, CTRL_PTZ, 0); 
		if (nRet > 0)
		{
			PTZ_CMD param;

			memcpy(&param, qbuf.mtext, sizeof(PTZ_CMD));
			
			CGI_ProcFun(qbuf.mtype, qbuf.mtext);
		}
		else
		{
			printf("msgrcv(%d): %s\n", errno, strerror(errno));
			usleep(10*1000);
		}
	}
	
	return 0;
}

int CGI_Proc_Start()
{
	int nRet = -1;
	pthread_t thrdID;
					
	nRet = pthread_create(&thrdID, NULL, (void *)&CGI_Proc_Func, NULL);
	if (nRet < 0)
	{
		return -1;
	}
		
	return 0;
}

void Stop(int signo) 
{
    printf("oops! stop!!!\n");
    exit(0);
}



int dvsInit()
{
	int ret = -1;
	int i = 0;
	NTP_PARAM ntpParam;
	AUDIO_PATH_PARAM audioPath;


	signal(SIGINT, Stop); 

	// 初始最大音视频通道数
	set_channel_num(MAX_CHANNEL);

	// 获取系统参数
	ret = getParamStatusFromFile();
	if (ret == 0)
	{
		printf("get saved parameter!\n");
		getParamFromFile();
	}
	else if (ret == SOFTWARE_RESET)
	{
		printf("get soft parameter!\n");
		initSystemParam(SOFTWARE_RESET);
	}
	else
	{
		printf("get hard parameter!\n");
		initSystemParam(HARDWARE_RESET);
	}

#ifdef MOBILE_VIEW
	system("mount -t tmpfs tmpfs /mnt/mtd/dvs/mobile/tmpfs");
#endif

#if 0
	// 启动看门狗
	ret = watchDogStart();
	if(ret < 0)
	{
		printf("watchDogStart() error \n");
		return -1;
	}
#endif


	// 初始化网络
	AllNetworkInit();

#if 1
	// 启动报警处理
//	alarmProcModuleInit();
//	alarmProcStart();

	// 初始化AV发送缓冲
	avSendBufferInit();

	//SNAPSHOT_Init();
	//SNAPSHOT_Start();

	for (i=0; i<MAX_CHANNEL; i++)
	{
		setVideoInAttr(i);
	}


	// 启动视频编码模块
	ret = videoEncModuleStartup(videoSendBitRateFun, SNAPSHOT_Send);
	if (ret < 0)
	{
		printf("videoEncModuleStartup() Failed!\n");		
		return -1;
	}
	else
	{
		printf("videoEncModuleStartup() OK!\n");	
	}
#endif
	// 启动网络SDK
	ret = netSdkStart();
	if (ret < 0)
	{
		printf("Init Net SDK error\n");
	}
	
	// 启动服务器信息广播
	ret = serInfoBroadcastStart();
	if (ret)
	{
		return -1;
	}
    //onvif 协议
	ret = ONVIFStart();
	if(ret < 0)
	{
		printf("ONVIFStart() error \n");
		return -1;
	}

	printf("DVS Initialize OK!\n");
	return 0;	
}

#ifdef RECORD
int UdpRecExt(int hSock, char *precvBuf, int nSize, struct sockaddr_in addr)
{
	int ret = 0;
	fd_set fset;
	struct timeval to;
	int nLen = 0;

	memset(&to, 0, sizeof(to));

	// 输入参数校验
	if (hSock<=0 || hSock>65535)
	{
		return -1;
	}
	if (precvBuf == NULL)
	{
		return -2;
	}
	if (nSize <= 0)
	{
		return -3;
	}

	FD_ZERO(&fset);
	FD_SET(hSock, &fset);
	to.tv_sec = 5;
	to.tv_usec = 0;

	printf("UdpRecExt ...\n");

	// SOCKET接收超时或错误,则退出
	ret = select(hSock+1, &fset, NULL, NULL, &to);
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
		net_debug();
		return -4;
	}

	if (!FD_ISSET(hSock, &fset))
	{
		net_debug();
		return -5;
	}
	printf("UdpRecExt recvfrom %d \n", nSize); 
		
	// 接收数据
	nLen = sizeof(addr);
	ret = recvfrom(hSock, precvBuf , nSize, MSG_WAITALL, (struct sockaddr*)&addr, (socklen_t *)&nLen);
	//ret = recv(hSock, precvBuf, nSize, 0);
	if (ret < 0)
	{
		//printf("TcpReceive Failed\n");
		return -6;
	}
	
	//printf("TcpReceive OK\n");

	return ret;
}

int TcpRecExt(int hSock, char *precvBuf, int nSize)
{
	int ret = 0;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	//int nLen = 0;

	memset(&to, 0, sizeof(to));

	// 输入参数校验
	if (hSock<=0 || hSock>65535)
	{
		return -1;
	}
	if (precvBuf == NULL)
	{
		return -2;
	}
	if (nSize <= 0)
	{
		return -3;
	}

	FD_ZERO(&fset);
	FD_SET(hSock, &fset);
	to.tv_sec = 5;
	to.tv_usec = 0;

	//printf("TcpRecExt ...\n");

	// SOCKET接收超时或错误,则退出
	ret = select(hSock+1, &fset, NULL, NULL, &to);
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
		net_debug();
		return -4;
	}

	if (!FD_ISSET(hSock, &fset))
	{
		net_debug();
		return -5;
	}
		
	// 接收数据
	//nLen = sizeof(addr);
	//ret = recvfrom(hSock, precvBuf , nSize, MSG_WAITALL, (struct sockaddr*)&addr, (socklen_t *)&nLen);
	ret = recv(hSock, precvBuf, nSize, 0);
	if (ret < 0)
	{
		//printf("TcpReceive Failed\n");
		return -6;
	}
	
	//printf("TcpReceive OK\n");

	return ret;
}


int readRecordFileInfo(FILE *fp, char *buffer, int *size)
{
	int ret = -1;
	int count = 0;
	int pos = 0;
	char *fileHead = NULL;
	char *pRecFileIndex = NULL;

	if (fp==NULL || buffer==NULL ||size==NULL)
	{
		return -1;
	}

	fileHead = (char *)buffer;
	pRecFileIndex = (char *)(buffer+140);

	ret = fread(fileHead, 1, 140, fp);
	if (ret < 140)
	{		
		return -2;
	}
	pos = *(int *)(fileHead+32);
	fseek(fp, pos, SEEK_SET);

	while (!feof(fp))
	{
		ret = fread(pRecFileIndex+count*8, 1, 8, fp);
		if (ret <= 0)
		{	
			break;
		}			

		count++;
	}


	*size = 140+count*8;

	return 0;
}

int sendUdpRecordFile(int sock, struct sockaddr_in addr, char *buffer)
{
	int i = 0;
	int ret = -1;
	char *pCommBuf = NULL;
	int socket = -1;

	NET_HEAD netHead;
	DEV_CMD_RETURN devCmdReturn;
	READ_REC_FILE readRecFileReturn;
	
	DEV_CMD_HEADER *devCmdHeader;
	READ_REC_FILE *readHead;

	int bFirst = 0;
	int count = 0;

	socket = sock;
	if (socket < 0)
	{
		return -1;
	}

	pCommBuf = (char *)malloc(256*1024);
	if (pCommBuf == NULL)
	{
		printf("malloc: pCommBuf Failed!!!\n");
		return -1;
	}

	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_REC_FILE;
	netHead.nBufSize = 0;
	netHead.nErrorCode = 0;
	netHead.nReserve = 0;
	
	{
		memcpy(pCommBuf, buffer, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(READ_REC_FILE));
		ret = sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(READ_REC_FILE);

	

		devCmdHeader = (DEV_CMD_HEADER *)(pCommBuf+sizeof(NET_HEAD));
		readHead = (READ_REC_FILE *)(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER));
		strcpy(g_server_info.cur_file_name, readHead->szFileName);
		memcpy(&readRecFileReturn, readHead, sizeof(READ_REC_FILE));

		if (devCmdHeader->nCmdID != REMOTE_RECORD_PLAYBACK 
			&& devCmdHeader->nCmdID != REMOTE_RECORD_BACKUP 
			&& devCmdHeader->nCmdID != REMOTE_JPEG_PLAYBACK 
			&& devCmdHeader->nCmdID != REMOTE_JPEG_BACKUP)
		{
			printf("UDP RECORD CMD NOT SUPPORT(%x)!!!\n", devCmdHeader->nCmdID);
			goto exit_pos;
		}
		

		devCmdReturn.nCmdID = devCmdHeader->nCmdID;
		switch (devCmdHeader->nReserve)
		{
		case OPEN_RECORD_FILE:

			printf("OPEN_RECORD_FILE: ...\n");

			if (g_server_info.hFile)
			{
				fclose(g_server_info.hFile);
				g_server_info.hFile = NULL;
			}
		
			g_server_info.hFile = fopen(g_server_info.cur_file_name, "rb");
			if (NULL == g_server_info.hFile)
			{
				printf("OPEN_RECORD_FILE: %s Error!\n", g_server_info.cur_file_name);

				devCmdReturn.nCmdLen = sizeof(READ_REC_FILE);
				devCmdReturn.nResult = -1;
				devCmdReturn.nReserve = 0;

				netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE);

				memset(pCommBuf, 256*1024, 0);
				memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
				memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
				memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));
				
				ret = sendto(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
		
			}
			else
			{
				if (devCmdHeader->nCmdID == REMOTE_RECORD_PLAYBACK || devCmdHeader->nCmdID == REMOTE_RECORD_BACKUP)
				{
					ret = readRecordFileInfo(g_server_info.hFile, pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), &g_server_info.size);
				}
				else	// JPEG
				{
					struct stat st;

					ret = 0;
					g_server_info.size = 0;

					ret = stat(g_server_info.cur_file_name, &st);
					if (ret == 0)
					{
						readRecFileReturn.nReadLen = st.st_size;
					}
					else
					{
						readRecFileReturn.nReadLen = 0;
					}

					ret = 0;
				}

				if (ret == 0)
				{
					netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+g_server_info.size;

					devCmdReturn.nCmdLen = sizeof(READ_REC_FILE)+g_server_info.size;
					devCmdReturn.nResult = 0;
					devCmdReturn.nReserve = 0;
					
					memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
					memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
					memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));
					
					ret = sendto(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+g_server_info.size, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
		
					printf("readRecordFileInfo: %d %d OK!\n", ret, g_server_info.size);
				}
				else
				{
					printf("readRecordFileInfo: %d Error!\n", ret);
				}
			}
			break;

		case CLOSE_RECORD_FILE:
			printf("CLOSE_RECORD_FILE: ...\n");
			if (g_server_info.hFile)
			{
				fclose(g_server_info.hFile);
				g_server_info.hFile = NULL;
			}
		goto exit_pos;

		case READ_RECORD_FILE:
		//	printf("READ_RECORD_FILE: ...\n");
			if (strcmp(readRecFileReturn.szFileName, g_server_info.cur_file_name) != 0)
			{			
				//curReadPos = 0;
				g_server_info.curReadPos = 0;
				
				if (g_server_info.hFile)
				{
					fclose(g_server_info.hFile);
					g_server_info.hFile = NULL;
				}

				break;
			}

			if (g_server_info.curReadPos != readRecFileReturn.nReadPos)
			{
				printf("file offset :%d\n",readRecFileReturn.nReadPos);
				fseek(g_server_info.hFile, readRecFileReturn.nReadPos, SEEK_SET);
				g_server_info.curReadPos = readRecFileReturn.nReadPos;
			}
			if (g_server_info.hFile)
			{
				ret = fread(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), sizeof(char), readRecFileReturn.nReadLen, g_server_info.hFile);
			}
			if (ret < 0)
			{
				break;
			}
			if (ret < readRecFileReturn.nReadLen)
			{
				if (g_server_info.hFile)
				{
					fclose(g_server_info.hFile);
					g_server_info.hFile = NULL;
				}
				g_server_info.curReadPos = 0;
			}
		//	printf("fread = %d\n", ret);
			readRecFileReturn.nReadLen = ret;
			g_server_info.curReadPos += ret;

			devCmdReturn.nCmdLen = sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen;
			devCmdReturn.nResult = 0;
			devCmdReturn.nReserve = 0;

			netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen;
					
			memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
			memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
			memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));

			ret = sendto(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr));
		
			break;

		default:
			
			printf("RECORD_FILE: NOT SUPPORT CMD: %x\n", devCmdHeader->nReserve);
			break;
		}
	}
	
	
exit_pos:
	
	if (pCommBuf)
	{
		free(pCommBuf);
		pCommBuf = NULL;
	}
	//printf("sendRecordFile: Exit!\n");
	
	return 0;
}
	

int sendRecordFile(int sock)
{
	int i = 0;
	int ret = -1;
	int size = 0;
	int curReadPos = -1;
	char *pCommBuf = NULL;
	FILE *hFile = NULL;
	char cur_file_name[128];
	int socket = -1;

	NET_HEAD netHead;
	DEV_CMD_RETURN devCmdReturn;
	READ_REC_FILE readRecFileReturn;
	
	DEV_CMD_HEADER *devCmdHeader;
	READ_REC_FILE *readHead;

	int bFirst = 1;
	int count = 0;

	socket = sock;
	if (socket < 0)
	{
		return -1;
	}

	pCommBuf = (char *)malloc(256*1024);
	if (pCommBuf == NULL)
	{
		printf("malloc: pCommBuf Failed!!!\n");
		return -1;
	}

	memset(cur_file_name, 0, 128);

	netHead.nFlag = HDVS_FLAG;
	netHead.nCommand = NETCMD_REC_FILE;
	netHead.nBufSize = 0;
	netHead.nErrorCode = 0;
	netHead.nReserve = 0;
	
	printf("sendRecordFile: %d\n", socket);

	while (1)
	{
		//printf("sendRecordFile ...\n");

		if (bFirst)
		{
			ret = TcpRecExt(socket, pCommBuf, sizeof(DEV_CMD_HEADER)+sizeof(READ_REC_FILE));
		}
		else
		{
			ret = TcpRecExt(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER)+sizeof(READ_REC_FILE));
		}

		if (ret < 0)
		{
			printf("TcpRecExt: %d %d\n", socket, ret);
			break;
		}
		else if(ret == 0)
		{
			count++;
			if (count >10)
			{
				break;
			}
			printf("TcpRecExt: %d %d Timeout!!!\n", socket, ret);
			continue;
		}

		if (ret < sizeof(DEV_CMD_HEADER)+sizeof(READ_REC_FILE))
		{
			printf("TcpRecExt: %d %d\n", socket, ret);
			break;
		}
		
		if (bFirst)
		{
			devCmdHeader = (DEV_CMD_HEADER *)pCommBuf;
			readHead = (READ_REC_FILE *)(pCommBuf+sizeof(DEV_CMD_HEADER));
			strcpy(cur_file_name, readHead->szFileName);
			memcpy(&readRecFileReturn, readHead, sizeof(READ_REC_FILE));

			bFirst = 0;
		}
		else
		{
			devCmdHeader = (DEV_CMD_HEADER *)(pCommBuf+sizeof(NET_HEAD));
			readHead = (READ_REC_FILE *)(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_HEADER));
			strcpy(cur_file_name, readHead->szFileName);
			memcpy(&readRecFileReturn, readHead, sizeof(READ_REC_FILE));
		}
	
		
		//if (devCmdHeader->nCmdID != REMOTE_RECORD_PLAYBACK && devCmdHeader->nCmdID != REMOTE_RECORD_BACKUP)
		if (devCmdHeader->nCmdID != REMOTE_RECORD_PLAYBACK 
			&& devCmdHeader->nCmdID != REMOTE_RECORD_BACKUP 
			&& devCmdHeader->nCmdID != REMOTE_JPEG_PLAYBACK 
			&& devCmdHeader->nCmdID != REMOTE_JPEG_BACKUP)
		{
			printf("RECORD CMD NOT SUPPORT(%x)!!!\n", devCmdHeader->nCmdID);
			break;
		}
		

		devCmdReturn.nCmdID = devCmdHeader->nCmdID;
		
		switch (devCmdHeader->nReserve)
		{
		case OPEN_RECORD_FILE:

			printf("OPEN_RECORD_FILE: ...\n");

			if (hFile)
			{
				fclose(hFile);
				hFile = NULL;
			}
		
			hFile = fopen(cur_file_name, "rb");
			if (NULL == hFile)
			{
				printf("OPEN_RECORD_FILE: %s Error!\n", cur_file_name);

				devCmdReturn.nCmdLen = sizeof(READ_REC_FILE);
				devCmdReturn.nResult = -1;
				devCmdReturn.nReserve = 0;

				netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE);
				
				memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
				memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
				memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));
				
				ret = send(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), 0);				
			}
			else
			{
				// Add the code by lvjh, 2008-04-28
				//ret = readRecordFileInfo(hFile, pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), &size);
				
				// Add the code by lvjh, 2008-05-05
				if (devCmdHeader->nCmdID == REMOTE_RECORD_PLAYBACK || devCmdHeader->nCmdID == REMOTE_RECORD_BACKUP)
				{
					ret = readRecordFileInfo(hFile, pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), &size);
				}
				else	// JPEG
				{
					struct stat st;

					ret = 0;
					size = 0;

					ret = stat(cur_file_name, &st);
					if (ret == 0)
					{
						readRecFileReturn.nReadLen = st.st_size;
					}
					else
					{
						readRecFileReturn.nReadLen = 0;
					}

					ret = 0;
				}

				if (ret == 0)
				{
					netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+size;

					devCmdReturn.nCmdLen = sizeof(READ_REC_FILE)+size;
					devCmdReturn.nResult = 0;
					devCmdReturn.nReserve = 0;
					
					memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
					memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
					memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));
					
					ret = send(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+size, 0);

					printf("readRecordFileInfo: %d %d OK!\n", ret, size);
				}
				else
				{
					printf("readRecordFileInfo: %d Error!\n", ret);
				}
			}
			break;

		case CLOSE_RECORD_FILE:
			printf("CLOSE_RECORD_FILE: ...\n");
			if (hFile)
			{
				fclose(hFile);
				hFile = NULL;
			}
			//break;
			goto exit_pos;

		case READ_RECORD_FILE:
			//printf("READ_RECORD_FILE: ...\n");
			if (strcmp(readRecFileReturn.szFileName, cur_file_name) != 0)
			{			
				curReadPos = 0;
				
				if (hFile)
				{
					fclose(hFile);
					hFile = NULL;
				}

				break;
			}

			if (curReadPos != readRecFileReturn.nReadPos)
			{
				fseek(hFile, readRecFileReturn.nReadPos, SEEK_SET);
				curReadPos = readRecFileReturn.nReadPos;
			}
			if (hFile)
			{
				ret = fread(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE), sizeof(char), readRecFileReturn.nReadLen, hFile);
			}
			if (ret < 0)
			{
				break;
			}
			if (ret < readRecFileReturn.nReadLen)
			{
				if (hFile)
				{
					fclose(hFile);
					hFile = NULL;
				}
				curReadPos = 0;
			}

			readRecFileReturn.nReadLen = ret;
			curReadPos += ret;

			devCmdReturn.nCmdLen = sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen;
			devCmdReturn.nResult = 0;
			devCmdReturn.nReserve = 0;

			netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen;
					
			memcpy(pCommBuf, &netHead, sizeof(NET_HEAD));
			memcpy(pCommBuf+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
			memcpy(pCommBuf+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &readRecFileReturn, sizeof(READ_REC_FILE));

			ret = send(socket, pCommBuf, sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(READ_REC_FILE)+readRecFileReturn.nReadLen, 0);
			
			break;

		default:
			printf("RECORD_FILE: NOT SUPPORT CMD: %x\n", devCmdHeader->nReserve);
			break;
		}
	}

exit_pos:
	if (hFile)
	{
		fclose(hFile);
		hFile = NULL;
	}
	if (pCommBuf)
	{
		free(pCommBuf);
		pCommBuf = NULL;
	}
	
	shutdown(socket, 2);
	close(socket);

	printf("sendRecordFile: Exit!\n");
	
	return 0;
}
#endif

