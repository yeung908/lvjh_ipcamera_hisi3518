#include <stdlib.h>
#include <stdio.h>

#include "netTools/netTools.h"
#include "param.h"
#include "session.h"

// 全局变量
static int g_pppoe_run = 0;
static int g_pppoe_pause = 0;
static int g_pppoe_status = 0;

char g_username[32];
char g_password[32];
char g_dns1[128];
char g_dns2[128];

static int set_pap_secrets(char *user,char *password)
{
	FILE *fp = NULL;
	char buf[1024];
	int len = 0;

	if (NULL == user || NULL == password)
	{
		return -1;
	}
	
	fp = fopen("/etc/ppp/pap-secrets", "w");
	if (fp == NULL)
	{
		return -1;
	}

	//len = sprintf(buf,"%c%s%c	*	%c%s%c", '"', user, '"', '"',password, '"');
	len = sprintf(buf, "%c%s%c	*	%c%s%c\n", '"', user, '"', '"',password, '"');

	fwrite(buf, 1, len, fp);
	
	fclose(fp);
	
	return 0;
}

static int start_adsl()
{
	return system("/mnt/mtd/dvs/pppoe/adsl-start");
}

static int stop_adsl()
{
	return system("/mnt/mtd/dvs/pppoe/adsl-stop");
}

int PPPOE_Open()
{
	FILE *fp = NULL;

	fp = fopen("/mnt/mtd/dvs/pppoe/pppoe", "r");
	if (fp == NULL)
	{
		printf("PPPOE_Open(): Failed!\n");
		return -1;
	}
	fclose(fp);
	
	memset(g_username, 0, 32);
	memset(g_password, 0, 32);
	memset(g_dns1, 0, 128);
	memset(g_dns2, 0, 128);
	
	//printf("PPPOE_Open(): OK!\n");

	return 0;
}

int PPPOE_Close()
{
	memset(g_username, 0, 32);
	memset(g_password, 0, 32);
	memset(g_dns1, 0, 128);
	memset(g_dns2, 0, 128);
	
	return 0;
}

int PPPOE_Setup(char *user, char *password, char *dns1, char *dns2)
{
	int ret = -1;
	FILE *fp = NULL;
	char buf[256];
	int len = 0;

	if (NULL == user)
	{
		//printf("PPPOE_Setup(): Failed!\n");
		return -1;
	}

	if (NULL == dns1 && NULL == dns1)
	{
		//printf("PPPOE_Setup(): Failed!\n");
		return -1;
	}
	
	ret = set_pap_secrets(user, password);
	if (ret < 0)
	{
		//printf("PPPOE_Setup(): Failed!\n");
		return -1;
	}
	
	fp = fopen("/etc/ppp/pppoe.conf","w+");
	if (fp == NULL)
	{
		//printf("PPPOE_Setup(): Failed!\n");	
		return -1;
	}
	
	//printf("PPPOE: %s %s %s %s\n", user, password, dns1, dns2);

	len = sprintf(buf,"ETH='%s'\n", "eth0");
	fwrite(buf, 1, len, fp);

	len = sprintf(buf, "USER='%s'\n", user);
	fwrite(buf, 1, len, fp);
	len = sprintf(buf,"DEMAND=%s\n","no");
	fwrite(buf,1,len,fp);	

	len = sprintf(buf,"DNSTYPE=%s\n","SERVER");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"PEERDNS=%s\n","yes");
	//len = sprintf(buf,"PEERDNS=%s\n","no");			// Add the code by lvjh, 2008-2-1
	fwrite(buf,1,len,fp);
	if (dns1 != NULL)
	{
		len = sprintf(buf,"DNS1=%s\n",dns1);
		fwrite(buf,1,len,fp);
	}

	if (dns2 != NULL)
	{
		len = sprintf(buf,"DNS2=%s\n",dns2);
		fwrite(buf,1,len,fp);
	}
	len = sprintf(buf,"DEFAULTROUTE=%s\n","yes");
	fwrite(buf,1,len,fp);
	//len = sprintf(buf,"CONNECT_TIMEOUT=%s\n","60");
	len = sprintf(buf,"CONNECT_TIMEOUT=%s\n","10");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"CONNECT_POLL=%s\n","2");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"ACNAME=%s\n","");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"SERVICENAME=%s\n","");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"PING=%c%s%c\n",'"',".",'"');
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"CF_BASE=%s\n","`basename $CONFIG`");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"PIDFILE=%c%s%c\n",'"',"/var/run/$CF_BASE-adsl.pid",'"');
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"SYNCHRONOUS=%s\n","no");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"CLAMPMSS=%s\n","1412");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"LCP_INTERVAL=%s\n","20");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"LCP_FAILURE=%s\n","3");
	fwrite(buf,1,len,fp);
	//len = sprintf(buf,"PPPOE_TIMEOUT=%s\n","120");
	len = sprintf(buf,"PPPOE_TIMEOUT=%s\n","80");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"FIREWALL=%s\n","NONE");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"LINUX_PLUGIN=%s\n","");
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"PPPOE_EXTRA=%c%c\n",'"','"');
	fwrite(buf,1,len,fp);
	len = sprintf(buf,"PPPD_EXTRA=%c%c\n",'"','"');
	fwrite(buf,1,len,fp);

	fclose(fp);
	
	strncpy(g_username, user, 32);
	strncpy(g_password, password, 32);
	if (dns1 != NULL)
	{
		strncpy(g_dns1, dns1, 128);
	}
	if (dns2 != NULL)
	{
		strncpy(g_dns2, dns2, 128);
	}

	fp = NULL;

	// Add the code by lvjh, 2008-04-09
	fp = fopen("/etc/ppp/pppoe-server-options","w+");
	if (fp == NULL)
	{
		return -1;
	}	

	len = sprintf(buf, "require-pap\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "login\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "lcp-echo-interval 10\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "lcp-echo-failure 2\n");
	fwrite(buf, 1, len, fp);

	fclose(fp);

	system("busybox rm /etc/ppp/options");
	system("busybox rm /etc/ppp/link");

	//printf("PPPOE_Setup(): OK!\n");	

	return 0;	
}

int PPPOE_GetSetup(char *user, char *password, char *dns1, char *dns2)
{
	if (user==NULL || password==NULL || dns1==NULL || dns2==NULL)
	{
		return -1;
	}
	
	strcpy(user, g_username);
	strcpy(password, g_password);
	strcpy(dns1, g_dns1);
	strcpy(dns2, g_dns2);
	
	return 0;
}

int PPPOE_Start()
{
	int ret = -1;
	
	ret = start_adsl();
	
	return ret;
}

int PPPOE_Stop()
{
	int ret = -1;
	
	ret = stop_adsl();
	
	return ret;	
}

int PPPOE_GetStatus()
{	
	return g_pppoe_status;	
}

int pppoeFun()
{
	int ret = -1;
	NET_PARAM netParam;
	PPPOE_PARAM pppoeParam;
	char gateway_addr[32];
	char pppoe_addr[32];
	char cmd[256];
	
	while (g_pppoe_run)
	{
		if (g_pppoe_pause)
		{
			sleep(1);
			continue;
		}
		
		memset(gateway_addr, 0, 32);
		memset(pppoe_addr, 0, 32);

		getNetParam(&netParam);
		getPPPOEParam(&pppoeParam);

		ret = PPPOE_Open();
		if (ret < 0)
		{
			break;
		}

		if (pppoeParam.nOnFlag)
		{
			ret = PPPOE_Setup(pppoeParam.userName, pppoeParam.userPsw, netParam.byDnsAddr, netParam.byDnsAddr);

			while (1)
			{
				ret = PPPOE_Start();
				if (ret == 0)
				{
					printf("PPPOE_Start(): OK!\n");
					ret = get_ip_addr("ppp0", pppoe_addr);
					printf("PPP0 Address: %s\n", pppoe_addr);

					ret = get_specific_gateway("ppp0", gateway_addr);
					if (ret == 0)
					{
						printf("PPP0 Gateway: %s\n", gateway_addr);

						set_gateway_addr(gateway_addr);
			
						memset(cmd, 0, 256);
						sprintf(cmd, "busybox echo %s > /mnt/mtd/dvs/gatwate", gateway_addr);
						system(cmd);
					}

					g_pppoe_pause = 1;
					g_pppoe_status = 1;
	
					// Add the code by lvjh, 2008-10-29
					while (1)
					{
						memset(cmd, 0, 256);
						sprintf(cmd, "busybox ping -c 10 %s", gateway_addr);
						ret = system(cmd);
						if (ret == 0)
						{
							sleep(30);
						}
						else
						{
							printf("PPPOE Link Down!\n");
							break;
						}
					}
				}

				printf("PPPOE_Start(): Failed!\n");
				
				sleep(30);
				PPPOE_Stop();
			}
			
		}
		
		sleep(30);
	}
	
	pthread_exit(NULL);

	return 0;
}

int pppoeStart()
{
	int ret = -1;
	pthread_t threadID;
	
	g_pppoe_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)pppoeFun, NULL);
	if (ret)
	{
		g_pppoe_run = 0;
		return -1;
	}
	
	printf("pppoeStart(): OK!\n");

	return 0;
}

int pppoeStop()
{
	g_pppoe_run = 0;
	
	return 0;
}

int pppoePause()
{
	g_pppoe_pause = 1;
	
	return 0;
}

int pppoeResume()
{
	g_pppoe_pause = 0;
	
	return 0;
}

