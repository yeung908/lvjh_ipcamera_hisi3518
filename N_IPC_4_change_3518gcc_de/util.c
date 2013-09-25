#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>



// 设置最大录像通道数
static int g_max_channel = 1;

int set_channel_num(int num)
{
	if (num <= 0 || num > 16)
	{
		return -1;
	}
	
	g_max_channel = num;
	
	return 0;
}

int get_channel_num()
{
	return g_max_channel;
}

int getYear()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return (p->tm_year + 1900);
}

int getMonth()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return (p->tm_mon + 1);
}

int getDay()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_mday;
}

int getHour()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_hour;
}

int getMinute()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_min;
}

int getSecond()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	return p->tm_sec;
}

int getMsec()
{
	struct timeval tv;
	struct timezone tz;
	int val;
	gettimeofday(&tv,&tz);
	val = tv.tv_sec * 1000 + (int)(tv.tv_usec/1000);
	return val;
}

int getSystemTime(unsigned long *year,unsigned long *month,unsigned long *day,unsigned long *hour,unsigned long *minute,unsigned long *second)
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

int getSystemTimeExt(unsigned long *year, unsigned long *month, unsigned long *day, unsigned long *week, unsigned long *hour, unsigned long *minute, unsigned long *second)
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
	*week = p->tm_wday+1;	// changed by zhb, 2007-09-18
	*hour = p->tm_hour;
	*minute = p->tm_min;
	*second = p->tm_sec;
	//printf("Get week: %d\n", (*week));
	return 0;
}

// 星期对应整数规则:0-6 星期一 --> 星期天
int getWeek()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);

	//return ((p->tm_wday == 0) ? 6 : (p->tm_wday -1));
	return p->tm_wday; // [zhb][change][2006-04-20]
}

int getWeekDay(unsigned long year, unsigned long month, unsigned long day)
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

int getDayNum(unsigned long year,unsigned long month)
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

int SeperateTime(char *pStr, unsigned long *start_hour, unsigned long *start_min, unsigned long *end_hour, unsigned long *end_min)
{
	char sztmp[5];
	if (NULL == pStr)
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

int setSystemTime(int year, int month, int day, int hour, int minute, int second)
{
	int ret = -1;
	time_t timep;
	struct tm *p;

	time(&timep);
	p = localtime(&timep);
	p->tm_year = year - 1900;
	p->tm_mon = month -1;
	p->tm_mday = day;
	p->tm_hour = hour;
	p->tm_min = minute;
	p->tm_sec = second;

	timep = mktime(p);

	ret = stime(&timep);
	if (ret < 0)
	{
		return -1;
	}
	else
	{
		return 0;
	}
} 

int RebootSystem()
{
	system("ifconfig ra0 down");
	system("busybox reboot -f ");
	return 0;
}

int DeleteTempFile()
{
	system("busybox rm -rf /root/*");
	system("busybox rm -rf /tmp/*");
	system("busybox rm -rf /var/run/*");
	system("busybox rm -rf /var/lock/*");
	system("busybox rm -rf /var/getip");
	//system("mv /mnt/mtd/dvs/app/upnpc /usr/bin");
	//system("mv /mnt/mtd/dvs/app/encrypt_check /usr/bin");
	//system("mv -f /mnt/mtd/dvs/etc/passwd /etc/passwd");

	return 0;
}

