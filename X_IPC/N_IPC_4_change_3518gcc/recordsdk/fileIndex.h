/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：indexFile.h
* 文件说明：该文件描述了操作录像索引文件的函数声明
* 作    者：庄惠斌
*           包括：
*		1．最在录像文件数的宏定义
*		2.	RECORD_FILE_INDEX数据结构的定义
*		3．操作录像索引文件的函数声明
* 版本信息：V1.0
* 设计日期: 2007-02-07
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/
 
#ifndef __FILE_INDEX_H_
#define __FILE_INDEX_H_

#include <pthread.h>
#include "indexFile.h"

typedef struct _fileIndexInfo
{
	char cur_disk[32];				//存放索引文件的当前盘名
	char next_disk[32];			//存放索引文件的下一个盘名
	char index_name[32];			//索旨文件名	
	char disk_mount_flag[8];		//硬盘分区MOUNT标志
	pthread_mutex_t index_mutex;	//索引文件操作的互锁信号
}fileIndexInfo;

int write_record_index_file(char *file_name, int minute, int file_type);
int search_record_index_file_by_type(char *file_name, int file_type);
int read_record_index_file(char *file_name, RECORD_FILE_INDEX *index);

#endif

