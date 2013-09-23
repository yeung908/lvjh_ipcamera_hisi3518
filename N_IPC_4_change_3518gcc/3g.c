#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>




#include "3g.h"
#include "util.h"
#include "netTools/netTools.h"

// /mnt/mtd/dvs/pppoe/huawei-ppp-on --APN=3gnet --pn=*99# --usr= --psw=
// /mnt/mtd/dvs/pppoe/huawei-ppp-on --pn=#777 --usr=card --psw=card
// /mnt/mtd/dvs/pppoe/huawei-ppp-on --pn=#777 --usr=ctnet@mycdma.cn --psw=vnet.mobi
// route add default dev ppp0

// 鍏ㄥ眬鍙橀噺
static int g_G3_flag = 0;	// 0: no module, 1: failed, 2: ok, 3: dial, 4: no sim card
static int g_G3_run = 0;
static int g_G3_pause = 0;
G3_PARAM g_G3_param;

int Detect3GNetwork()
{
	int i = 0;
	int ret = -1;
	int fd = -1;
	int nFlag = 0;
	struct sockaddr_in addr;
	struct hostent *host = NULL;
	
	printf("Detect 3G Network ...\n");
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		printf("socket: error!\n");
		return -1;
	}
	
	if ((host=gethostbyname("www.microsoft.com")) == NULL) 
	{
		printf("gethostbyname(www.microsoft.com): error!\n");
		shutdown(fd, 2);
		close(fd);
		return -1;
	}
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(addr.sin_zero), 8);
	
	for (i=0; i<10; i++)
	{
		printf("Detect 3G Network(%d) ...\n", i);
		
		ret = connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
		if (ret < 0)
		{
			printf("Detect 3G Network(%d): Failed!\n", i);
			
			//shutdown(fd, 2);
			//close(fd);
			//return -1;
			
			// Add the code by lvjh, 2011-06-30
			continue;
		}
		else
		{
			printf("Detect 3G Network(%d): OK!\n", i);
			nFlag = 1;
			break;
		}
	}
	
	shutdown(fd, 2);
	close(fd);
	
	if (nFlag == 0)
	{
		return -1;
	}
		
	return 0;
}

int DeleteDialTempFile()
{
	system("busybox rm -rf /var/run/*");
	system("busybox rm -rf /var/lock/*");
	system("busybox rm -rf /var/getip");
	
	return 0;
}

int KillDialProcess()
{
	int nRet = -1;
	FILE *fp = NULL;
	char buffer[16];
	char cmd[128];
	
	// ps | grep -v grep | grep "pppd" | awk '{ print $1; exit }' > /var/dial
	system("busybox ps | grep -v grep | grep \"pppd\" | awk \'{ print $1; exit }\' > /var/dial");
	
	fp = fopen("/var/dial", "rb");
	if (fp == NULL)
	{
		printf("Can not open file: /var/dial\n");
		
		system("busybox rm -f /var/dial");
		return -1;
	}
	
	memset(buffer, 0, 16);
	nRet = fread(buffer, 1, 15, fp);
	if (nRet <= 0)
	{
		printf("Can not read file: /var/dial\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);
	system("busybox rm -rf /var/dial");
	
	memset(cmd, 0, 128);
	sprintf(cmd, "kill -9 %s", buffer);
	system(cmd);
	printf("KillDialProcess: %s\n", cmd);
	
	DeleteDialTempFile();
	
	return 0;
}

// /mnt/mtd/dvs/pppoe/huawei-ppp-on --APN=3gnet --pn=*99# --usr= --psw=
int dial_cmd()
{
	int nRet = -1;
	char pCmd[256];
	
	// Add the code by lvjh, 2011-03-30
	//DeleteDialTempFile();
	
	//sleep(1);
	
	// Add the code by lvjh, 2010-05-24
	g_G3_flag = 3;
	
	memset(pCmd, 0, 256);

	switch (g_G3_param.nType)
	{
	case 0:
		sprintf(pCmd, "/mnt/mtd/dvs/pppoe/huawei-ppp-on.wcdma --APN=%s --pn=%s --usr=%s --psw=%s", g_G3_param.pAPN, g_G3_param.pDialNumber, g_G3_param.pAccount, g_G3_param.pPassword);
		break;
		
	case 1:
		sprintf(pCmd, "/mnt/mtd/dvs/pppoe/huawei-ppp-on.cdma2000 --APN=%s --pn=%s --usr=%s --psw=%s", g_G3_param.pAPN, g_G3_param.pDialNumber, g_G3_param.pAccount, g_G3_param.pPassword);
		break;
		
	case 2:
		sprintf(pCmd, "/mnt/mtd/dvs/pppoe/huawei-ppp-on.td-scdma --APN=%s --pn=%s --usr=%s --psw=%s", g_G3_param.pAPN, g_G3_param.pDialNumber, g_G3_param.pAccount, g_G3_param.pPassword);
		break;
		
	default:
		sprintf(pCmd, "/mnt/mtd/dvs/pppoe/huawei-ppp-on.wcdma --APN=%s --pn=%s --usr=%s --psw=%s", g_G3_param.pAPN, g_G3_param.pDialNumber, g_G3_param.pAccount, g_G3_param.pPassword);
		break;
	}
	
	//sprintf(pCmd, "/mnt/mtd/dvs/pppoe/huawei-ppp-on --APN=%s --pn=%s --usr=%s --psw=%s", g_G3_param.pAPN, g_G3_param.pDialNumber, g_G3_param.pAccount, g_G3_param.pPassword);
	
	return system(pCmd);
	
}

int G3_Open()
{
	memset(&g_G3_param, 0, sizeof(G3_PARAM));

	return 0;
}

int G3_Close()
{
	memset(&g_G3_param, 0, sizeof(G3_PARAM));
	
	return 0;
}

int G3_Setup(G3_PARAM param)
{	
	memcpy(&g_G3_param, &param, sizeof(G3_PARAM));

	return 0;	
}

int G3_GetSetup(G3_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	memcpy(param, &g_G3_param, sizeof(G3_PARAM));
	
	return 0;
}

int check_3g_module()
{
	int fd = -1;

	// Add the code by lvjh, 2010-10-15
	if (g_G3_param.nType == 2)
	{
		//fd = open("/dev/usb/acm/0", O_RDWR); //delete by liujw 2012-3-19
		fd = open("/dev/usb/tts/0", O_RDWR);
		if (fd < 0)
		{
			return -1;
		}
	}
	else
	{	
		fd = open("/dev/usb/tts/0", O_RDWR);
		if (fd < 0)
		{
			return -1;
		}
	}

	close(fd);
	
	return 0;
}

int get_3g_ip(char *addr)
{
	int code;
	char buf[32];
	
	if (addr == NULL)
	{
		return -1;
	}
	
	switch (g_G3_flag)
	{
	case 0:
		strcpy(addr, "NO 3G MODULE");
		break;
		
	case 1:
		sprintf(addr, "DIAL FAIL: -1");	// 
		break;
		
	case 3:
		strcpy(addr, "3G DIAL ...");
		break;
		
	case 4:
		strcpy(addr, "DIAL FAIL: -2");	// no sim card
		break;
	}
	
	if (g_G3_flag != 2)
	{
		return 0;	
	}
	
	memset(buf, 0, 32);
	get_ip_addr("ppp0", buf);
	
	memcpy(addr, buf, 16);
	addr[15] = '\0';
	
	return 0;
}

int G3Fun()
{
	int ret = -1;
	char gateway_addr[32];
	char G3_addr[32];
	char cmd[256];
	
	// Add the code by lvjh, 2010-04-22
	DeleteDialTempFile();
	sleep(30);
	
	while (g_G3_run)
	{
		if (g_G3_pause)
		{
			sleep(1);
			continue;
		}
		
		memset(gateway_addr, 0, 32);
		memset(G3_addr, 0, 32);

		if (g_G3_param.nOnFlag)
		{
			while (1)
			{
				// Check 3G Module
				//没有必要在while循环里去检测3G模块 add by zhanghui
				
				ret = check_3g_module();
				if (ret < 0)
				{
					printf("Not 3G Module!\n");
					g_G3_flag = 0;
					sleep(10);
					continue;
				}
				
				ret = dial_cmd();
				if (ret == 0)
				{
					sleep(15);
								
					memset(G3_addr, 0, 32);
					memset(gateway_addr, 0, 32);
					
					ret = get_ip_addr("ppp0", G3_addr);
					if (ret < 0)
					{
						g_G3_flag = 4;
						memset(G3_addr, 0, 32);
					}
					else
					{
						g_G3_flag = 2;
						
						printf("3g_dial_cmd(): OK!\n");
								
						printf("PPP0 Address: %s\n", G3_addr);
						
						ret = get_specific_gateway("ppp0", gateway_addr);
						if (ret == 0)
						{
							printf("PPP0 Gateway: %s\n", gateway_addr);
	
							set_gateway_addr(gateway_addr);
						}
						
						// dns
						system("cp -f /etc/ppp/resolv.conf /etc/resolv.conf");
					}				
					
					sleep(5);
					
					//while (1)
					while (g_G3_flag == 2)
					{
						
						ret = Detect3GNetwork();
						if (ret < 0)
						{
							printf("G3 Link Down!\n");
							
							g_G3_flag = 1;
							
							break;
						}
						
						sleep(30);
					}
					
					/*
					while (1)
					//while (g_G3_flag == 2)
					{
						printf("Test 3G Network ...\n");
						
						memset(cmd, 0, 256);
						sprintf(cmd, "busybox ping -c 10 www.google.com");
						ret = system(cmd);
						if (ret == 0)
						{
							sleep(30);
						}
						else
						{
							printf("G3 Link Down!\n");
							
							g_G3_flag = 1;
							
							break;
						}
					}
					*/
				}
				
				printf("G3_Start(): Failed!\n");
							
				KillDialProcess();
				
				g_G3_flag = 1;
				
				sleep(20);
			}
		}
		sleep(2);//add code for debug by liujw 2012-02-29
	}

	return 0;
}

int G3_Start()
{
	int ret = -1;
	pthread_t threadID;
	
	g_G3_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)G3Fun, NULL);
	if (ret)
	{
		g_G3_run = 0;
		return -1;
	}
	printf("G3Fun pid %d \n",threadID);

	return 0;
}

int G3_Stop()
{
	g_G3_run = 0;
	
	return 0;
}

int G3_Pause()
{
	g_G3_pause = 1;
	
	return 0;
}

int G3_Resume()
{
	g_G3_pause = 0;
	
	return 0;
}
