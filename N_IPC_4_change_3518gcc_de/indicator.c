#include <stdlib.h>
#include <stdio.h>
#include<fcntl.h>
#include <pthread.h>

#include "indicator.h"
#include "miscDrv.h"

// 全局变量
static int g_indicator_run = 0;
static int g_indicator_pause = 0;

int indicatorModuleInit()
{
	miscDrv_Load();
	miscDrv_Open();
		
	return 0;
}

int indicatorModuleDeInit()
{
	int ret = -1;
	
	g_indicator_run = 0;
	g_indicator_pause = 0;
		
	return 0;
}

int indicatorFun()
{
	int i = 0;
	int ret = -1;
	
	while (g_indicator_run)
	{
		if (g_indicator_pause)
		{
			sleep(1);
			continue;
		}
		
		for (i=0; i<10; i++)
		{
			//miscDrv_SetIndicator(1);
			usleep(100*1000);
			///sleep(1);
			//miscDrv_SetIndicator(0);
			usleep(100*1000);
			//sleep(1);
		}
		
		sleep(1);
	}
	
	pthread_exit(NULL);
	
	return 0;
}

int indicatorStart()
{
	int ret = -1;
	pthread_t threadID;

	ret = miscDrv_Open();
	if (ret < 0)
	{
		printf("miscDrv_Open: Failed!\n");
		return -1;
	}
	
	g_indicator_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)indicatorFun, NULL);
	if (ret < 0)
	{
		//printf("indicatorStart(): Failed!\n");

		g_indicator_run = 0;
		return -1;
	}

	//printf("indicatorStart(): Ok!\n");

	return 0;
}

int indicatorStop()
{
	g_indicator_run = 0;
	
	return 0;
}

int indicatorPause()
{
	g_indicator_pause = 1;
	
	return 0;
}

int indicatorResume()
{
	g_indicator_pause = 0;
	
	return 0;
}

