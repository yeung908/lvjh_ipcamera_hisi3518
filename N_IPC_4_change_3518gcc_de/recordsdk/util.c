/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：util.h
* 文件说明：该文件描述了时间操作
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-01-30
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>

#include "global.h"

// 全局变量
static int g_max_record_channel = 1;

int set_av_channel_num(int num)
{
	if (num <= 0 || num > MAX_AV_CHANNEL)
	{
		return -1;
	}
	
	g_max_record_channel = num;
	
	return 0;
}

int get_av_channel_num()
{
	return g_max_record_channel;
}

int get_year()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return (p->tm_year + 1900);
}

int get_month()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return (p->tm_mon + 1);
}

int get_day()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_mday;
}

int get_hour()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_hour;
}

int get_minute()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_min;
}

int get_second()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_sec;
}

int get_msec()
{
	struct timeval tv;
	struct timezone tz;
	int val;
	gettimeofday(&tv,&tz);
	val = tv.tv_sec * 1000 + (int)(tv.tv_usec/1000);
	return val;
}

int get_week()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	//return ((p->tm_wday == 0) ? 6 : (p->tm_wday -1));
	return p->tm_wday;	// 星期对应整数规则:0-6 星期一 --> 星期天
}

int set_system_time(int year,int month,int day,int hour,int minute,int second)
{
	time_t timep;
	struct tm *p;
	struct timeval tv;
	struct timezone tz;

	time(&timep);
	p = localtime(&timep);
	p->tm_year = year - 1900;	
	p->tm_mon =  month -1;		
	p->tm_mday = day;
	p->tm_hour = hour;		
	p->tm_min =  minute;		
	p->tm_sec =  second;
	
	timep = mktime(p);
	gettimeofday(&tv,&tz);
	tv.tv_sec = timep;
	tv.tv_usec = 0;
	settimeofday(&tv,&tz);

	return 0;
}

int get_system_time(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
	time_t timep;
	struct tm *p;

	if (year==NULL || month==NULL || day==NULL || hour==NULL || minute==NULL || second==NULL)
	{
		return -1;
	}

	time(&timep);
	p = localtime(&timep);

	*year = p->tm_year + 1900;
	*month = p->tm_mon + 1;
	*day = p->tm_mday;
	*hour = p->tm_hour;
	*minute = p->tm_min;
	*second = p->tm_sec;

	return 0;
}

int get_week_day(int year, int month, int day)
{
	struct tm time;
	struct tm *p;
	time_t timep;
	
	memset(&time,0,sizeof(struct tm));
	time.tm_year = year - 1900;
	time.tm_mon = month - 1;
	time.tm_mday = day;

	timep = mktime(&time);
	p = localtime(&timep);
	if(p->tm_wday == 0)
	{
		return 6;
	}
	else
	{
		return p->tm_wday - 1;
	}
}

int get_day_num(int year,int month)
{
	int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};

	if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
	{
		if(month == 2)
		{
			return days[month-1] + 1;
		}
		else
		{
			return days[month-1];
		}
	}
	else
	{
		return days[month-1];
	}
}

int string_to_time(char *pStr, unsigned long *start_hour, unsigned long *start_min, unsigned long *end_hour, unsigned long *end_min)
{
	char sztmp[5];

	if (pStr==NULL || start_hour==NULL || start_min==NULL || end_hour==NULL || end_min==NULL)
	{
        return -1;
	}
	
	bzero(sztmp,5);
	memcpy(sztmp,pStr,2);
	*start_hour = atoi(sztmp);
	memcpy(sztmp,pStr+3,2);
	*start_min = atoi(sztmp);

	memcpy(sztmp,pStr+10,2);
	*end_hour = atoi(sztmp);
	memcpy(sztmp,pStr+13,2);
	*end_min = atoi(sztmp);

	return 0;
}
