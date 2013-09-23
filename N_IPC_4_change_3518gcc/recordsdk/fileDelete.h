/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：fileDelete.h
* 文件说明：该文件描述了录像文件删除的函数声明
*           包括：
*           1．录像文件信息的数据结构定义
*           2．录像文件删除的函数声明
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

#ifndef __FILE_DELETE_H_
#define __FILE_DELETE_H_

#define MAX_TEXT_INFO_LEN			64
#define RECORD_FILE_NAME_LEN 		10

typedef struct __RECORD_FILE_INFO
{
	char text[MAX_TEXT_INFO_LEN];
	struct __RECORD_FILE_INFO *next;

}RECORD_FILE_INFO, *RECORD_FILE_INFO_LIST;

//返回值定义：
//	0:删除成功
//	1:正在删除
//错误码定义:
//  -1:get_oldest_date出错
//  -2:搜寻到的最老文件的年月日出错
//  -3:不能删除当天的文件
//  -4:创建删除线程出错

int get_total_size(char *dir_name,unsigned int *size);	// 计算目录中所有文件的大小 
int del_oldest_record_file();						// 以天为单位删除最老的录像文件 
int del_oldest_record_file_ext();
int open_search_record_file_info();
int sort_record_file_info();						// 对录像文件信息进行排序 
int close_search_record_file_info();
int get_record_file_info_num();
int get_record_file_info_by_index(char *text,int index);
int delete_dir(char *dir_name);
int set_del_file_status(int status);
int  get_del_file_status(void);

#endif

