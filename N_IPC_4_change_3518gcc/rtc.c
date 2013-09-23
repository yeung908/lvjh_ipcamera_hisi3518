#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "rtc.h"
#include "param.h"

// 全局变量
static int g_rtc_fd = -1;
static int g_rtc_run = 0;
static int g_rtc_pause = 0;

int rtcModuleLoad()
{
	return 0;
}

int rtcModuleUnload()
{	
	return 0;
}

int rtcCalibration(DATE_PARAM param)
{
	int ret = -1;
	char buffer[32];
	
	if (g_rtc_fd != -1)
	{
		buffer[0] = param.second;
		buffer[1] = param.minute;
		buffer[2] = param.hour;
		buffer[3] = param.day;
		buffer[4] = param.week;
		buffer[5] = param.month;
		buffer[6] = param.year-2000;

		//printf("param.year = %d \n", param.year);
		if(param.year >= 2000)
		ret = write(g_rtc_fd, buffer, 7);
		if (ret >= 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}

int rtcFun()
{
	int ret = -1;
	char buffer[32];
	DATE_PARAM param;
	NTP_PARAM ntpParam;
	
	while (g_rtc_run)
	{
		if (g_rtc_pause)
		{
			sleep(1);
			continue;
		}

#if 0	
		getNtpParam(&ntpParam);
		if (ntpParam.nOnFlag == 1)
		{
			sleep(1);
		}
		else
		{
#endif

			if (g_rtc_fd > 0)
			{
				memset(buffer, 0, 32);

				ret = read(g_rtc_fd, buffer, 7);
				param.year = 2000+buffer[6];
				param.month = buffer[5];
				param.week = buffer[4];
				param.day = buffer[3];
				param.hour = buffer[2];
				param.minute = buffer[1];
				param.second = buffer[0];
				if (buffer[6]>=0 && buffer[6]<999 && 
					buffer[5]>0 && buffer[5]<=12 &&
					buffer[4]>=0 && buffer[4]<=8 &&
					buffer[3]>0 && buffer[3]<=31 &&
					buffer[2]>=0 && buffer[2]<=23 &&
					buffer[1]>=0 && buffer[1]<=59 &&
					buffer[0]>=0 && buffer[0]<=59)
				{
					if(param.year >= 2000){
						ret = setTimeParam(param);
					}
				}

			}
			else
			{
				printf("RTC: Error!\n");
			}
		sleep(60*60);
	}
	
	if (g_rtc_fd != -1)
	{
		close(g_rtc_fd);
		g_rtc_fd = -1;
	}
	
	g_rtc_run = 0;
	g_rtc_pause = 0;
	
	pthread_exit(NULL);
	
	return 0;
}

int rtcCalibrationStart()
{
	int ret = -1;
	pthread_t threadID;

	ret = open("/dev/misc/pcf8563", O_RDWR);
	if (ret < 0)
	{
		printf("rtcCalibrationStart(): %s Failed!\n", strerror(errno));
		return -1;
	}	
	g_rtc_fd = ret;	
	
	g_rtc_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)rtcFun, NULL);
	if (ret)
	{
		g_rtc_run = 0;
		return -1;
	}
	
	printf("rtcCalibrationStart(): OK!\n");

	return 0;
}

int rtcCalibrationStop()
{
	g_rtc_run = 0;
	
	return 0;
}

int rtcCalibrationPause()
{
	g_rtc_pause = 1;
	
	return 0;
}

int rtcCalibrationResume()
{
	g_rtc_pause = 0;
	
	return 0;
}

