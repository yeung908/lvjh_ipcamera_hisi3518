/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：mp4File.h
* 文件说明：该文件描述了录像文件备份操作
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-02-05
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/
 

#ifndef __FILE_BAK_H_
#define __FILE_BAK_H_

typedef struct
{
	int channel;
	int year;
	int month;
	int day;
	int start_hour;
	int start_minute;
	int end_hour;
	int end_minute;
	int disk_type;	
	int dst_disk_no;
	
}FILE_BACKUP_INFO;

int start_backup_proc(int channel,int year,int month,int day,
					  int start_hour,int start_minute,
					  int end_hour,int end_minute,
					  int disk_type,int dst_disk_no);
int get_total_file_num();
int get_cur_file_no();
int end_backup_proc();
char *get_iso_back_dir();

#endif




 
