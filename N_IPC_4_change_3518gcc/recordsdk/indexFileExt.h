/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：indexFileExt.h
* 文件说明：该文件描述了操作录像索引文件的函数声明
* 作    者：庄惠斌
*           包括：
*           1．最在录像文件数的宏定义
*			2. RECORD_FILE_INDEX数据结构的定义
*           3．操作录像索引文件的函数声明
* 版本信息：V1.0
* 设计日期: 2008-04-29
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/
 
#ifndef __INDEX_FILE_EXT_H_
#define __INDEX_FILE_EXT_H_

#define MAX_RECORD_FILE_NUM	60

typedef struct
{
	unsigned long nSize;
	unsigned long nPlayTime;
	char szFileName[64];
	unsigned long nType;
	
}REC_FILE_INFO;

typedef struct __RECORD_FILE_INDEX_EXT
{
	REC_FILE_INFO recFileInfo[MAX_RECORD_FILE_NUM];		// 数组的索引代表时间的分钟数 
}RECORD_FILE_INDEX_EXT;

int write_record_index_file_ext(char *indexFile, int minute, REC_FILE_INFO recFileInfo);
int read_record_index_file_ext(char *indexFile, RECORD_FILE_INDEX_EXT *indexInfo);
int search_record_index_file_by_type_ext(char *indexFile, int file_type);

int write_jpeg_index_file(char *indexFile, REC_FILE_INFO recFileInfo);
int read_jpeg_index_file(char *indexFile, char *recFileInfo);
int search_jpeg_index_file_by_type(char *indexFile, int file_type);

#endif

