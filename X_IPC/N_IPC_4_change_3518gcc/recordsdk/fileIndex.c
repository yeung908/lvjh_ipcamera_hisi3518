/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：indexFile.c
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>

#include "fileIndex.h"

#include "fileQuery.h"


static fileIndexInfo *g_fileIndexInfo = NULL;	//索引文件信息

/******************************************************************************
* 函数名称：init_file_index_func
* 功能描述：初始化系统索引文件的回调函数，该函数是线程回调函数，在系统初始化时
*			创建文件索引线程.
* 输入参数：void *file_index_info	
* 输出参数：无
* 返 回 值：无
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
void *init_file_index_func(void *file_index_info)
{	
	int hd_num;
	int partition_num;
	int channel_num;
	FILE *fp = NULL;
	char index_name_full[64];
	char hd_name[16];
	char partition_name[16];
	char channel_name[16];
	char data[16];
	char file_name[16];
	char full_path[128];
	DIR *dir = NULL;
	struct dirent *ptr;
	pthread_mutexattr_t attr;

	//传递回调函数的参数,提高模块的独立性
	if (file_index_info == NULL)
	{
		printf("(init_file_index_func): not parameter.\n");
		return;
	}
	else
	{
		g_fileIndexInfo = (fileIndexInfo *)malloc(sizeof(fileIndexInfo));
		if (g_fileIndexInfo == NULL)
		{
			printf("(init_file_index_func): memory allocate fail.\n");			
			return;
		}
		else
		{	
			memset(g_fileIndexInfo, 0, sizeof(fileIndexInfo));
			memcpy(g_fileIndexInfo, (fileIndexInfo *)file_index_info, sizeof(fileIndexInfo));
		}
	}

	//判断系统是否有硬盘
	if (g_fileIndexInfo->cur_disk == NULL)
	{
		printf("(init_file_index_func):System not hard disk.\n");
		free(g_fileIndexInfo);
		return;
	}
	
	//创建互斥变量
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_FAST_NP);	// [ZHB]
	pthread_mutex_init(&g_fileIndexInfo->index_mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	//创建索引文件
	sprintf(index_name_full, "/%s/%s", g_fileIndexInfo->cur_disk, g_fileIndexInfo->index_name);
	fp = fopen(index_name_full, "w+");
	if (fp == NULL)
	{
		if (g_fileIndexInfo)
		{		
			free(g_fileIndexInfo);
		}
		printf("(init_file_index_func):Can not open the index file(%s).\n", index_name_full);
		return;
	}

	//硬盘
	for (hd_num=0; hd_num<8; hd_num++)
	{
		//写硬盘号
		sprintf(hd_name, "[HD%02d]\n", hd_num);
		fputs(hd_name, fp);

		//检查硬盘MOUNT
		if(!g_fileIndexInfo->disk_mount_flag[hd_num])
		{
			fputs("\tNULL\n\n", fp);
			continue;
		}

		//分区
		for (partition_num=0; partition_num<2; partition_num++)
		{
			//写分区号
			if (partition_num == 0)
			{			
				//partition_name = "\t[DATA PARTITION]\n";
				strcpy(partition_name, "\t[DATA PARTITION]\n");
			}
			else
			{
				//partition_name = "\t[BACKUP PARTITION]\n";
				strcpy(partition_name, "\t[BACKUP PARTITION]\n");
			}
			fputs(partition_name, fp);
			
			dir = opendir(full_path);
			if (dir == NULL)
			{
				fputs("\t\t[NULL]\n\n", fp);
				continue;	
			}
			
			//通道号
			for (channel_num=0; channel_num<16; channel_num++)
			{
				//写通道号
				sprintf(channel_name, "\t\t[CHANNEAL%02d]", channel_num);
				fputs(channel_name, fp);
			
				dir = opendir(full_path);
				if (dir == NULL)
				{
					fputs("\t\t[NULL]\n\n", fp);
					continue;	
				}				
			}
			
			while ((ptr=readdir(dir)) != NULL)
			{
				//空目录
				if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0 )
				{
						
				}
			}
		}
	}

	return ;
}

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
	struct stat buf;
	RECORD_FILE_INDEX index;
	int num = 0;
	
	if (minute < 0 || minute > 59)
		return -1;
	
	memset(&index,0,sizeof(RECORD_FILE_INDEX));
	
	if (stat(file_name,&buf) != -1) /* 文件存在 */
	{
		file = fopen(file_name, "rb");
		if (file == NULL)
		{
			printf("open index file = %s failed \n",file_name);
			return -1;
		}

		num = fread(&index,1,sizeof(RECORD_FILE_INDEX),file);
		if (num < sizeof(RECORD_FILE_INDEX))
			memset(&index,0,sizeof(RECORD_FILE_INDEX));
		fclose(file);	
	}
		
	index.file_type[minute] = file_type;
	file = fopen(file_name, "wb");
	if (file == NULL)
	{
		printf("open index file = %s failed \n",file_name);
		return -1;
	}
	fseek(file,0,SEEK_SET);
	fwrite(&index,1,sizeof(RECORD_FILE_INDEX),file);
	fflush(file);
	sync();
	fclose(file);

	return 0;
}

/******************************************************************************
* 函数名称：search_record_index_file_by_type
* 功能描述：通过索引文件来查询文件类型
* 输入参数：char *file_name		文件名
*			int file_type		文件类型
* 输出参数：无
* 返 回 值：成功: 1; 失败: 0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int search_record_index_file_by_type(char *file_name,int file_type)
{
	FILE * file = NULL;
	struct stat buf;
	RECORD_FILE_INDEX index;
	int i,num = 0;
	
	if (stat(file_name, &buf) != 0) /*            */
	{
		printf("the file (%s) is not exist!\n");
		return 0;
	}
		
	file = fopen(file_name,"rb");
	if(file == NULL)
	{
		printf("open index file = %s failed \n",file_name);
		return 0;
	}
	
	num = fread(&index,1,sizeof(RECORD_FILE_INDEX),file);
	fclose(file);
	if (num < sizeof(RECORD_FILE_INDEX))
	{
		return 0;
	}
		
	for (i=0; i<60; i++)
	{
		if ((index.file_type[i] & file_type) != 0)
		{
			return 1;
		}
	}	
	
	return 0;
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
int read_record_index_file(char *file_name,RECORD_FILE_INDEX *index)
{
	FILE * file = NULL;
	int num = 0;
		
	file = fopen(file_name,"rb");
	if(file == NULL)
	{
		return -1;
	}
	
	num = fread(index,1,sizeof(RECORD_FILE_INDEX),file);
	fclose(file);
	if(num < sizeof(RECORD_FILE_INDEX))
	{
		return -1;
	}
	
	return 0;
}



















