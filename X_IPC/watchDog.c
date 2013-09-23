#include <stdlib.h>
#include <stdio.h>
#include<fcntl.h>
#include <pthread.h>

#include "watchDog.h"
#include "miscDrv.h"

// 全局变量
static int g_watchDog_open_flag = 0;
static int g_watchDog_fd = -1;
static int g_watchDog_run = 0;
static int g_watchDog_pause = 0;

static int watchDog_open()
{
	int ret = -1;

#ifdef HI3518
	ret = open("/dev/watchdog", O_RDWR);
	printf("%s %d\n", __func__, __LINE__);
#else
	ret = open("/dev/misc/watchdog", O_RDWR);
	printf("%s %d\n", __func__, __LINE__);
#endif
	if (ret < 0)
	{
		return -1;
	}
	else
	{
		g_watchDog_fd = ret;
		return 0;
	}
}

static int watchDog_close()
{
	if (g_watchDog_fd > 0)
	{
		close(g_watchDog_fd);
	}
	return 0;
}

static int watchDog_feed()
{
	int ret = -1;
	char buffer[16];
	
	if (g_watchDog_fd > 0)
	{
		strcpy(buffer, "tm");
		ret = write(g_watchDog_fd, buffer, 16);
		if (ret == 16)
		{
			return 0;
		}
	}
	
	return -1;
}

int watchDogModuleInit()
{
	int ret = -1;

	ret = watchDog_open();
		
	return ret;
}

int watchDogModuleDeInit()
{
	int ret = -1;
	
	g_watchDog_run = 0;
	g_watchDog_pause = 0;
		
	return 0;
}

int watchDogFun()
{
	int ret = -1;
	
	while (g_watchDog_run)
	{
		if (g_watchDog_pause)
		{
			sleep(1);
			continue;
		}
		
		watchDog_feed();
		sleep(1);
	}
	
	pthread_exit(NULL);

	watchDog_close();
	
	return 0;
}

int watchDogStart()
{
	int ret = -1;
	pthread_t threadID;
	
	ret = watchDog_open();
	if (ret < 0)
	{
		printf("watchDog_open(): Failed!\n");
		return -1;
	}
	if (g_watchDog_fd < 0)
	{
		printf("watchDogStart(): Failed!\n");
		return -1;
	}

	g_watchDog_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)watchDogFun, NULL);
	if (ret)
	{
		g_watchDog_run = 0;
		return -1;
	}
	printf("watchDogStart(): OK!\n");

	return 0;
}

int watchDogStop()
{
	g_watchDog_run = 0;
	
	return 0;
}

int watchDogPause()
{
	g_watchDog_pause = 1;
	
	return 0;
}

int watchDogResume()
{
	g_watchDog_pause = 0;
	
	return 0;
}

