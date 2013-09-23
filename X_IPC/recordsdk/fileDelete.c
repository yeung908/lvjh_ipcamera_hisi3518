/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：fileDelete.c
* 文件说明：该文件描述了录像文件删除的函数定义
* 函数列表：
*    函数1：
*        函数名称：set_del_file_status
*        函数功能：设置录像文件删除状态
*    函数2：
*        函数名称：get_del_file_status
*        函数功能：获取录像文件删除状态
*    函数3：
*        函数名称：filter
*        函数功能：录像文件过虑
*    函数4：
*        函数名称：get_oldest_date
*        函数功能：获取最老录像文件的日期
*    函数5：
*        函数名称：get_total_size
*        函数功能：计算目录中所有文件的大小
*    函数6：
*        函数名称：delete_dir
*        函数功能：删除整个目录
*    函数7：
*        函数名称：del_oldest_file_fun
*        函数功能：以天为单位删除最老的录像文件(线程实现函数)
*    函数8：
*        函数名称：del_oldest_record_file
*        函数功能：开始删除最老的录像文件
*    函数9：
*        函数名称：open_search_record_file_info
*        函数功能：创建录像文件信息
*    函数10：
*        函数名称：sort_record_file_info
*        函数功能：对录像文件信息进行排序
*    函数11：
*        函数名称：close_search_record_file_info
*        函数功能：关闭录像文件信息
*    函数12：
*        函数名称：get_record_file_info_num
*        函数功能：获取符合删除条件的文件数
*    函数13：
*        函数名称：get_record_file_info_by_index
*        函数功能：获取录像文件的信息(通过索引)
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-02-05
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
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

#include "global.h"
#include "fileDelete.h"

pthread_t  g_del_file_thread;	//录像文件删除线程
int  del_file_status = 0;		//录像文件删除状态
static RECORD_FILE_INFO_LIST g_list = NULL;	//录像文件信息单向链表

/******************************************************************************
* 函数名称：set_del_file_status
* 功能描述：设置录像文件删除状态
* 输入参数：int status 录像文件删除状态(0:删除成功1:正在删除)
* 输出参数：无
* 返 回 值：无
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_del_file_status(int status)
{
	del_file_status = status;

	return 0;
}

/******************************************************************************
* 函数名称：get_del_file_status
* 功能描述：获取录像文件删除状态
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，录像文件删除状态(0:删除成功1:正在删除)
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int  get_del_file_status()
{
	return del_file_status;
}

/******************************************************************************
* 函数名称：filter
* 功能描述：录像文件过虑，根据文件名的长度，长度为RECORD_FILE_NAME_LEN
* 输入参数：const struct dirent *dir 目录指针
* 输出参数：无
* 返 回 值：匹配： 1; 不匹配： 0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
static int filter(const struct dirent *dir)
{
	if (strlen(dir->d_name) == RECORD_FILE_NAME_LEN)
	{
		//return 0;
		return 1;
	}
	else
	{
		//return -1;
		return 0;
	}
}

/******************************************************************************
* 函数名称：get_oldest_date
* 功能描述：获取最老录像文件的日期
* 输入参数：无
* 输出参数：int *year	最老录像文件的日期-年
*			int *month	最老录像文件的日期-月
*			int *day	最老录像文件的日期-日
* 返 回 值：成功： -1; 失败： 0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
static int get_oldest_date(int *year, int *month, int *day)
{
	int i,j,k,total;
	struct dirent **namelist;
	char path_name[MAX_PATH];
	char dir_name[24];
	int  disk_num;
	int channel_num = 0;

	memset(dir_name, 0xff, sizeof(dir_name));

#ifdef SD_STORAGE
	disk_num = 1;
#else
	// 遍历所有硬盘的所有分区 
	disk_num = get_hard_disk_num();
#endif

	channel_num = get_av_channel_num();
	
	for (i=0; i<disk_num; i++)
	{
#ifdef SD_STORAGE

#else
		if (hd_get_mount_flag(i,0) != 1) 		// 只搜寻数据分区 
		{
			continue;
		}
#endif

		for (j=0; j<channel_num; j++)
		{
			sprintf(path_name,"/record/hd%02d/%02d/ch%02d", i, 0, j);

			printf("%s\n", path_name);

			// 按过虑条件搜索文件夹,
			total = scandir(path_name, &namelist, filter, alphasort);
			if (total > 0)
			{
				printf("%s\n", namelist[0]->d_name);

				if (strcmp(dir_name, namelist[0]->d_name) > 0)
				{
					strcpy(dir_name, namelist[0]->d_name);
				}

				for (k=0; k<total; k++)
				{
					if (namelist[k] != NULL)
					{
 						free(namelist[k]);
					}
				}

				if (namelist != NULL)
				{
					free(namelist);
				}
			}
			else
			{
				printf("scandir: Failed!\n");
			}
			
			//usleep(50*1000);
		}
	}

	if (strlen(dir_name) > 0)
	{
		sscanf(dir_name, "%d-%d-%d", year, month, day);

		return 0;
	}
	else
	{
		return -1;
	}
}

static int filter_ext(const struct dirent *dir)
{
	if (dir->d_name[0] >= 48 && dir->d_name[0] <= 50)
	{
		printf("value: %s\n", dir->d_name);
		return 1;
	}
	else
	{
		return 0;
	}
}

static int get_oldest_time(int *year, int *month, int *day, int *hour)
{
	int i,j,k,total;
	struct dirent **namelist;
	char path_name[MAX_PATH];
	char dir_name[24];
	int  disk_num;
	int channel_num = 0;

	memset(dir_name, 0xFF, sizeof(dir_name));

	// 遍历所有硬盘的所有分区 
#ifdef SD_STORAGE
	disk_num = 1;
#else
	disk_num = get_hard_disk_num();
#endif

	channel_num = get_av_channel_num();
	
	for (i=0; i<disk_num; i++)
	{
#ifndef SD_STORAGE
		if (hd_get_mount_flag(i,0) != 1) 		// 只搜寻数据分区 
		{
			continue;
		}
#endif

		for (j=0; j<channel_num; j++)
		{
			sprintf(path_name,"/record/hd%02d/%02d/ch%02d", i, 0, j);
			// 按过虑条件搜索文件夹,
			total = scandir(path_name, &namelist, filter, alphasort);
	               //  printf(" total = %d\n", total);
			if (total > 1)
			{
				if (strcmp(dir_name,namelist[0]->d_name) > 0)
				{
					printf("dir_name = %s\n", dir_name);
					strcpy(dir_name,namelist[0]->d_name);
				}

				for (k=0; k<total; k++)
				{
					if (namelist[k] != NULL)
					{
 						free(namelist[k]);
                                                namelist[k] = NULL;
					}
				}

				if (namelist != NULL)
				{
					free(namelist);
					namelist = NULL;
					
				}
			}
			else{
				memset(dir_name, 0, 24);
				//printf("dir_name = %d\n",strlen( dir_name));

			}
			//usleep(500*1000);
		}
	}

	if (strlen(dir_name) > 0)
	{

		sscanf(dir_name, "%d-%d-%d", year, month, day);

		strcat(path_name, "/");
		strcat(path_name, dir_name);
		total = scandir(path_name, &namelist, filter_ext, alphasort);
		if (total > 0)
		{
			//printf(" namelist[0]->d_name = %s\n", namelist[0]->d_name);
			sscanf(namelist[0]->d_name, "%d", hour);

			for (k=0; k<total; k++)
			{
				if (namelist[k] != NULL)
				{
 					free(namelist[k]);
				}
			}

			if (namelist != NULL)
			{
				free(namelist);
			}
		}
		else
		{
			delete_dir(path_name);
			//printf("delete_dir scussed %s!!!\n\n", path_name);
		}
		//printf("############################total = %d path_name = %s\n dir_name= %s  \n", path_name,dir_name);
		return 0;
	}
	else
	{
		return -1;
	}
}

/******************************************************************************
* 函数名称：get_total_size
*
* 输入参数：char *dir_name
* 输出参数：unsigned int *size
* 返 回 值：成功：0; 失败：-1
* 修改记录:
* 其他说明:
********************************************************************************/
int get_total_size(char *dir_name, unsigned int *size)
{
	int ret ;
	DIR *dir;
	struct stat buf;
	struct dirent *ptr;
	char file_name[MAX_PATH];

	ret = stat(dir_name,&buf);
	
	if (S_ISDIR(buf.st_mode))
	{
		// 计算目录中所有文件的大小 
		dir = opendir(dir_name);
		while ((ptr = readdir(dir)) != NULL)
		{
			if (strcmp(ptr->d_name,".") == 0 
				|| strcmp(ptr->d_name,"..") == 0
				/*|| strcmp(ptr->d_name,"index.tm3k") == 0*/)
			{
				continue;
			}

			sprintf(file_name, "%s/%s", dir_name, ptr->d_name);
			stat(file_name, &buf);
			if (S_ISDIR(buf.st_mode))
			{
				get_total_size(file_name,size);
			}
			else
			{
				*size += buf.st_size;
			}
		}
		closedir(dir);
	}
	else
	{
		*size += buf.st_size;
	}

	return ret;
}

/******************************************************************************
* 函数名称：delete_dir
*
* 输入参数：char *dir_name
*
* 返 回 值：成功：0; 失败：-1
* 修改记录:
* 其他说明:
********************************************************************************/
int delete_dir(char *dir_name)
{
	int ret;
	DIR *dir;
	struct stat buf;
	struct dirent *ptr;
	char file_name[MAX_PATH];

	stat(dir_name,&buf);
	
	if (S_ISDIR(buf.st_mode))
	{
		dir = opendir(dir_name);
		while ((ptr = readdir(dir)) != NULL)
		{
			if (strcmp(ptr->d_name,".") == 0 
				|| strcmp(ptr->d_name,"..") == 0)
			{
				continue;
			}

			sprintf(file_name, "%s/%s", dir_name, ptr->d_name);
			stat(file_name, &buf);

			if (S_ISDIR(buf.st_mode))
			{
				delete_dir(file_name);
			}
			else
			{
				remove(file_name);
			}
		}
		closedir(dir);
		ret = remove(dir_name);
	        printf("starting delete director 2 %s \n", file_name);
	}
	else
	{
		ret = remove(dir_name);
	}

	return ret;
}

/******************************************************************************
* 函数名称：del_oldest_file_fun
* 功能描述：以天为单位删除最老的录像文件(线程实现函数)
*
*
*
* 修改记录:
* 其他说明:
********************************************************************************/
int del_oldest_file_fun()
{
	int  i,j;
	int  ret;
	int  year,month,day;
	char dir_name[MAX_PATH];
	int  cur_year, cur_month, cur_day;
	int  disk_num;
	int  channel_num = get_av_channel_num();

	cur_year   = get_year();	//获取当前时间-
	cur_month  = get_month();	//获取当前时间-
	cur_day    = get_day();		//获取当前时间-

	do
	{
		ret = get_oldest_date(&year, &month, &day);
		if (ret != 0)
		{
			printf("get_oldest_date: Failed!\n");
			del_file_status = -1;
			break;
		}

		//
		if (year < 1970 || year >2100 || month < 1 || month > 12 || day < 1 || day >31 )
		{
			del_file_status = -2;
			break;
		}

		//
		if (year == cur_year && month == cur_month && day == cur_day )
		{
			del_file_status = -3;
			break;
		}

#ifdef SD_STORAGE
		disk_num = 1;
#else
		//
		disk_num = get_hard_disk_num();
#endif
		
		for (i=0; i<disk_num; i++)
		{
#ifdef SD_STORAGE

#else
			//
			if (hd_get_mount_flag(i, 0) != 1)
			{
				continue;
			}
#endif

			//
			for (j=0; j<channel_num; j++)
			{
				sprintf(dir_name,"/record/hd%02d/%02d/ch%02d/%04d-%02d-%02d",i,0,j,year,month,day);
				printf("Delete the oldest file: %s\n", dir_name);
				delete_dir(dir_name);
			}
		}
	}while(0);

	if (del_file_status == 1)
	{
		del_file_status = 0;
	}

	//pthread_exit(NULL);

	return 0;
}

int del_oldest_file_ext_fun()
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	char dir_name[MAX_PATH];
	int  cur_year, cur_month, cur_day, cur_hour;
	int  disk_num;
	int  channel_num = get_av_channel_num();

	cur_year = get_year();		//获取当前时间-
	cur_month = get_month();	//获取当前时间-
	cur_day = get_day();		//获取当前时间-
	cur_hour = get_hour();

	do
	{
#ifdef SD_STORAGE
		
#endif

#if 1
		ret = get_oldest_time(&year, &month, &day, &hour);
		if (ret != 0)
		{
			del_file_status = -1;
			break;
		}
#endif		
		printf("the old time: %d %d %d %d\n", year, month, day, hour);
		//
		if (year < 1970 || year >2100 || month < 1 || month > 12 || day < 1 || day >31 || hour<0 || hour>=24)
		{
			del_file_status = -2;
			break;
		}
		/*
		//
		if (year == cur_year && month == cur_month && day == cur_day && hour == cur_hour)
		{
			del_file_status = -3;
			break;
		}
		*/

#ifdef SD_STORAGE
		for (j=0; j<channel_num; j++)
		{
			sprintf(dir_name,"/record/hd%02d/%02d/ch%02d/%04d-%02d-%02d/%02d",i,0,j,year,month,day,hour);
			//printf("Delete: %s\n", dir_name);
			delete_dir(dir_name);
		}
#else
		//
		disk_num = get_hard_disk_num();
		
		for (i=0; i<disk_num; i++)
		{
			//
			if (hd_get_mount_flag(i, 0) != 1)
			{
				continue;
			}

			//
			for (j=0; j<channel_num; j++)
			{
				sprintf(dir_name,"/record/hd%02d/%02d/ch%02d/%04d-%02d-%02d/%02d",i,0,j,year,month,day,hour);
				delete_dir(dir_name);
			}
		}
#endif
	}while(0);

	if (del_file_status == 1)
	{
		del_file_status = 0;
	}

	return 0;
}

/******************************************************************************
* 函数名称：del_oldest_record_file
*
*
*
* 返 回 值：成功：0; 失败：-1
* 修改记录:
* 其他说明:
********************************************************************************/
int del_oldest_record_file()
{
	int ret = 0;

	del_file_status = 1;

#if 1
	ret = pthread_create(&g_del_file_thread, NULL, (void *)del_oldest_file_fun, NULL);
	if (ret != 0)
	{
		del_file_status = -4;
	}
#endif
	

	return ret;
}

int del_oldest_record_file_ext()
{
	int ret = 0;

	del_file_status = 1;

	ret = pthread_create(&g_del_file_thread, NULL, (void *)del_oldest_file_ext_fun, NULL);
	if (ret != 0)
	{
		del_file_status = -4;
	}

	return ret;
}

/******************************************************************************
* 函数名称：open_search_record_file_info
*
*
*
* 返 回 值：0
* 修改记录:
* 其他说明:
********************************************************************************/
int open_search_record_file_info()
{
	int i,j,k,total;
	unsigned int size;
	struct dirent **namelist;
	char path_name[MAX_PATH];
	char dir_name[MAX_PATH];
	RECORD_FILE_INFO *record = NULL;
	RECORD_FILE_INFO_LIST tmp_list = NULL;
	int  language = 0;
	int  disk_num;
	int  channel_num = 0;

	channel_num = get_av_channel_num();

	while (g_list != NULL)
	{
		tmp_list = g_list->next;
		free(g_list);
		g_list = tmp_list;
	}
	tmp_list = g_list = NULL;

	memset(dir_name, 0, sizeof(dir_name));

	// 遍历所有硬盘的所有分区 
#ifdef SD_STORAGE
	disk_num = 1;
#else
	disk_num = get_hard_disk_num();
#endif
	
	for (i=0; i<disk_num; i++)
	{
#ifdef SD_STORAGE

#else
		if (hd_get_mount_flag(i, 0) != 1 )
		{
			continue;
		}
#endif

		for (j=0; j<channel_num; j++)
		{
			sprintf(path_name,"/record/hd%02d/%02d/ch%02d", i, 0, j);
			
			total = scandir(path_name,&namelist, filter, alphasort);
			
			if (total > 0)
			{
				for (k=0; k<total; k++)
				{
					size = 0;
					sprintf(dir_name,"%s/%s", path_name, namelist[k]->d_name);
					get_total_size(dir_name, &size);

					record = (RECORD_FILE_INFO *)malloc(sizeof(RECORD_FILE_INFO));
					if (record != NULL)
					{
						sprintf(record->text, "%s CH%02d HD%02d %d M",namelist[k]->d_name, (j + 1),(i+1), size/(1024*1024));
						record->next = NULL;

						if (tmp_list == NULL)
							tmp_list = g_list = record;
						else
						{
							tmp_list->next = record;
							tmp_list = record;
						}
					}
				}

				for (k=0; k<total; k++)
				{
					free(namelist[k]);
				}

				free(namelist);
			}
		}
	}

	// 对录像文件信息进行排序 
	sort_record_file_info();

	return 0;
}

/******************************************************************************
* 函数名称：sort_record_file_info
* 功能描述：对录像文件信息进行排序
			思路:将链表结构变换成数组结构,再用简单的选择排序方法进行排序
			缺点：效率不高，在一般情况下，选择排序适用于数据个数在30个以内
			建议：用快排(在实际录像过程中，每分钟保存一个文件，故在一个文件夹中
			      会有很多文件，远远超过30个。
* 输入参数：无						
* 输出参数：无
* 返 回 值：成功：0; 失败：-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sort_record_file_info()
{
	int i,j;
	int num;
	unsigned long tmp_pointer;
	unsigned long *pointer = NULL;
	RECORD_FILE_INFO_LIST tmp_list = NULL;

	num = get_record_file_info_num();
	if (num <= 1)
	{
		return -1;
	}

	// 将链表结构变换成数组结构 
	tmp_list = g_list;
	pointer = (unsigned long *)malloc(sizeof(unsigned long)*num);
	if (pointer == NULL)
	{
		return -1;
	}

	for (i=0; i<num; i++)
	{
		pointer[i] = (unsigned long)tmp_list;
		tmp_list = tmp_list->next;
	}

	// 选择法排序 
	for (i=0; i<num-1; i++)
	{
		for (j=i+1; j<num; j++) // 找最小项 
		{
			if (strncmp(((RECORD_FILE_INFO *)pointer[i])->text,((RECORD_FILE_INFO *)pointer[j])->text,50) > 0)
			{
				tmp_pointer = pointer[i];
				pointer[i] = pointer[j];
				pointer[j] = tmp_pointer;
			}
		}
	}

	for (i=0; i<num-1; i++)
	{
		((RECORD_FILE_INFO *)pointer[i])->next = (RECORD_FILE_INFO *)pointer[i+1];
	}

	((RECORD_FILE_INFO *)pointer[num-1])->next = NULL;

	g_list = (RECORD_FILE_INFO *)pointer[0];

	free(pointer);

	return 0;
}

/******************************************************************************
* 函数名称：close_search_record_file_info
* 功能描述：关闭录像文件信息
* 输入参数：无						
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int close_search_record_file_info()
{
	RECORD_FILE_INFO_LIST tmp_list = NULL;

	// 释放查询录像文件信息的单向链表的内存
	while (g_list != NULL)
	{
		tmp_list = g_list->next;
		free(g_list);
		g_list = tmp_list;
	}
	tmp_list = g_list = NULL;

	return 0;
}

/******************************************************************************
* 函数名称：get_record_file_info_num
* 功能描述：获取符合删除条件的文件数
* 输入参数：无						
* 输出参数：无
* 返 回 值：整型值，文件数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_record_file_info_num()
{
	int num = 0;
	RECORD_FILE_INFO_LIST tmp_list = g_list;

	//逐个查询录像文件信息的单向链表
	while (tmp_list != NULL)
	{
		num++;
		tmp_list = tmp_list->next;
	}

	return num;
}

/******************************************************************************
* 函数名称：get_record_file_info_by_index
* 功能描述：获取录像文件的信息(通过索引)
* 输入参数：int index 索引						
* 输出参数：char *text 查询到的录像文件名
* 返 回 值：成功：0; 失败：-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_record_file_info_by_index(char *text,int index)
{
	int i;
	int num = 0;
	RECORD_FILE_INFO_LIST tmp_list = g_list;

	//查询录像文件信息的单向链表中文件数目
	num = get_record_file_info_num();
	if (index < 0 || index >= num)
	{
		return -1;
	}

	// 逐个查询录像文件信息的单向链表
	for (i=0; i<index; i++)
	{
		if (tmp_list != NULL)
		{
			tmp_list = tmp_list->next;
		}
	}

	strcpy(text,tmp_list->text);

	return 0;
}

