/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：indexFileExt.c
* 文件说明：该文件描述了操作录像索引文件的函数的定义
* 函数列表：
*    函数1：
*        函数名称：write_record_index_file_ext
*        函数功能：写索引文件
*    函数2：
*        函数名称：search_record_index_file_by_type_ext
*        函数功能：通过索引文件来查询文件类型
*    函数3：
*        函数名称：read_record_index_file_ext
*        函数功能：读索引文件
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2008-04-29
* 修改记录:
*   修改1     日    期:
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
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "indexFileExt.h"

/******************************************************************************
* 函数名称：write_record_index_file
* 功能描述：写索引文件
* 输入参数：char *indexFile		文件名
*			int minute			分钟
*			int file_type		文件类型
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int write_record_index_file_ext(char *indexFile, int minute, REC_FILE_INFO recFileInfo)
{
	FILE *file = NULL;
	RECORD_FILE_INDEX_EXT index;
	int num = 0;

	//printf("write_record_index_file_ext: %s %d (%d %d %s %d)\n", indexFile, minute, recFileInfo.nSize, recFileInfo.nPlayTime, recFileInfo.szFileName, recFileInfo.nType);

	if (indexFile == NULL)
	{
		return -1;
	}
	
	if (minute < 0 || minute >= MAX_RECORD_FILE_NUM)
	{
		return -1;
	}
	
	memset(&index, 0, sizeof(RECORD_FILE_INDEX_EXT));

	file = fopen(indexFile, "rb");
	if (file)
	{
		fread(&index, 1, sizeof(RECORD_FILE_INDEX_EXT), file);
		fclose(file);
		file = NULL;
	}

	file = fopen(indexFile, "w+b");
	if (file == NULL)
	{
		printf("write_record_index_file(Error): %s %d\n", indexFile, minute);
		return -1;
	}

	memcpy(&index.recFileInfo[minute], &recFileInfo, sizeof(REC_FILE_INFO));
	fwrite(&index, 1, sizeof(RECORD_FILE_INDEX_EXT), file);
	fflush(file);
	fclose(file);
	
	return 0;
}

/******************************************************************************
* 函数名称：search_record_index_file_by_type_ext
* 功能描述：通过索引文件来查询文件类型
* 输入参数：char *indexFile		文件名
*			int file_type		文件类型
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int search_record_index_file_by_type_ext(char *indexFile, int file_type)
{
	FILE * file = NULL;
	struct stat buf;
	RECORD_FILE_INDEX_EXT index;
	int i,num = 0;

	if (indexFile == NULL)
	{
		return -1;
	}
	/*
	if (stat(indexFile, &buf) != 0) // 文件不存在 
	{
		printf("stat: %s error!\n", indexFile);
		return -1;
	}
	*/
	file = fopen(indexFile, "rb");
	if (file == NULL)
	{
		return -1;
	}
	
	num = fread(&index, 1, sizeof(RECORD_FILE_INDEX_EXT), file);
	
	fclose(file);
	
	if (num < sizeof(RECORD_FILE_INDEX_EXT))
	{
		return -1;
	}
		
	for (i=0; i<MAX_RECORD_FILE_NUM; i++)
	{
		if ((index.recFileInfo[i].nType & file_type) != 0)
		{
			return 1;
		}
	}	
	
	return -1;
}

int search_record_index_file_by_type_ext2(char *indexFile, int file_type)
{
	struct stat buf;

	if (indexFile == NULL)
	{
		return -1;
	}
	
	if (stat(indexFile, &buf) != 0) // 文件不存在 
	{
		//printf("stat: %s error!\n", indexFile);
		return -1;
	}
	
	return 1;
}

/******************************************************************************
* 函数名称：read_record_index_file
* 功能描述：读索引文件
* 输入参数：char *indexFile				文件名		
* 输出参数：RECORD_FILE_INDEX_EXT *index	文件索引信息
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int read_record_index_file_ext(char *indexFile, RECORD_FILE_INDEX_EXT *index)
{
	FILE * file = NULL;
	int num = 0;

	if (indexFile==NULL || index==NULL)
	{
		return -1;
	}
	
	//printf("read_record_index_file_ext: %s\n", indexFile);

	file = fopen(indexFile, "rb");
	if (file == NULL)
	{
		return -2;
	}
	
	num = fread(index, 1, sizeof(RECORD_FILE_INDEX_EXT), file);
	
	fclose(file);
	
	if(num < sizeof(RECORD_FILE_INDEX_EXT))
	{
		return -3;
	}
	
	return 0;
}

int write_jpeg_index_file(char *indexFile, REC_FILE_INFO recFileInfo)
{
	FILE *file = NULL;
	int num = 0;

	printf("write_jpeg_index_file: %s (%d %d %s %d)\n", indexFile, recFileInfo.nSize, recFileInfo.nPlayTime, recFileInfo.szFileName, recFileInfo.nType);

	if (indexFile == NULL)
	{
		return -1;
	}
	
	file = fopen(indexFile, "r+b");
	if (file)
	{
		fread(&num, 1, sizeof(int), file);
		if (num < 0)
		{
			num = 0;
		}
		num++;
		fseek(file, 0, SEEK_SET);
		fwrite(&num, 1, sizeof(int), file);
		fseek(file, sizeof(int)+sizeof(REC_FILE_INFO)*(num-1), SEEK_SET);
		fwrite(	&recFileInfo, 1, sizeof(REC_FILE_INFO), file);
		fflush(file);
		fclose(file);
		file = NULL;
	}
	else
	{
		file = fopen(indexFile, "w+b");
		if (file == NULL)
		{
			printf("write_jpeg_index_file(Error): %s\n", indexFile);
			return -1;
		}
		num = 1;
		fwrite(	&num, 1, sizeof(int), file);
		fwrite(	&recFileInfo, 1, sizeof(REC_FILE_INFO), file);
		fflush(file);
		fclose(file);
		file = NULL;
	}

	return 0;
}

int read_jpeg_index_file(char *indexFile, char *recFileInfo)
{
	int ret = 0;
	FILE *file = NULL;
	int num = 0;
	char *buffer = NULL;

	if (indexFile==NULL || recFileInfo==NULL)
	{
		return -1;
	}

	file = fopen(indexFile, "rb");
	if (file == NULL)
	{
		return -2;
	}
	
	fread(&num, 1, sizeof(int), file);
	if (num <= 0)
	{
		fclose(file);
		return -3;
	}

	buffer = (char *)recFileInfo;
	ret = fread(buffer, 1, sizeof(REC_FILE_INFO)*num, file);
	
	fclose(file);
	
	printf("read_jpeg_index_file: %s %d\n", indexFile, num);

	return num;
}

int search_jpeg_index_file_by_type(char *indexFile, int file_type)
{
	int ret = 0;
	FILE *file = NULL;
	int num = 0;
	char *buffer = NULL;

	if (indexFile == NULL)
	{
		return -1;
	}

	file = fopen(indexFile, "rb");
	if (file == NULL)
	{
		return -2;
	}
	
	fread(&num, 1, sizeof(int), file);
	if (num <= 0)
	{
		fclose(file);
		return -3;
	}
	
	fclose(file);
	
	return 1;
}




















