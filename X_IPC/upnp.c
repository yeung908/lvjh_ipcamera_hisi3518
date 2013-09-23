#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include "param.h"

static int g_upnp_ctrl_run_flag = 0;
static int g_wifi_ctrl_run_flag = 0;
static int g_wifi_status_flag = 0;
static int g_scheduled_reboot_ctrl_run_flag = 0;
 


static int g_upnp_ctrl_pause_flag = 0;
static int g_wifi_ctrl_pause_flag = 1;
static int g_web_upnp_ctrl_pause_flag = 1;
static int g_time_flag = 0;
	

static char g_device_check_upnp_strLocalIp[16] = {0};


static unsigned long g_cur_year = 0;
static unsigned long g_cur_month = 0;
static unsigned long g_cur_day = 0;
static unsigned long g_cur_week = 0;
static unsigned long g_cur_hour = 0;
static unsigned long g_cur_minute = 0;
static unsigned long g_cur_second = 0;

int UPNP_Open()
{
	return 0;
}

int UPNP_Close()
{
	return 0;
}

int check_wifi_reboot_func()
{
    NET_PARAM net_param;
	int flags = 0;
	NET_PARAM netParam;
	WIFI_PARAM wifiParam;

	while(g_upnp_ctrl_run_flag)
	{
		printf("check_wifi_status_func ....\n");
		if(g_wifi_ctrl_pause_flag){
			sleep(60);
			g_wifi_ctrl_pause_flag = check_wifi_status();
			continue;
		}
		AllNetworkInit();
		sleep(60);
		g_wifi_ctrl_pause_flag = check_wifi_status();
	}
	
	return 0;
}

int check_wifi_status_func(void)
{
    NET_PARAM net_param;
	int ret = 0;
	NET_PARAM netParam;
	WIFI_PARAM wifiParam;
	

	while(g_wifi_ctrl_run_flag)
	{
			sleep(10);
			ret = check_wifi_status();
			if(ret < 0)
			{
				printf("check_wifi_status error!!!\n");
			}
	 }
	
	return 0;
}


int scheduled_reboot_func(void)
{

 	int cur_time = 0;
	int j = 0;
	int ret = 0;
	int flag = 0;
	TIME_REBOOT_PARAM timerRebootParam;
	int nRebootInterval = 0;
	int nRebootHous = 0;
	int nRebootTime = 0;
	int nTimeSetFlag = 0;
	

	time_t lasttime; 
	time_t nowtime; 

	
	struct tm *p;	   //定义tm结构体

		
	getLastRebootTimeParam(&lasttime);
	while(g_scheduled_reboot_ctrl_run_flag)
	{
		getTimeRebootParam(&timerRebootParam);
		g_cur_minute = getMinute()+getHour()*60;
		g_cur_second = (getMinute()+getHour()*60)*60 + getSecond();
 		if(timerRebootParam.bEnable== 0){
			sleep(1);
			continue;
		}
		#if 0
		p = gmtime(&lasttime);	
			printf("reboot time:: %04d %02d %02d %02d:%02d:%02d \n", 
			 p->tm_year + 1900,p->tm_mon,,p->tm_mday
			p->tm_hour, p->tm_min, p->tm_sec);
		#endif
		
			
		if(g_time_flag == 0){
			if(lasttime == 0){
				if(g_cur_minute == (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute))
				{
					g_time_flag = 1;
				}
			}
			else{
					g_time_flag = 1;
	
			}
		}
		else
		{
			if(g_time_flag == 1){
				time(&nowtime); 
				//printf("start timer nowtime %d %d %d\n\n", nowtime, lasttime, timerRebootParam.nRebootInterval);
				if (((nowtime - lasttime) > 0 )&&((nowtime - lasttime) >= timerRebootParam.nRebootInterval*60*60)) 
				{
						printf("timer reboot\n");
						setLastRebootTimeParam(&nowtime);
						saveParamToFile();
						RebootSystem();
				}
			}
			else if(g_time_flag == 2)
			{
				nTimeSetFlag = g_cur_minute - (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute);
				if(nTimeSetFlag != 0){
					//printf("(g_cur_minute - (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute)) = %d\n",(g_cur_minute - (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute))); 
					if(nTimeSetFlag <= 0)
						lasttime = nowtime - (g_cur_minute - (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute))*60 - 24*60*60;
					else 
						lasttime = nowtime - (g_cur_minute - (timerRebootParam.nRebootHour*60 + timerRebootParam.nRebootMinute))*60;
						setLastRebootTimeParam(&lasttime);
						saveParamToFile();
				}
				g_time_flag = 0;
			}
			
		}
		usleep(1000*500);
	 }
	
	return 0;
}


int UPNP_NAT_Setup()
{
    NET_PARAM net_param;
	DVSNET_UPNP_PARAM upnp_param;
	UPNP_PORT_INFO upnp_update_ip_param;
	UPNP_PORT_INFO upnp_ip_param;
	WIFI_PARAM wifiParam;
	UPNP_WIFI_IP_PORT_INFO ip_wifi_param;
	DVSNET_RTSP_PARAM rtsp_param;

	char upnpCmd[256];
	int flags = 0;
	sleep(40);
	getNetParam(&net_param);
	getNewUpnpParam(&upnp_param);
	getRtspParam(&rtsp_param);
	getWifiParam(&wifiParam);
	memset(&ip_wifi_param, 0, sizeof(UPNP_WIFI_IP_PORT_INFO));
	strcpy(ip_wifi_param.strLocalIp, net_param.byServerIp);
	strcpy(ip_wifi_param.strWIFIIp, wifiParam.byServerIp);
	ip_wifi_param.nWebPort = net_param.wWebPort;
	ip_wifi_param.nDataPort = net_param.wServerPort;
	ip_wifi_param.nRtspPort = rtsp_param.nRtspPort;
	ip_wifi_param.nOnFlag = wifiParam.nOnFlag;
	save_ip_wifi_configure(&ip_wifi_param);
	
	#if 0
	while(1)
	{
		getWifiParam(&wifiParam);
		//printf("g_upnp_ctrl_pause_flag  = %d\n", g_upnp_ctrl_pause_flag );
		if(g_upnp_ctrl_pause_flag == 0){
			system("upnpc -w");
			getUSERP2PPORTConfigure(&upnp_update_ip_param);
			printf("upnpc:::upnp_update_ip_param = %s:: %d\n", upnp_update_ip_param.strRemoteIp, strlen(upnp_update_ip_param.strRemoteIp));
			if(strcmp(upnp_update_ip_param.strRemoteIp, g_device_check_upnp_strLocalIp))
			{
				printf("UPNP_NAT_Setup::UPNP CHANGE\n");
				strcpy(g_device_check_upnp_strLocalIp, upnp_update_ip_param.strRemoteIp);
				printf("update_gateway = %s\n", upnp_update_ip_param.strRemoteIp);
			
				UPNP_Resume();
			}
			sleep(1);
			continue;
		}
		upnp_param.upnpWebPort = net_param.wWebPort;
		upnp_param.upnpDataPort = net_param.wServerPort;
		upnp_param.upnpRtspPort = rtsp_param.nRtspPort;
		memset(upnpCmd, 0, 256);

	//	printf("wifiParam.nOnFlag = %d, wifiParam.Reserve  = %d\n",wifiParam.nOnFlag, wifiParam.Reserve);
		if(wifiParam.nOnFlag&&(g_wifi_status_flag == 1)){
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);
		
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
 			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
 			usleep(1000*10);
		
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, rtsp_param.nRtspPort , upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
 			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
 			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, rtsp_param.nRtspPort, upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
		}
		else{
			#if 1
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}

			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, rtsp_param.nRtspPort , upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
			
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, rtsp_param.nRtspPort, upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
			usleep(1000*10);
		#endif
		}
		upnp_param.bEnabled = 1;
		setNewUpnpParam(&upnp_param);
		UPNP_Pause();
	}
	#endif
	

	return 0;
}

int UPNP_Setup()
{
    NET_PARAM net_param;
	DVSNET_UPNP_PARAM upnp_param;
	UPNP_PORT_INFO upnp_update_ip_param;
	UPNP_PORT_INFO upnp_ip_param;
	WIFI_PARAM wifiParam;
	UPNP_WIFI_IP_PORT_INFO ip_wifi_param;
	
	DVSNET_RTSP_PARAM rtsp_param;
	char upnpCmd[256];
	int flags = 0;
	
	getNetParam(&net_param);
	getNewUpnpParam(&upnp_param);
	getRtspParam(&rtsp_param);
	getWifiParam(&wifiParam);

	memset(&ip_wifi_param, 0, sizeof(UPNP_WIFI_IP_PORT_INFO));
	strcpy(ip_wifi_param.strLocalIp, net_param.byServerIp);
	strcpy(ip_wifi_param.strWIFIIp, wifiParam.byServerIp);
	ip_wifi_param.nWebPort = net_param.wWebPort;
	ip_wifi_param.nDataPort = net_param.wServerPort;
	ip_wifi_param.nRtspPort = rtsp_param.nRtspPort;
	save_ip_wifi_configure(&ip_wifi_param);

	sleep(30);
	while(1)
	{
		upnp_param.upnpWebPort = net_param.wWebPort;
		upnp_param.upnpDataPort = net_param.wServerPort;
		upnp_param.upnpRtspPort = rtsp_param.nRtspPort;
		memset(upnpCmd, 0, 256);

		if(wifiParam.nOnFlag&&(g_wifi_status_flag == 1)){
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);
		
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
 			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
 			usleep(1000*10);
		
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, rtsp_param.nRtspPort , upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
 			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
 			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, rtsp_param.nRtspPort, upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
		}
		else{
			#if 1
			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}

			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);

			sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, rtsp_param.nRtspPort , upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
			
			upnp_param.bEnabled = 1;
			setNewUpnpParam(&upnp_param);
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
			if(system(upnpCmd) !=  0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpWebStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpWebStatus = 1;
			}
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpDataStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpDataStatus = 1;
			}
			usleep(1000*10);
			sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, rtsp_param.nRtspPort, upnp_param.upnpRtspPort);
			if(system(upnpCmd) != 0 ){
				printf("upnpc portmapping error = %s\n", upnpCmd);
				upnp_param.nReserved = 0;
				upnp_param.upnpRtspStatus = 0;
			}
			else{
				upnp_param.nReserved = 1;
				upnp_param.upnpRtspStatus = 1;
			}
			usleep(1000*10);
		#endif
		}
		upnp_param.bEnabled = 1;
		setNewUpnpParam(&upnp_param);
		#if 0
		if(!flags){
			set_register_ddns();
			flags = 1;
		}
		#endif
		usleep(1000*1000);
		//sleep(60*60);
	}
	return 0;
}

int UPNP_Start()
{
	pthread_t thread;
	pthread_t upnp_nat_thread;
	
	int ret = 0;
	g_upnp_ctrl_run_flag = 1;
	g_wifi_ctrl_run_flag  = 1;
	g_upnp_ctrl_pause_flag = 1;

	#if 0
	ret = pthread_create(&thread, NULL, (void *)UPNP_Setup, NULL);	
	if (ret != 0)
	{
		printf("upnp create thread error \n");
		return -1;
	}
	#endif

	#if 0
	ret = pthread_create(&upnp_nat_thread, NULL, (void *)UPNP_NAT_Setup, NULL);	
	if (ret != 0)
	{
		printf("upnp create thread error \n");
		return -1;
	}
	#endif
	
	
	
	sleep(1);

	return 0;
}


int CHECK_WIFI_Start()
{
	pthread_t thread;
	pthread_t wifi_thread;
	
	int ret = 0;
	g_wifi_ctrl_run_flag  = 1;
	
	ret = pthread_create(&wifi_thread, NULL, (void *)check_wifi_status_func, NULL);	
	if (ret != 0)
	{
		printf("upnp create thread error \n");
		return -1;
	}
	sleep(1);

	return 0;
}


int scheduledReboot()
{
	pthread_t time_reboot_thread;
	
	int ret = 0;
	g_scheduled_reboot_ctrl_run_flag  = 1;

	pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&time_reboot_thread, NULL, (void *)scheduled_reboot_func, NULL);	
	if (ret != 0)
	{
		pthread_attr_destroy(&attr);
		printf("timeReboot create thread error \n");
		return -1;
	}
	sleep(1);
	pthread_attr_destroy(&attr);

	return 0;
}

int UPNP_Stop()
{
	g_upnp_ctrl_run_flag = 0;
	return 0;
}

int UPNP_Pause()
{
	g_upnp_ctrl_pause_flag = 0;
	return 0;
}

int UPNP_Resume()
{
	g_upnp_ctrl_pause_flag = 1;
	return 0;
}


int upnp_web_pause()
{
	g_web_upnp_ctrl_pause_flag = 1;
}

int upnp_web_resume()
{
	g_web_upnp_ctrl_pause_flag = 0;
}

int set_wifi_status_pause()
{
	g_wifi_status_flag = 0;
}

int set_wifi_status_resume()
{
	g_wifi_status_flag = 1;
}

int get_wifi_status()
{
	return g_wifi_status_flag;
}

int set_time_flag()
{
	g_time_flag = 2;
	return 0;
}





