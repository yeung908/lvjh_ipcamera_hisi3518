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

#ifndef __UTIL_H_
#define __UTIL_H_

int set_av_channel_num(int num);
int get_av_channel_num();

int get_year();
int get_month();
int get_day();
int get_hour();
int get_minute();
int get_second();
int get_msec();
int get_week();
int set_system_time(int year, int month, int day, int hour, int minute, int second);
int get_system_time(int *year, int *month, int *day, int *hour, int *minute, int *second);
int get_week_day(int year, int month, int day);
int get_day_num(int year, int month);
int string_to_time(char *pStr, unsigned long *start_hour, unsigned long *start_min, unsigned long *end_hour, unsigned long *end_min);

#endif


