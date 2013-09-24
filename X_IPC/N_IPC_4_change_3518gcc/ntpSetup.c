#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "ntpSetup.h"
#include "param.h"

// 全局变量
static int g_ntp_run = 0;
static int g_ntp_pause = 1;

int ntp_setup(NTP_PARAM param)
{
	// synchronize with time server
	struct hostent *servp = NULL;
	
	if (param.nOnFlag == 0)
	{
		printf("ntpPause \n");
		ntpPause();
		return 0;
	}
	
	/*
	servp = gethostbyname(param.Server);
	if (servp == NULL)
	{
		printf("gethostbyname(%s): Failed!\n", param.Server);
		return -1;
	}
	else
	*/
	else
	{
		char command[64];

		strcpy(command, "/mnt/mtd/dvs/app/msntp -o ");
		
		switch(param.TimeZone)
		{
			case 1: strcat(command, "-12 "); break;
			case 2: strcat(command, "-11 "); break;
			case 3: strcat(command, "-10 "); break;
			case 4: strcat(command, "-9 "); break;
			case 5: strcat(command, "-8 "); break;
			case 6: strcat(command, "-7 "); break;
			case 7: strcat(command, "-6 "); break;
			case 8: strcat(command, "-5 "); break;
			case 9: strcat(command, "-4 "); break;
			case 10: strcat(command, "-3 "); break;
			case 11: strcat(command, "-2 "); break;
			case 12: strcat(command, "-1 "); break;
			case 13: strcat(command, "0 "); break;
			case 14: strcat(command, "1 "); break;
			case 15: strcat(command, "2 "); break;
			case 16: strcat(command, "3 "); break;
			case 17: strcat(command, "4 "); break;
			case 18: strcat(command, "5 "); break;
			case 19: strcat(command, "6 "); break;
			case 20: strcat(command, "7 "); break;
			case 21: strcat(command, "8 "); break;
			case 22: strcat(command, "9 "); break;
			case 23: strcat(command, "10 "); break;
			case 24: strcat(command, "11 "); break;
			case 25: strcat(command, "12 "); break;
			default: strcat(command, "0 "); break;
		}
				
		strcat(command, "-r ");
		strcat(command, param.Server);
		
		printf("%s\n", command);
	
		if( system(command) < 0 )
		{
			 printf("An internal error occurred\n");
			 return -1;
		}
		else
		{
			printf("Server command=%s", command);
			return 0;
		}
		ntpResume();
		printf("ntpResume\n");
	}
}

int ntpFun()
{
	int ret = -1;
	NTP_PARAM ntpParam;
	int nTime = 0;
	
	sleep(10);
	
	while (g_ntp_run)
	{
		if (g_ntp_pause)
		{
			sleep(1);
			continue;
		}
		
		getNtpParam(&ntpParam);
		ntp_setup(ntpParam);
		
		//sleep(60);
		// Add the code by lvjh, 2011-07-05
		if (ntpParam.nInterval == 0)
		{
			nTime = 60*60;
		}
		else if (ntpParam.nInterval <= 100)
		{
			nTime = 60*60*ntpParam.nInterval;
		}
		else
		{
			nTime = 60*60*100;
		}
		
		sleep(nTime);
	}
	
	pthread_exit(NULL);

	return 0;
}

int ntpStart()
{
	int ret = -1;
	pthread_t threadID;
	
	g_ntp_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)ntpFun, NULL);
	if (ret)
	{
		g_ntp_run = 0;
		return -1;
	}
	
	printf("ntpStart(): OK!\n");

	return 0;
}

int ntpStop()
{
	g_ntp_run = 0;
	
	return 0;
}

int ntpPause()
{
	g_ntp_pause = 1;
	
	return 0;
}

int ntpResume()
{
	g_ntp_pause = 0;
	
	return 0;
}
