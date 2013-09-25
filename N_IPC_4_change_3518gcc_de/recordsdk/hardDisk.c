/* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：hardDisk.c
* 文件说明：该文件描述了操作硬盘的函数的定义
* 函数列表：
*    函数1：
*        函数名称：get_hard_disk_info
*        函数功能：获取硬盘信息
*    函数2：
*        函数名称：get_partition_name
*        函数功能：获得分区名
*    函数3：
*        函数名称：hd_mount_partition
*        函数功能：MOUNT分区
*    函数4：
*        函数名称：hd_umount_partition
*        函数功能：UMOUNT分区
*    函数5：
*        函数名称：hd_mount_disk
*        函数功能：MOUNT硬盘
*    函数6：
*        函数名称：hd_umount_disk
*        函数功能：UMOUNT硬盘
*    函数7：
*        函数名称：set_cur_disk_no
*        函数功能：设置当前纪录录像的硬盘号
*    函数8：
*        函数名称：set_cur_partition_no
*        函数功能：设置分区号
*    函数9：
*        函数名称：get_cur_disk_no
*        函数功能：获取当前正在记录录像的硬盘号
*    函数10：
*        函数名称：get_cur_partition_no
*        函数功能：获取当前正在记录录像的分区号
*    函数11：
*        函数名称：get_hard_disk_num
*        函数功能：获取硬盘数
*    函数12：
*        函数名称：hd_get_mount_flag
*        函数功能：获取硬盘MOUNT标志
*    函数13：
*        函数名称：hd_get_disk_formated
*        函数功能：获取硬盘格式化标志
*    函数14：
*        函数名称：hd_mount_all_partition
*        函数功能：MOUNT所有硬盘的所有分区
*    函数15：
*        函数名称：hd_umount_all_partition
*        函数功能：UMOUNT所有硬盘的所有分区
*    函数16：
*        函数名称：hd_fdisk
*        函数功能：单个硬盘分区
*    函数17：
*        函数名称：hd_format
*        函数功能：格式化单一分区
*    函数18：
*        函数名称：hd_cur_partition_full
*        函数功能：检查硬盘是否满
*    函数19：
*        函数名称：hd_get_disk_info_for_frontpanel
*        函数功能：检查硬盘数量及是否满 <前面板用>
*    函数20：
*        函数名称：hd_find_empty_partition
*        函数功能：查找未满分区(仅限于数据分区)
*    函数21：
*        函数名称：hd_query_disk_info
*        函数功能：定时查询硬盘信息
*    函数22：
*        函数名称：int hd_get_disk_total_size
*        函数功能：获取所有硬盘总容量
*    函数23：
*        函数名称：hd_get_disk_used_size
*        函数功能：获取所有硬盘的已用空间
*    函数24：
*        函数名称：hd_get_disk_available_size
*        函数功能：整型值，所有硬盘的可用空间
*    函数25：
*        函数名称：hd_get_partition_total_size
*        函数功能：获取分区总容量
*    函数26：
*        函数名称：hd_get_partition_used_size
*        函数功能：获取分区已用空间
*    函数27：
*        函数名称：hd_get_partition_available_size
*        函数功能：获取分区可用空间
*    函数28：
*        函数名称：hd_get_backup_partition_num
*        函数功能：获取备份分区数(已mount成功的备份区)
*    函数29：
*        函数名称：hd_get_backup_disk_no
*        函数功能：通过备份分区号来获得备份硬盘号
*    函数30：
*        函数名称：CDRecord_GetDeviceName
*        函数功能：获取CD设备名字
*    函数31：
*        函数名称：Check_CDRecorder
*        函数功能：检测CDRecorder是否存在
*    函数32：
*        函数名称：usb_GetUSBName
*        函数功能：获取USB存储设备名字
*    函数33：
*        函数名称：check_usb_disk_num
*        函数功能：检测USB盘是否存在
*    函数34：
*        函数名称：usb_mount_partition
*        函数功能：MOUNT USB盘的分区
*    函数35：
*        函数名称：usb_umount_partition
*        函数功能：UMOUNT USB盘的分区
*    函数36：
*        函数名称：format_fun
*        函数功能：FORMAT线程处理函数
*    函数37：
*        函数名称：fdisk_fun
*        函数功能：FDISK线程处理函数
*    函数38：
*        函数名称：get_fdisk_format_process
*        函数功能：获取分区格式化操作的进度
*    函数39：
*        函数名称：get_fdisk_format_status
*        函数功能：获取分区格式化操作的结果
*    函数40：
*        函数名称：get_disk_avail_size
*        函数功能：获得已mount目录的剩余空间
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-02-07
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
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/vfs.h>

#include "hardDisk.h"
#include "fdisk.h"

#ifndef MAX_PATH
#define MAX_PATH	256
#endif

static HARD_DISK_INFO g_hard_disk_info = {0};
static int fdisk_format_process = 0;			//硬盘格式化进度
static int fdisk_format_status  = 0;			//硬盘格式化状态
static int g_file_delete_status = 0;			//文件删除状态

/******************************************************************************
* 函数名称：get_hard_disk_info
* 功能描述：获取硬盘信息
* 输入参数：无
* 输出参数：HARD_DISK_INFO *hard_disk_info 硬盘信息数据结构
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_hard_disk_info(HARD_DISK_INFO *hard_disk_info)
{
	if (hard_disk_info == NULL)
	{
		return -1;
	}
	
	memcpy( hard_disk_info, &g_hard_disk_info, sizeof(HARD_DISK_INFO) );
	
	return 0;
}

/******************************************************************************
* 函数名称：get_partition_name
* 功能描述：获得分区名,如:/dev/hde1, /dev/hde2
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：char *partition_name 分区名
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
static int get_partition_name(char *partition_name, int disk_no, int partition_no)
{
	if (partition_name == NULL)
	{
		return -1;
	}
	if (disk_no<0 || disk_no >7 || partition_no<0 || partition_no>2)
	{
		return -1;
	}

	sprintf(partition_name,"%s%d", tm3k_HD_GetDiskName(disk_no), partition_no + 1);

	return 0;
}

/******************************************************************************
* 函数名称：hd_mount_partition
* 功能描述：MOUNT分区
* 输入参数：char *source	硬盘分区设备名字
*			char *target	硬盘分区分区MOUNT目标名字
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_mount_partition(char *source, char *target)
{
	int ret = -1;
	int i = 0;

	if (source==NULL || target==NULL)
	{
		return -1;
	}	

	do
	{
		ret = mount(source, target, "vfat", 0, NULL);
		if (ret != 0)
		{
			printf("mount %s to %s failed = %d\n", source, target, i);
			sleep(1);
		}
		else
		{
			return 0;
		}
		
		i++;
	}while(i < 10 ); //最多尝试10次

	return -1;
}

/******************************************************************************
* 函数名称：hd_umount_partition
* 功能描述：UMOUNT分区
* 输入参数：char *target	硬盘分区分区UMOUNT目标名字
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_umount_partition(char *target)
{
	int ret = -1;
	int i = 0;

	if (target == NULL)
	{
		return -1;
	}	
	
	do
	{
		ret = umount(target);
		if(ret != 0)
		{
			printf("umount %s failed = %d \n", target, i);
			sleep(1);
		}
		else
		{
			return 0;
		}

		i++;
	}while(i < 10 );	//最多尝试10次

	return -1;
}

/******************************************************************************
* 函数名称：hd_mount_disk
* 功能描述：MOUNT硬盘
* 输入参数：int disk_no	硬盘号
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_mount_disk(int disk_no)
{
	int i,ret;
	int partition_num;
	char partition_name[MAX_PATH];
	char dir_name[MAX_PATH];
	int  type;				//   1: 数据分区   2: 备份分区  3: 其它  
	DISKSIZE disk_size;

	//pause_record_file(); 		// 暂停所有录像 

	if ( tm3k_HD_IsDiskFormated(tm3k_HD_GetDiskName(disk_no)) == 0 )
	{		
		partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(disk_no));
		
		for (i=0; i<partition_num; i++)
		{
			g_hard_disk_info.disk_info[disk_no].partition_info[i].format_flag = 1;	//已经格式化

			type = tm3k_HD_GetPartionVol( tm3k_HD_GetDiskName(disk_no), i + 1 );
			
			g_hard_disk_info.disk_info[disk_no].partition_info[i].type = type;

			//printf("hd_mount_disk==type = %d\n", type);

			get_partition_name(partition_name, disk_no, i);
			
			sprintf(dir_name,"/record/hd%02d/%02d",disk_no,i);
			
			ret = hd_mount_partition(partition_name,dir_name);
			if (ret == 0)
			{
				g_hard_disk_info.disk_info[disk_no].partition_info[i].mount_flag = 1;
				tm3k_HD_GetDiskInfo(partition_name, &disk_size);
				g_hard_disk_info.disk_info[disk_no].partition_info[i].availabe_size = disk_size.availablesize;
			}
			else
			{
				// 重新挂载
				hd_umount_partition(dir_name);
				ret = hd_mount_partition(partition_name,dir_name);
				if (ret == 0)
				{
					g_hard_disk_info.disk_info[disk_no].partition_info[i].mount_flag = 1;
					tm3k_HD_GetDiskInfo(partition_name,&disk_size);
					g_hard_disk_info.disk_info[disk_no].partition_info[i].availabe_size = disk_size.availablesize;
				}
			}
		}
	}
	else
	{
		g_hard_disk_info.disk_info[disk_no].partition_info[0].format_flag = 0;
		g_hard_disk_info.disk_info[disk_no].partition_info[1].format_flag = 0;
		g_hard_disk_info.disk_info[disk_no].partition_info[0].mount_flag = 0;
		g_hard_disk_info.disk_info[disk_no].partition_info[1].mount_flag = 0;
	}

	//restart_record_file(); // 重新开始所有录像 

	return 0;
}

/******************************************************************************
* 函数名称：hd_umount_disk
* 功能描述：UMOUNT硬盘
* 输入参数：int disk_no	硬盘号
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_umount_disk(int disk_no)
{
	int i,ret;
	int partition_num;
	char dir_name[MAX_PATH];

	//pause_record_file(); // 暂停所有录像 

	partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(disk_no));
	
	for (i=0; i<partition_num; i++)
	{
		sprintf(dir_name, "/record/hd%02d/%02d", disk_no, i);
		if (g_hard_disk_info.disk_info[disk_no].partition_info[i].mount_flag == 1)
		{
			ret = hd_umount_partition(dir_name);
			if (ret == 0)
				g_hard_disk_info.disk_info[disk_no].partition_info[i].mount_flag = 0;
			else
				return -1;
		}
	}

	return 0;
}

/******************************************************************************
* 函数名称：set_cur_disk_no
* 功能描述：设置当前纪录录像的硬盘号
* 输入参数：int disk_no	硬盘号
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_cur_disk_no(int disk_no)
{
	g_hard_disk_info.cur_disk_no = disk_no;
	
	return 0;
}

/******************************************************************************
* 函数名称：set_cur_partition_no
* 功能描述：设置分区号
* 输入参数：int disk_no	硬盘号
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_cur_partition_no(int partition_no)
{
	g_hard_disk_info.cur_partition_no = partition_no;
	
	return 0;
}

/******************************************************************************
* 函数名称：get_cur_disk_no
* 功能描述：获取当前正在记录录像的硬盘号
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，当前正在纪录录像的硬盘号
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_cur_disk_no()
{
	return g_hard_disk_info.cur_disk_no;
}

/******************************************************************************
* 函数名称：get_cur_partition_no
* 功能描述：获取当前正在记录录像的分区号
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，当前正在纪录录像的分区号
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_cur_partition_no()
{
	return g_hard_disk_info.cur_partition_no;
}

/******************************************************************************
* 函数名称：get_hard_disk_num
* 功能描述：获取硬盘数
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，硬盘数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_hard_disk_num()
{
	return g_hard_disk_info.hard_disk_num;
}

int get_hard_disk_num_ext()
{
	g_hard_disk_info.hard_disk_num = tm3k_HD_GetDiskNum();

	return g_hard_disk_info.hard_disk_num;
}

/******************************************************************************
* 函数名称：hd_get_mount_flag
* 功能描述：获取硬盘MOUNT标志
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，MOUNT标志
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_mount_flag(int disk_no,int partition_no)
{
	return g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].mount_flag;
}

/******************************************************************************
* 函数名称：hd_get_disk_formated
* 功能描述：获取硬盘格式化标志
* 输入参数：int disk_no			硬盘号
* 输出参数：无
* 返 回 值：整型值，格式化标志
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_disk_formated(int disk_no, int partition_no)
{
	return g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].format_flag;
}

/******************************************************************************
* 函数名称：hd_mount_all_partition
* 功能描述：MOUNT所有硬盘的所有分区
* 输入参数：无
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_mount_all_partition()
{
	int i;
	int disk_num;

	disk_num = tm3k_HD_GetDiskNum();
	
	if (disk_num == 0)
	{
		g_hard_disk_info.hard_disk_num = disk_num;
		return -1;
	}
	
	for(i=0; i<disk_num; i++)
	{
		hd_mount_disk(i); // mount 第i块硬盘 
	}
	
	g_hard_disk_info.hard_disk_num = disk_num;

	return 0;
}

/******************************************************************************
* 函数名称：hd_umount_all_partition
* 功能描述：UMOUNT所有硬盘的所有分区
* 输入参数：无
* 输出参数：无
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_umount_all_partition()
{
	int i;
	int disk_num;

	disk_num = tm3k_HD_GetDiskNum();
	
	if (disk_num == 0)
	{
		g_hard_disk_info.hard_disk_num = disk_num;
		return -1;
	}
	
	for(i=0; i<disk_num; i++)
	{
		hd_umount_disk(i); // umount 第i块硬盘 
	}

	return 0;
}

/******************************************************************************
* 函数名称：hd_fdisk
* 功能描述：单个硬盘分区
* 输入参数：int disk_no				硬盘号
*			int data_partition		数据分区的容量
*			int backup_partition	备份分区的容量
* 输出参数：无
* 返 回 值：成功:0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_fdisk(int disk_no,int data_partition,int backup_partition)
{
	int ret;
	int disk_num;
	int cur_disk_no;

	cur_disk_no = g_hard_disk_info.cur_disk_no;

	disk_num = get_hard_disk_num();
	if ( disk_no > (disk_num - 1) )
	{
		return -4;
	}

	if ( disk_no == cur_disk_no )	// 如果是当前盘
	{
		//暂停录像
		//pause_record_file();

		// umount该硬盘的所有分区 
		ret = hd_umount_disk(disk_no);
		if (ret < 0)
		{
			//恢复录像
			//restart_record_file();
			
			return -1;
		}

		// 分区格式化该硬盘 
		ret = tm3k_HD_FDisk(tm3k_HD_GetDiskName(disk_no),data_partition,backup_partition);
		if (ret != 0)
			printf("fdisk disk: %s failed \n",tm3k_HD_GetDiskName(disk_no));
		else
			printf("fdisk disk: %s success \n",tm3k_HD_GetDiskName(disk_no));

		// mount该硬盘的所有分区 
		hd_mount_disk(disk_no);

		//恢复系统
		//restart_record_file();
	}
	else		// 如果不是当前盘
	{
		// umount该硬盘的所有分区 
		ret = hd_umount_disk(disk_no);
		if(ret < 0)
		{
			return -1;
		}
		
		// 分区格式化该硬盘 
		ret = tm3k_HD_FDisk(tm3k_HD_GetDiskName(disk_no),data_partition,backup_partition);
		if (ret != 0)
			printf("fdisk disk: %s failed \n",tm3k_HD_GetDiskName(disk_no));
		else
			printf("fdisk disk: %s success \n",tm3k_HD_GetDiskName(disk_no));

		// mount该硬盘的所有分区 
		hd_mount_disk(disk_no);
	}
	return ret;
}

/******************************************************************************
* 函数名称：hd_format
* 功能描述：格式化单一分区
* 输入参数：int disk_no				硬盘号
*			int partition_no		分区号
* 输出参数：无
* 返 回 值：成功:0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_format(int disk_no,int partition_no)
{
	int ret;
	int partition_num;
	int disk_num;

	int cur_disk_no;
	int cur_partition_no;
	char dir_name[MAX_PATH];
	char dev_name[MAX_PATH];

	cur_disk_no = g_hard_disk_info.cur_disk_no;
	cur_partition_no = g_hard_disk_info.cur_partition_no;

	partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(disk_no));
	if ( partition_no > (partition_num -1) )
	{
		return -3;
	}

	disk_num = get_hard_disk_num();
	if ( disk_no > (disk_num - 1) )
	{
		return -4;
	}

	if ( (disk_no == cur_disk_no) && (partition_no == cur_partition_no) )
	{
		//暂停录像
		//pause_record_file();
		
		sprintf(dir_name,"/record/hd%02d/%02d",disk_no, partition_no);
		if (g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].mount_flag == 1)
		{
			ret = hd_umount_partition(dir_name);		// umount该分区 
			if (ret != 0)
			{
				//恢复录像
				//restart_record_file();
				return -1;
			}
		}

		ret = tm3k_HD_Format(tm3k_HD_GetDiskName(disk_no), partition_no + 1);

		get_partition_name(dev_name, disk_no, partition_no);

		hd_mount_partition(dev_name,dir_name);

		//恢复录像
		//restart_record_file();
	}
	else
	{
		sprintf(dir_name,"/record/hd%02d/%02d",disk_no, partition_no);
		if (g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].mount_flag == 1)
		{
			ret = hd_umount_partition(dir_name);		// umount该分区 
			if (ret != 0)
			{
				return -1;
			}
		}

		ret = tm3k_HD_Format(tm3k_HD_GetDiskName(disk_no), partition_no + 1);
		
		get_partition_name(dev_name, disk_no, partition_no);
		
		hd_mount_partition(dev_name,dir_name);
	}

	return ret;
}

/******************************************************************************
* 函数名称：hd_cur_partition_full
* 功能描述：检查硬盘是否满
* 输入参数：int disk_no				硬盘号
*			int partition_no		分区号
* 输出参数：无
* 返 回 值：未满:0; 满: 1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_cur_partition_full(int disk_no, int partition_no)
{
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;

	get_partition_name(partition_name,disk_no,partition_no);
	tm3k_HD_GetDiskInfo(partition_name,&disk_size);
	g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].availabe_size = disk_size.availablesize;

	// 当硬盘空间小于300M,则认为这个硬盘没有空间了 
	if (disk_size.availablesize < 300) 
	{
		return 1;
	}

	return 0;
}

/******************************************************************************
* 函数名称：hd_get_disk_info_for_frontpanel
* 功能描述：检查硬盘数量及是否满 <前面板用>
* 输入参数：无
* 输出参数：unsigned char *disk_num		硬盘存在标志
*			unsigned char *disk_full	硬盘满标志
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_disk_info_for_frontpanel(unsigned char *disk_num, unsigned char *disk_full)
{
	int i;
	int hd_num;
	unsigned char disk_num_status  = 0;
	unsigned char disk_full_status = 0;

	if (disk_num==NULL ||disk_full==NULL)
	{
		return -1;
	}
	
	hd_num = get_hard_disk_num();

	for(i = 0; i < hd_num; i++)
	{
		disk_num_status |= (1<<i);
		if (g_hard_disk_info.disk_info[i].partition_info[0].availabe_size < 300 )
		{
			disk_full_status |= (1<<i);
		}
	}

	*disk_num  = disk_num_status;
	*disk_full = disk_full_status;

	return 0;
}

/******************************************************************************
* 函数名称：hd_find_empty_partition
* 功能描述：查找未满分区(仅限于数据分区)
* 输入参数：无
* 输出参数：int *disk_no		硬盘号
*			int *partition_no	分区号
* 返 回 值：成功:0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_find_empty_partition(int *disk_no,int *partition_no)
{
	int i;
	int ret = 0;
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;
	int  disk_num;

	*disk_no = -1;
	*partition_no = -1;

	// 只查找数据分区,每个硬盘的第一个区为数据区 
	disk_num = get_hard_disk_num();
	
	for (i=0; i<disk_num; i++)
	{
		if ( g_hard_disk_info.disk_info[i].partition_info[0].format_flag == 1 )	//是否格式化
		{							
			//不为备份分区
			if (g_hard_disk_info.disk_info[i].partition_info[0].mount_flag && g_hard_disk_info.disk_info[i].partition_info[0].type != 2 )
			{
				get_partition_name(partition_name,i,0);
				
				ret = tm3k_HD_GetDiskInfo(partition_name,&disk_size);

				g_hard_disk_info.disk_info[i].partition_info[0].availabe_size = disk_size.availablesize;

				// 当硬盘空间大于300M,才认为这个硬盘有空间 
				if (disk_size.availablesize >= 300)
				{
					*disk_no = i;
					*partition_no = 0;
					break;
				}
			}
		}
	}

	if (*disk_no == -1 || *partition_no == -1)
		return -1;
	else
		return 0;
}

/******************************************************************************
* 函数名称：hd_query_disk_info
* 功能描述：定时查询硬盘信息(mainMenuDlg.c在定时器中调用)
* 输入参数：无
* 输出参数：无
* 返 回 值：2: 硬盘满暂停录像,删除最老文件
*			1: 硬盘满暂停录像,报警提示
*			0: 错误
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_query_disk_info()
{
	int cur_disk_no;
	int cur_partition_no;
	int cover_mode = 0;
	int ret = 0;
	int disk_num = 0;
	cur_disk_no = g_hard_disk_info.cur_disk_no;
	cur_partition_no = g_hard_disk_info.cur_partition_no;

	disk_num = get_hard_disk_num();
	if (disk_num <= 0)
	{
		return -1;
	}
		
	if (hd_cur_partition_full(cur_disk_no,cur_partition_no))	// 当前分区满 
	{
		if (get_del_file_status() == 1)					//正在删除文件
		{
			return 0;
		}

		if (hd_find_empty_partition(&cur_disk_no, &cur_partition_no) == 0) // 找一个空的分区 
		{
			g_hard_disk_info.cur_disk_no = cur_disk_no;
			g_hard_disk_info.cur_partition_no = cur_partition_no;

			if(g_file_delete_status == 1)
			{
				g_file_delete_status = 0;
			}
		}
		else // 没有硬盘空间,停止录像或者覆盖老的录像文件 
		{
			//cover_mode = getRecordCoverMode(); // [zhb][add][2006-03-01]
			if (cover_mode == 0)
			{
				//删除最老文件
				del_oldest_record_file();
				
				g_file_delete_status = 1;

				ret = 2;				//硬盘满暂停录像,删除最老文件 
			}
			else
			{
				//暂停系统
				g_file_delete_status = 1;
				//nvsRecorderPause();
				ret = 1;				//硬盘满暂停录像,报警提示   
			}
		}
	}
	else		// 当前分未满 
	{
		if (g_file_delete_status == 1)
		{
			g_file_delete_status = 0;
		}
		ret = 3;
	}
	return ret;
}

int hd_query_disk_info_ext()
{
	static int first_query_flag = 1;
	int i = 0;
	int j = 0;
	int cur_disk_no;
	int cur_partition_no;
	int cover_mode = 0;
	int ret = 0;
	int disk_num = 0;
	char *disk_name = NULL;
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;
	
	cur_disk_no = g_hard_disk_info.cur_disk_no;
	cur_partition_no = g_hard_disk_info.cur_partition_no;

	disk_num = get_hard_disk_num_ext();
	if (disk_num <= 0)
	{
		printf("No found hard disk!\n");
		return -1;
	}

	for (i=0; i<disk_num; i++)
	{
		if (g_hard_disk_info.disk_info[i].partition_info[0].format_flag == 0) // not formated
		{
			// Add the code by lvjh, 2008-01-10
			unsigned long disksize = 0;

			disk_name = tm3k_HD_GetDiskName(i);
			ret = tm3k_HD_GetDiskInfo_ext(disk_name, &disksize);
			if (ret < 0)
			{
				printf("tm3k_HD_GetDiskInfo_ext(%s): Failed!n", disk_name);
				continue;
			}
			else
			{
				for (j=0; j<1; j++)
				{
					g_hard_disk_info.disk_info[i].partition_info[j].mount_flag = 0;
					g_hard_disk_info.disk_info[i].partition_info[j].format_flag = 0;
					g_hard_disk_info.disk_info[i].partition_info[j].total_size = disksize;
					g_hard_disk_info.disk_info[i].partition_info[j].used_size = 0;
					g_hard_disk_info.disk_info[i].partition_info[j].availabe_size = 0;
					g_hard_disk_info.disk_info[i].partition_info[j].used_parent = 0;
				}
			}
		}
		else
		{
			for (j=0; j<2; j++)
			{
				get_partition_name(partition_name, i, j);

				printf("(%d %d)Partition Name: %s\n", i, j, partition_name);

				ret = tm3k_HD_GetDiskInfo(partition_name, &disk_size);
				if (!ret)
				{
					g_hard_disk_info.disk_info[i].partition_info[j].total_size = disk_size.totalsize;
					g_hard_disk_info.disk_info[i].partition_info[j].used_size = disk_size.usedsize;
					g_hard_disk_info.disk_info[i].partition_info[j].availabe_size = disk_size.availablesize;
					g_hard_disk_info.disk_info[i].partition_info[j].used_parent = disk_size.usedpercent;
				}		
			}
		}

	}

	/*
	if (first_query_flag)
	{
		for (i=0; i<disk_num; i++)
		{
			// Add the code by lvjh, 2008-01-10
			unsigned long disksize = 0;

			disk_name = tm3k_HD_GetDiskName(i);
			ret = tm3k_HD_GetDiskInfo_ext(disk_name, &disksize);
			if (ret < 0)
			{
				continue;
			}
			else
			{

			}

			//if (g_hard_disk_info.disk_info[i].formated == 1) // formated
			{
				for (j=0; j<2; j++)
				{
					//if (g_hard_disk_info.disk_info[i].partition_info[j].mount_flag)
					{
						get_partition_name(partition_name, i, j);

						printf("(%d %d)Partition Name: %s\n", i, j, partition_name);

						ret = tm3k_HD_GetDiskInfo(partition_name, &disk_size);
						if (!ret)
						{
							g_hard_disk_info.disk_info[i].partition_info[j].total_size = disk_size.totalsize;
							g_hard_disk_info.disk_info[i].partition_info[j].used_size = disk_size.usedsize;
							g_hard_disk_info.disk_info[i].partition_info[j].availabe_size = disk_size.availablesize;
							g_hard_disk_info.disk_info[i].partition_info[j].used_parent = disk_size.usedpercent;
						}
					}
				}
			}
		}
		first_query_flag = 0;
	}
	else
	{
		//if (g_hard_disk_info.disk_info[cur_disk_no].formated == 1) // formated
		{
			for (j=0; j<2; j++)
			{
				//if (g_hard_disk_info.disk_info[cur_disk_no].partition_info[j].mount_flag)
				{
					get_partition_name(partition_name, cur_disk_no, j);

					printf("(%d %d)Partition Name: %s\n", i, j, partition_name);

					ret = tm3k_HD_GetDiskInfo(partition_name, &disk_size);
					if (!ret)
					{
						g_hard_disk_info.disk_info[cur_disk_no].partition_info[j].total_size = disk_size.totalsize;
						g_hard_disk_info.disk_info[cur_disk_no].partition_info[j].used_size = disk_size.usedsize;
						g_hard_disk_info.disk_info[cur_disk_no].partition_info[j].availabe_size = disk_size.availablesize;
						g_hard_disk_info.disk_info[cur_disk_no].partition_info[j].used_parent = disk_size.usedpercent;
					}
				}
			}
		}
	}
	*/

	return 0;
}

/******************************************************************************
* 函数名称：int hd_get_disk_total_size
* 功能描述：获取所有硬盘总容量
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，所有硬盘总容量
* 修改记录: 无
* 其他说明: 无

********************************************************************************/
int hd_get_disk_total_size()
{
	int i,j;
	int disk_num;
	int partition_num;
	int total_size = 0;

	disk_num = get_hard_disk_num();
	for(i=0; i<disk_num; i++)
	{
		partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(i));
		for(j=0; j<partition_num; j++)
		{
			total_size += hd_get_partition_total_size(i,j);
		}
	}

	return total_size;
}

/******************************************************************************
* 函数名称：hd_get_disk_used_size
* 功能描述：获取所有硬盘的已用空间
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，所有硬盘的已用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_disk_used_size()
{
	int i,j;
	int disk_num;
	int partition_num;
	int used_size = 0;

	disk_num = get_hard_disk_num();
	
	for(i=0; i<disk_num; i++)
	{
		partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(i));
		for(j=0; j<partition_num; j++)
		{
			used_size += hd_get_partition_used_size(i,j);
		}
	}
	
	g_hard_disk_info.hard_disk_num = disk_num;
	
	return used_size;
}


/******************************************************************************
* 函数名称：hd_get_disk_available_size
* 功能描述：获取所有硬盘的可用空间
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，所有硬盘的可用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_disk_available_size()
{
	int i,j;
	int disk_num;
	int partition_num;
	int available_size = 0;

	disk_num = get_hard_disk_num();
	
	for(i=0; i<disk_num; i++)
	{
		partition_num = tm3k_HD_GetDiskPartionNum(tm3k_HD_GetDiskName(i));
		for(j=0; j<partition_num; j++)
		{
			available_size += hd_get_partition_available_size(i,j);
		}
	}

	return available_size;
}

/******************************************************************************
* 函数名称：hd_get_partition_total_size
* 功能描述：获取分区总容量
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区总容量
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_partition_total_size(int disk_no,int partition_no)
{
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;

	get_partition_name(partition_name,disk_no,partition_no);
	tm3k_HD_GetDiskInfo(partition_name,&disk_size);

	g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].availabe_size = disk_size.availablesize;

	return disk_size.totalsize;
}

/******************************************************************************
* 函数名称：hd_get_partition_used_size
* 功能描述：获取分区已用空间
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区已用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_partition_used_size(int disk_no,int partition_no)
{
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;

	get_partition_name(partition_name,disk_no,partition_no);
	tm3k_HD_GetDiskInfo(partition_name,&disk_size);

	g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].availabe_size = disk_size.availablesize;

	return disk_size.usedsize;
}

/******************************************************************************
* 函数名称：hd_get_partition_available_size
* 功能描述：获取分区可用空间
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区可用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_partition_available_size(int disk_no,int partition_no)
{
	char partition_name[MAX_PATH];
	DISKSIZE disk_size;

	get_partition_name(partition_name,disk_no,partition_no);
	tm3k_HD_GetDiskInfo(partition_name,&disk_size);

	g_hard_disk_info.disk_info[disk_no].partition_info[partition_no].availabe_size = disk_size.availablesize;

	return disk_size.availablesize;
}

/******************************************************************************
* 函数名称：hd_get_backup_partition_num
* 功能描述：获取备份分区数(已mount成功的备份区)
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，备份分区数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_backup_partition_num()
{
	int i;
	int backup_partition_num;

	backup_partition_num = 0;
	for (i=0; i<MAX_DISK_NUM; i++)
	{
		if (g_hard_disk_info.disk_info[i].partition_info[1].mount_flag)
			backup_partition_num++;
	}

	return backup_partition_num;
}

/******************************************************************************
* 函数名称：hd_get_backup_disk_no
* 功能描述：通过备份分区号来获得备份硬盘号
* 输入参数：int backup_partition_no		备份分区号
* 输出参数：无
* 返 回 值：成功：备份硬盘号; 失败：-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hd_get_backup_disk_no(int backup_partition_no)
{
	int i;
	int backup_partition_num;
	int disk_no;
	int partition_no;

	backup_partition_num = hd_get_backup_partition_num();
	if (backup_partition_num <= 0 || backup_partition_no >= backup_partition_num)
		return -1;

	disk_no = -1;
	partition_no = -1;
	for (i=0; i<MAX_DISK_NUM; i++)		//backup_partition_num
	{
		if (g_hard_disk_info.disk_info[i].partition_info[1].mount_flag)
			partition_no++;

		if (partition_no == backup_partition_no)
		{
			disk_no = i;
			return  disk_no;
		}
	}

	return disk_no;
}

/******************************************************************************
* 函数名称：get_fdisk_format_process
* 功能描述：获取分区格式化操作的进度
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，分区格式化操作的进度值
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_fdisk_format_process()
{
	return fdisk_format_process;
}

/******************************************************************************
* 函数名称：get_fdisk_format_status
* 功能描述：获取分区格式化操作的结果
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，分区格式化操作的结果
* 修改记录: 无

* 其他说明: 无
********************************************************************************/
int get_fdisk_format_status()
{
	return fdisk_format_status;
}

static long kscale(long b, long bs)
{
    return ( b * (long long) bs + KILOBYTE/2 ) / KILOBYTE;
}

static long mscale(long b, long bs)
{
    return ( b * (long long) bs + 1024 ) / MILOBYTE;
}

/******************************************************************************
* 函数名称：get_fdisk_format_status
* 功能描述：获得已mount目录的剩余空间
* 输入参数：char *mount_point MOUNT路径
* 输出参数：无
* 返 回 值：>= 0 空间大小,M为单位,< 0 mount_point不存在等
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_disk_avail_size(char *mount_point)
{
	long avail_size = 0;
	struct statfs s;

	if (statfs(mount_point, &s) != 0)
	{
		return -1;
	}

	if (s.f_blocks > 0)
	{
		avail_size  = mscale(s.f_bavail, s.f_bsize);
	}

	return avail_size;
}

//=====================================================================
//CDRecord 检测
//=====================================================================
char *CDRecorderName = "/dev/scd0";

/******************************************************************************
* 函数名称：CDRecord_GetDeviceName
* 功能描述：获取CD设备名字
* 输入参数：无
* 输出参数：无
* 返 回 值：字符串，CD设备名字(/dev/scd0)
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
char *CDRecord_GetDeviceName(void)
{
   	return CDRecorderName;
}

/******************************************************************************
* 函数名称：CDRecord_GetDeviceName
* 功能描述：检测CDRecorder是否存在
* 输入参数：无
* 输出参数：无
* 返 回 值：有: 1; 无: 0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int Check_CDRecorder()
{
	//int  fd;
	//int  ret;
	//char buf[128];
	FILE *fp = NULL;
    int  nCDRecorderNum = 0;

    if ((fp = fopen(CDRecorderName, "r")) == NULL)
	{
		nCDRecorderNum = 0;
		printf("not find cd\n");
    }
	else
	{
		fclose(fp);
		nCDRecorderNum = 1;
		printf("find cd\n");
	}
	return nCDRecorderNum;
}


//=====================================================================
//USB 检测
//=====================================================================
int   UsbIndex[4] = {-1, -1, -1, -1};
char *USBName[4]  = {"/dev/sda1", "/dev/sdb1", "/dev/sdc1", "/dev/sdd1"};

/******************************************************************************
* 函数名称：usb_GetUSBName
* 功能描述：获取USB存储设备名字
* 输入参数：无
* 输出参数：无
* 返 回 值：字符串，USB存储设备名字
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
char *usb_GetUSBName(int no)
{
    int       i;

    if( (UsbIndex[no] < 0)  ||   (UsbIndex[no] > 3))
      return NULL;

    i = UsbIndex[no];
    if( i >= 0 && i < 4)
    	return USBName[i];
    else
		return NULL;
}

/******************************************************************************
* 函数名称：CDRecord_GetDeviceName
* 功能描述：检测USB盘是否存在
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值,USB盘的个数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int check_usb_disk_num()
{
	int  fd;
	int  ret;
	int  i;
	char buf[128];
    int  nUsbNum = 0;

	UsbIndex[0] = -1;
	UsbIndex[1] = -1;
	UsbIndex[2] = -1;
	UsbIndex[3] = -1;

	for(i = 0; i < 4; i++)
	{
		fd = open(USBName[i], O_RDWR );         //| O_NOCTTY | O_NDELAY
 		if(fd >= 0)
		{
			ret = read(fd, buf, 128);
			if(ret > 0)
			{
				close(fd);
			    UsbIndex[nUsbNum] = i;
             	nUsbNum ++;
			}
			close(fd);
		}
	}
	return nUsbNum;
}

/******************************************************************************
* 函数名称：usb_mount_partition
* 功能描述：MOUNT USB盘的分区
* 输入参数：int no USB盘号(0-3)
* 输出参数：无
* 返 回 值：成功:0; 失败:-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int usb_mount_partition(int no)		//no:   0----3
{
	char usb_name[32];
	int  i = 0;

	if(no < 0 || no > 3)
	   return -1;

	if( (UsbIndex[no] < 0)  ||   (UsbIndex[no] > 3))
		return -1;

	i = UsbIndex[no];

	memset(usb_name, 0, sizeof(usb_name));
	sprintf(usb_name, "/usb%d", no);
	return hd_mount_partition(USBName[i], usb_name);
}

/******************************************************************************
* 函数名称：usb_mount_partition
* 功能描述：UMOUNT USB盘的分区
* 输入参数：int no USB盘号(0-3)
* 输出参数：无
* 返 回 值：成功:0; 失败:-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int usb_umount_partition(int no)	//no:   0----3
{
	char usb_name[32];

	if(no < 0 || no > 3)
	   return -1;

	memset(usb_name, 0, sizeof(usb_name));
	sprintf(usb_name, "/usb%d", no);

	return hd_umount_partition(usb_name);
}
