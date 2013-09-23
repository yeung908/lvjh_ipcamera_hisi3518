/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：fileBak.c
* 文件说明：该文件描述了录像文件备份操作
* 函数列表：
*    函数1：
*        函数名称：start_backup_proc
*        函数功能：录像文件的备分操作
*    函数2：
*        函数名称：get_total_file_num
*        函数功能：获得待备份文件的总数
*    函数3：
*        函数名称：get_cur_file_no
*        函数功能：获得当前正在备份的文件号
*    函数4：
*        函数名称：end_backup_proc
*        函数功能：文件备份结束
*    函数5：
*        函数名称：get_iso_back_dir
*        函数功能：获取备份文件夹名
*    函数6：
*        函数名称：my_cp
*        函数功能：复制单个文件
*    函数7：
*        函数名称：get_iso_back_dir_full_flag
*        函数功能：获取备份文件夹全路径标志
*    函数8：
*        函数名称：create_iso_dir
*        函数功能：创建备份文件夹
*    函数9：
*        函数名称：delete_iso_dir
*        函数功能：删除备份文件夹
*    函数10：
*        函数名称：get_iso_back_dir
*        函数功能：获取备份文件夹名
*    函数11：
*        函数名称：file_backup_fun
*        函数功能：文件备份线程处理
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
#include <dirent.h>
#include <sys/stat.h>

#include "global.h"
#include "fileBak.h"
#include "util.h"

#define DATA_BLK_SIZE	(1024*8)					//每次读取文件的字节数

static int g_total_file_num = 0;
static int g_cur_file_no = 0;
static int g_end_cp_flag = 0;
static FILE_BACKUP_INFO g_file_backup_info = {0};
static pthread_t g_p_thread;						//备分录像文件的线程
static char g_iso_back_dir[256] = "";
static int  g_iso_backed_file_size = 0;
static int  g_iso_back_dir_full_flag = 0; 

/******************************************************************************
* 函数名称：my_cp
* 功能描述：复制单个文件
* 输入参数：char *src_file_name 源文件名
*           char *dst_file_name 目标文件名
* 输出参数：无
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
static int my_cp(char *src_file_name, char *dst_file_name)
{
	int ret = -1;
	char buf[DATA_BLK_SIZE];
	FILE *src_fd = NULL;
	FILE *dst_fd = NULL;

	src_fd = fopen(src_file_name, "r+b"); //只读打开源文件
	if (src_fd < 0)
	{
		printf("open src file: %s error!\n", src_file_name);
		return -1;
	}

	dst_fd = fopen(dst_file_name, "w+b"); //打开目标文件
	if(dst_fd < 0)
	{
		printf("open dst file: %s error!\n", dst_file_name);
		fclose(src_fd);
		return -1;
	}

	while (!feof(src_fd))
	{
		ret = fread(buf, 1, DATA_BLK_SIZE, src_fd); //读源文件
		if (ret <= 0)
		{
			break;
		}

		ret = fwrite(buf, 1, ret, dst_fd); //写目标文件
		if (ret <= 0)
		{
			break;
		}
		//fflush(dst_fd);
	}

	fclose(src_fd);
	fclose(dst_fd);

	return 0;
}

/******************************************************************************
* 函数名称：get_iso_back_dir_full_flag
* 功能描述：获取备份文件夹全路径标志
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，返回获取备份文件夹全路径标志
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_iso_back_dir_full_flag()
{
	return g_iso_back_dir_full_flag;
}

/******************************************************************************
* 函数名称：create_iso_dir
* 功能描述：创建备份文件夹
* 输入参数：无
* 输出参数：无
* 返 回 值：成功：0，失败：-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int create_iso_dir()
{
	int i = 0;
	int size = 0;
	int disk_num = get_hard_disk_num();	//获取存储硬件的个数

	for (i=0; i<disk_num; i++)
	{
		size = hd_get_partition_available_size(i, 1); // 获取备份分区可用空间的大小

		if (size >= 600*2) //可用空间>1200
		{
			sprintf(g_iso_back_dir, "/record/hd%02d/01/iso_back/",i);
			return 0;
		}
	}

	return -1;
}

/******************************************************************************
* 函数名称：delete_iso_dir
* 功能描述：删除备份文件夹
* 输入参数：无
* 输出参数：无
* 返 回 值：无
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int delete_iso_dir()
{
	if (g_iso_back_dir != "")
	{
		delete_dir((char *)(get_iso_back_dir())); //删除备分文件夹及其下的所有文件
		strcpy(g_iso_back_dir, "");
		g_iso_backed_file_size = 0;
		g_iso_back_dir_full_flag = 0;

		return 0;
	}

	return -1;
}

/******************************************************************************
* 函数名称：get_iso_back_dir
* 功能描述：获取备份文件夹名
* 输入参数：无
* 输出参数：无
* 返 回 值：字符串，备份文件夹名
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
char *get_iso_back_dir()
{
	return g_iso_back_dir;
}

/******************************************************************************
* 函数名称：file_backup_fun
* 功能描述：文件备份线程处理
* 输入参数：void *param 实际上传入FILE_BACKUP_INFO数据结构的指针
* 输出参数：无
* 返 回 值：void *
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
static void * file_backup_fun(void *param)
{
	int i,j;
	int channel,year,month,day;
	int start_hour,start_minute;
	int end_hour,end_minute,dst_disk_no;
	int disk_type;
	char src_path[MAX_PATH];
	char dst_path[MAX_PATH];
	char src_start_file_name[MAX_PATH];
	char src_end_file_name[MAX_PATH];
	char src_file_name[MAX_PATH];
	char src_full_file_name[MAX_PATH];
	char dst_file_name[MAX_PATH];
	char dst_index_name[MAX_PATH];
	char str[MAX_PATH];
	int  index_minute = 0;
	int  disk_num;

	struct dirent *ptr;
	DIR *dir = NULL;
	struct stat st;

#ifdef CD_R
	DIR *dst_dir = NULL;
#endif
	

	FILE_BACKUP_INFO *file_backup_info = (FILE_BACKUP_INFO *)param;

	channel = file_backup_info->channel;
	year    = file_backup_info->year;
	month   = file_backup_info->month;
	day     = file_backup_info->day;

	start_hour   = file_backup_info->start_hour;
	start_minute = file_backup_info->start_minute;
	end_hour     = file_backup_info->end_hour;
	end_minute   = file_backup_info->end_minute;

	dst_disk_no  = file_backup_info->dst_disk_no;
	disk_type    = file_backup_info->disk_type;

	// 查找所有硬盘的所有数据分区来获得源数据 
	disk_num = get_hard_disk_num();
	
	for (i=0; i<disk_num; i++)
	{
		if (!hd_get_mount_flag(i, 0)) //判断存储盘录像数据分区是否挂靠上
		{
			continue;
		}

		sprintf(src_path,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/",i,channel,year,month,day);
		sprintf(src_start_file_name,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/%02d/%02d-%02d-%02d.tm4",
					i,channel,year,month,day,start_hour,start_hour,start_minute,0);
		sprintf(src_end_file_name,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/%02d/%02d-%02d-%02d.tm4",
					i,channel,year,month,day,end_hour,end_hour,end_minute,59);

		for (j=start_hour; j<=end_hour; j++) // 按小时时间段 
		{
			sprintf(src_file_name, "%s%02d/", src_path,j);

			dir = opendir(src_file_name); //打开小时时间段文件夹
			if (dir == NULL)
			{
				printf("opendir failed ! dir = %s\n",src_file_name);
				continue;
			}

			while ((ptr = readdir(dir)) != NULL) //读取文件目录
			{
				if (strcmp(ptr->d_name, ".") == 0 
					|| strcmp(ptr->d_name, "..") == 0 
					|| strcmp(ptr->d_name, "index.tds") == 0)
				{
					continue;
				}

				sprintf(src_full_file_name, "%s%s", src_file_name, ptr->d_name);

				if (strcmp(src_full_file_name, src_start_file_name) < 0 
					|| strcmp(src_full_file_name, src_end_file_name) > 0)
				{
					continue;
				}

				// 创建目标目录 
				if (disk_type == 0)  //备份到硬盘
				{
					// 创建目标目录 
					sprintf(dst_path, "/record/hd%02d/01/ch%02d/", dst_disk_no, channel);
					mkdir(dst_path, 0x777);
					sprintf(dst_path, "%s%04d-%02d-%02d/", dst_path, year, month, day);
					mkdir(dst_path, 0x777);
					sprintf(dst_path, "%s%02d/", dst_path, j);
					mkdir(dst_path, 0x777);
					sprintf(dst_file_name, "%s%s", dst_path,ptr->d_name);
					sprintf(dst_index_name, "%s%s", dst_path, "index.tds");
				}
				else if (disk_type == 1)	//备份到U盘
				{
					// 创建目标目录 
					sprintf(dst_path, "/usb%01d/ch%02d/", dst_disk_no, channel);
					mkdir(dst_path,0x777);
					sprintf(dst_path, "%s%04d-%02d-%02d/", dst_path, year, month, day);
					mkdir(dst_path, 0x777);
					sprintf(dst_path, "%s%02d/", dst_path, j);
					mkdir(dst_path, 0x777);

					sprintf(dst_file_name, "%s%s", dst_path,ptr->d_name);
					sprintf(dst_index_name, "%s%s", dst_path, "index.tds");
				}
				else if(disk_type == 2) // 备份到备份区下的特定目录;为刻录
				{
					// 创建目标目录 
#ifdef CD_R					
					sprintf(dst_path, "%s", get_iso_back_dir());
					dst_dir = opendir(dst_path);
					if (dst_dir == NULL)
					{
						mkdir(dst_path, 0x777);
					}
					else
					{
						closedir(dst_dir);
					}

					mkdir(dst_path,0x777);
					sprintf(dst_path,"%sch%02d/", dst_path, channel);
					dst_dir = opendir(dst_path);
					if (dst_dir == NULL)
					{
						mkdir(dst_path,0x777);
					}
					else
					{
						closedir(dst_dir);
					}

					mkdir(dst_path,0x777);
					sprintf(dst_path,"%s%04d-%02d-%02d/", dst_path,year,month,day);
					dst_dir = opendir(dst_path);
					if (dst_dir == NULL)
					{
						mkdir(dst_path,0x777);
					}
					else
					{
						closedir(dst_dir);
					}

					mkdir(dst_path, 0x777);
					sprintf(dst_path, "%s%02d/", dst_path, j);
					dst_dir = opendir(dst_path);
					if (dst_dir == NULL)
					{
						mkdir(dst_path,0x777);
					}
					else
					{
						closedir(dst_dir);
					}

					mkdir(dst_path, 0x777);
					sprintf(dst_file_name, "%s%s", dst_path, ptr->d_name);
					sprintf(dst_index_name, "%s%s", dst_path, "index.tds");
#endif					
				}

				// 复制单个文件 
				memcpy(str, ptr->d_name, sizeof(ptr->d_name) );
       			memmove(str, &str[3], 2);
				str[2] = '\0';
				index_minute = atoi(str);
				
				if (strcmp(dst_index_name, src_full_file_name) != 0)	
				{
					my_cp(src_full_file_name, dst_file_name);
					if ( index_minute >=0 && index_minute <= 59 )
					{
						//  写索引文件
						write_record_index_file(dst_index_name, index_minute, 0xff);
					}
				}

				g_cur_file_no++;

				// 备份到备份区下的特定目录;为刻录
				if (disk_type == 2)
				{
					stat(dst_file_name, &st);

					g_iso_backed_file_size += st.st_size ;
					if (g_iso_backed_file_size >= 600*1024*1024)
					{
						g_iso_back_dir_full_flag = 1;
						return NULL;
					}
				}
				
				if (g_end_cp_flag) 
				{
					break;
				}

				usleep(1000*200); // 100 ms
			}

			closedir(dir);

			if (g_end_cp_flag)
			{
				break;
			}
		}

		if (g_end_cp_flag) //文件拷贝结束，结束线程
		{
			break;
		}
	}

	return NULL;
}

/******************************************************************************
* 函数名称：start_backup_proc
* 功能描述：开始备份数据处理
* 输入参数：int channel			要备份录像文件的通道
*			int year			要备份录像文件的年度
*			int month			要备份录像文件的月份
*			int day				要备份录像文件的日期
*			int start_hour		要备份录像文件的开始时间：小时
*			int start_minute	要备份录像文件的开始时间：分钟
*			int end_hour		要备份录像文件的结束时间：小时
*			int end_minute		要备份录像文件的结束时间：分钟
*			int disk_type		要备份录像文件的存储盘的类型
*			int dst_disk_no		要备份录像文件的存储盘号
* 输出参数：无
* 返 回 值：失败: -1; 成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int start_backup_proc(int channel,int year,int month,int day,int start_hour,int start_minute,
							int end_hour,int end_minute,int disk_type, int dst_disk_no)
{
	int ret = -1;
	struct sched_param param;

	g_end_cp_flag = 0;
	g_cur_file_no = 0;
	g_file_backup_info.channel = channel;
	g_file_backup_info.year    = year;
	g_file_backup_info.month   = month;
	g_file_backup_info.day     = day;
	g_file_backup_info.start_hour  = start_hour;
	g_file_backup_info.start_minute= start_minute;
	g_file_backup_info.end_hour    = end_hour;
	g_file_backup_info.end_minute  = end_minute;
	g_file_backup_info.disk_type   = disk_type;
	g_file_backup_info.dst_disk_no = dst_disk_no;
	
	//创建备份录像数据的线程
	ret = pthread_create(&g_p_thread,NULL,file_backup_fun,&g_file_backup_info);
	
	//设置备份录像数据的线程的优先级
	param.sched_priority = 10;
	pthread_setschedparam(g_p_thread, SCHED_OTHER, &param);
	
	return ret;
}

/******************************************************************************
* 函数名称：get_total_file_num
* 功能描述：获得待备份文件的总数(为了显示备份进度时用)
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，待备份文件的总数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_total_file_num()
{
	int i,j;
	int disk_num;
	int channel,year,month,day;
	int start_hour,start_minute;
	int end_hour,end_minute,dst_disk_no;
	char src_path[MAX_PATH];
	char src_start_file_name[MAX_PATH];
	char src_end_file_name[MAX_PATH];
	char src_file_name[MAX_PATH];
	char src_full_file_name[MAX_PATH];
	struct dirent *ptr;
	DIR *dir = NULL;

	channel = g_file_backup_info.channel;
	year = g_file_backup_info.year;
	month = g_file_backup_info.month;
	day = g_file_backup_info.day;
	start_hour = g_file_backup_info.start_hour;
	start_minute = g_file_backup_info.start_minute;
	end_hour = g_file_backup_info.end_hour;
	end_minute = g_file_backup_info.end_minute;
	dst_disk_no = g_file_backup_info.dst_disk_no;

	// 查找所有硬盘的所有数据分区来获得源数据 
	g_total_file_num = 0;
	
	disk_num = get_hard_disk_num(); //获取硬盘个数
	
	for (i=0; i<disk_num; i++)
	{
		//判断硬盘录像数据区是否挂靠上
		if (!hd_get_mount_flag(i, 0))
		{
			continue;
		}

		sprintf(src_path,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/",i,channel,year,month,day);
		sprintf(src_start_file_name,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/%02d/%02d-%02d-%02d.tm4",
					i,channel,year,month,day,start_hour,start_hour,start_minute,0);
		sprintf(src_end_file_name,"/record/hd%02d/00/ch%02d/%04d-%02d-%02d/%02d/%02d-%02d-%02d.tm4",
					i,channel,year,month,day,end_hour,end_hour,end_minute,59);

		for (j=start_hour; j<=end_hour; j++) // 按小时时间段查询 
		{
			sprintf(src_file_name, "%s%02d/", src_path, j);

			dir = opendir(src_file_name);
			if (dir == NULL)
			{
				continue;
			}

			//计算小时区间内文件的个数
			while ((ptr = readdir(dir)) != NULL)
			{
				if (strcmp(ptr->d_name,".") == 0 
					|| strcmp(ptr->d_name,"..") == 0
					|| strcmp(ptr->d_name,"index.tds") == 0)
				{
					continue;
				}

				sprintf(src_full_file_name,"%s%s", src_file_name, ptr->d_name);

				if (strcmp(src_full_file_name,src_start_file_name) < 0
					|| strcmp(src_full_file_name,src_end_file_name) > 0)
				{
					continue;
				}

				g_total_file_num++;
			}
			
			closedir(dir);
		}
	}

	return g_total_file_num;
}

/******************************************************************************
* 函数名称：get_cur_file_no
* 功能描述：获得当前正在备份的文件号(为了显示备份进度时用)
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，当前正在备份的文件号
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_cur_file_no()
{
	return g_cur_file_no;
}

/******************************************************************************
* 函数名称：end_backup_proc
* 功能描述：文件备份结束
* 输入参数：无
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int end_backup_proc()
{
	g_end_cp_flag = 1;
	
	return 0;
}

 
