#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "indexFile.h"

/******************************************************************************
* 函数名称：write_record_index_file
* 功能描述：写索引文件
* 输入参数：char *file_name		文件名
*			int minute			分钟
*			int file_type		文件类型
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int write_record_index_file(char *file_name, int minute, int file_type)
{
	FILE * file = NULL;
	RECORD_FILE_INDEX index;
	int num = 0;

	printf("write_record_index_file: %s %d %d\n", file_name, minute, file_type);

	if (file_name == NULL)
	{
		return -1;
	}
	
	if (minute < 0 || minute >= MAX_RECORD_FILE_NUM)
	{
		return -1;
	}
	
	memset(&index, 0, sizeof(RECORD_FILE_INDEX));
	
	file = fopen(file_name, "r+b");
	if (file == NULL)
	{
		file = fopen(file_name, "w+b");
		if (file == NULL)
		{
			printf("write_record_index_file(Error): %s %d %d\n", file_name, minute, file_type);
			return -1;
		}
	}

	num = fread(&index, 1, sizeof(RECORD_FILE_INDEX), file);
	if (num < sizeof(RECORD_FILE_INDEX))
	{
		printf("fread index error(%d %d)!\n", num, sizeof(RECORD_FILE_INDEX));
		memset(&index, 0, sizeof(RECORD_FILE_INDEX));
	}

	index.file_type[minute] = file_type;
	fwrite(&index, 1, sizeof(RECORD_FILE_INDEX), file);
	fflush(file);
	fsync(file);
	fclose(file);
	
	return 0;
}

/******************************************************************************
* 函数名称：search_record_index_file_by_type
* 功能描述：通过索引文件来查询文件类型
* 输入参数：char *file_name		文件名
*			int file_type		文件类型
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int search_record_index_file_by_type(char *file_name, int file_type)
{
	FILE * file = NULL;
	struct stat buf;
	RECORD_FILE_INDEX index;
	int i,num = 0;

	//printf("%s %x\n", file_name, file_type);

	if (file_name == NULL)
	{
		return -1;
	}
	/*
	if (stat(file_name, &buf) != 0) // 文件不存在 
	{
		printf("stat: %s error!\n", file_name);
		return -1;
	}
	*/
	file = fopen(file_name, "rb");
	if (file == NULL)
	{
		//printf("fopen: %s error!\n", file_name);
		return -1;
	}
	
	num = fread(&index, 1, sizeof(RECORD_FILE_INDEX), file);
	
	fclose(file);
	
	if (num < sizeof(RECORD_FILE_INDEX))
	{
		//printf("fread(%d): %s error!\n", num, file_name);
		return -1;
	}
		
	for (i=0; i<MAX_RECORD_FILE_NUM; i++)
	{
		//printf("%d---%x---%x\n", i,  index.file_type[i], file_type);

		if ((index.file_type[i] & file_type) != 0)
		{
			return 1;
		}
	}	
	
	return -1;
}

/******************************************************************************
* 函数名称：read_record_index_file
* 功能描述：读索引文件
* 输入参数：char *file_name				文件名		
* 输出参数：RECORD_FILE_INDEX *index	文件索引信息
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int read_record_index_file(char *file_name, RECORD_FILE_INDEX *index)
{
	FILE * file = NULL;
	int num = 0;

	if (file_name==NULL || index==NULL)
	{
		return -1;
	}
	
	file = fopen(file_name, "rb");
	if (file == NULL)
	{
		return -1;
	}
	
	num = fread(index, 1, sizeof(RECORD_FILE_INDEX), file);
	
	fclose(file);
	
	if(num < sizeof(RECORD_FILE_INDEX))
	{
		return -1;
	}
	
	return 0;
}



















