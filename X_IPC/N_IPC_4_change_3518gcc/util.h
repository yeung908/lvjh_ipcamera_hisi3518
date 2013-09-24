#ifndef __UTIL_H_
#define __UTIL_H_

#ifndef MAX_PATH
#define MAX_PATH			128
#endif

int set_channel_num(int num);
int get_channel_num();

int getYear();
int getMonth();
int getDay();
int getHour();
int getMinute();
int getSecond();
int getMsec();
int getWeek();
int getSystemTime(unsigned long *year, unsigned long *month, unsigned long *day, unsigned long *hour, unsigned long *minute, unsigned long *second);
int getSystemTimeExt(unsigned long *year, unsigned long *month, unsigned long *day, unsigned long *week, unsigned long *hour, unsigned long *minute, unsigned long *second);
int setSystemTime(unsigned long year,unsigned long month,unsigned long day,unsigned long hour,unsigned long minute,unsigned long second);
int getWeekDay(unsigned long year,unsigned long month,unsigned long day);
int getDayNum(unsigned long year,unsigned long month);
int SeperateTime(char *pStr, unsigned long *start_hour, unsigned long *start_min, unsigned long *end_hour, unsigned long *end_min);
int RebootSystem();

#endif


