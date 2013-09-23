/* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：sdCard.c
* 文件说明：该文件描述了操作硬盘的函数的定义
* 函数列表：
*    函数1：
*        函数名称：get_hard_disk_info
*        函数功能：获取硬盘信息
*    函数2：
*        函数名称：get_partition_name
*        函数功能：获得分区名
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
#include <errno.h>

#include "sdCard.h"
#include "fdisk.h"

#ifndef MAX_PATH
#define MAX_PATH	256
#endif

static SD_CARD_INFO g_sd_card_info;
static char g_sd_dev_name[128];

static char *g_usb_dev_name1 = "/dev/scsi/host0/bus0/target0/lun0/disc";	// Add the code by lvjh, 2008-03-12
static char *g_usb_dev_name2 = "/dev/scsi/host0/bus0/target0/lun0/part1";	// Add the code by lvjh, 2008-08-18

static char *g_sd_dev_name1 = "/dev/mmc/blk0/disc";	// Add the code by lvjh, 2008-03-12
static char *g_sd_dev_name2 = "/dev/mmc/blk0/part1";

static char *g_sd_mount_name = "/record/hd00/00/";

static int fdisk_format_process = 100;			//硬盘格式化进度
static int fdisk_format_status  = 0;			//硬盘格式化状态
static int g_file_delete_status = 0;			//文件删除状态

/******************************************************************************
* 函数名称：sd_check
* 功能描述：检测USB盘是否存在
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值,USB盘的个数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_check()
{
	int fd = -1;

	fd = open(g_sd_dev_name, O_RDWR);
	if (fd < 0)
	{
		fd = open(g_sd_dev_name, O_RDONLY);
		if (fd < 0)
		{
			return -1;
		}
		close(fd);

		return 1;
	}

	close(fd);
	
	return 0;
}

/******************************************************************************
* 函数名称：sd_mount
* 功能描述：MOUNT分区
* 输入参数：char *source	硬盘分区设备名字
*			char *target	硬盘分区分区MOUNT目标名字
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
/*
int sd_mount()
{
	int ret = -1;
	int i = 0;

	ret = tm3k_HD_IsDiskFormated(g_sd_dev_name);
	if (ret < 0)
	{
		g_sd_card_info.formated = 0;
		
		//return -1;
	}

	g_sd_card_info.formated = 1;
	
	do
	{
		ret = mount(g_sd_dev_name, g_sd_mount_name, "vfat", 0, NULL);
		if (ret != 0)
		{
			printf("Mount1(%s %s %d): Failed!\n", g_sd_dev_name, g_sd_mount_name, ret);
			printf("Mount1: %d %s\n", errno, strerror(errno));

			// Add the code by lvjh, 2008-05-14
			ret = mount(g_sd_dev_name, g_sd_mount_name, "vfat", MS_RDONLY, NULL);
			if (ret != 0)
			{
				printf("Mount2(%s %s %d): Failed!\n", g_sd_dev_name, g_sd_mount_name, ret);
				printf("Mount2: %d %s\n", errno, strerror(errno));
			}
			else
			{
				g_sd_card_info.rdwr_flag = 1;
				g_sd_card_info.mount_flag = 1;
				return 0;
			}
			usleep(100*1000);
		}
		else
		{
			g_sd_card_info.rdwr_flag = 0;
			g_sd_card_info.mount_flag = 1;

			return 0;
		}
		
		i++;
	}while(i < 10 ); //最多尝试10次

	return -1;
}
*/

int sd_mount()
{
	int ret = -1;
	int i = 0;
	
	// 
	do
	{
		ret = mount(g_sd_dev_name1, g_sd_mount_name, "vfat", 0, NULL);
		if (ret != 0)
		{
			//printf("Mount1(%s %s %d)(%d %s): Failed!\n", g_sd_dev_name1, g_sd_mount_name, ret, errno, strerror(errno));

			// Add the code by lvjh, 2008-05-14
			ret = mount(g_sd_dev_name1, g_sd_mount_name, "vfat", MS_RDONLY, NULL);
			if (ret != 0)
			{
				//printf("Mount2(%s %s %d)(%d %s): Failed!\n", g_sd_dev_name1, g_sd_mount_name, ret, errno, strerror(errno));
			}
			else
			{
				g_sd_card_info.rdwr_flag = 0;
				g_sd_card_info.mount_flag = 1;
			
				strcpy(g_sd_dev_name, g_sd_dev_name1);
				
				goto CHECK_FORMAT;
			}
			usleep(10*1000);
		}
		else
		{
			g_sd_card_info.rdwr_flag = 1;
			g_sd_card_info.mount_flag = 1;

			strcpy(g_sd_dev_name, g_sd_dev_name1);

			goto CHECK_FORMAT;
		}
		
		i++;
	}while(i < 2 ); //最多尝试2次
	
	do
	{
		ret = mount(g_sd_dev_name2, g_sd_mount_name, "vfat", 0, NULL);
		if (ret != 0)
		{
			//printf("Mount1(%s %s %d)(%d %s): Failed!\n", g_sd_dev_name2, g_sd_mount_name, ret, errno, strerror(errno));

			// Add the code by lvjh, 2008-05-14
			ret = mount(g_sd_dev_name2, g_sd_mount_name, "vfat", MS_RDONLY, NULL);
			if (ret != 0)
			{
				//printf("Mount2(%s %s %d)(%d %s): Failed!\n", g_sd_dev_name2, g_sd_mount_name, ret, errno, strerror(errno));
			}
			else
			{
				g_sd_card_info.rdwr_flag = 0;
				g_sd_card_info.mount_flag = 1;

				strcpy(g_sd_dev_name, g_sd_dev_name2);
				
				goto CHECK_FORMAT;
			}
			usleep(10*1000);
		}
		else
		{
			g_sd_card_info.rdwr_flag = 1;
			g_sd_card_info.mount_flag = 1;

			strcpy(g_sd_dev_name, g_sd_dev_name2);

			goto CHECK_FORMAT;
		}
		
		i++;
	}while(i < 10 ); //最多尝试10次
	
	do
	{
		ret = mount(g_usb_dev_name1, g_sd_mount_name, "vfat", 0, NULL);
		if (ret != 0)
		{
		//	printf("Mount1(%s %s %d)(%d %s): Failed!\n", g_usb_dev_name1, g_sd_mount_name, ret, errno, strerror(errno));

			// Add the code by lvjh, 2008-05-14
			ret = mount(g_usb_dev_name1, g_sd_mount_name, "vfat", MS_RDONLY, NULL);
			if (ret != 0)
			{
			//	printf("Mount2(%s %s %d)(%d %s): Failed!\n", g_usb_dev_name1, g_sd_mount_name, ret, errno, strerror(errno));
			}
			else
			{
				g_sd_card_info.rdwr_flag = 0;
				g_sd_card_info.mount_flag = 1;
			
				strcpy(g_sd_dev_name, g_usb_dev_name1);
				
				goto CHECK_FORMAT;
			}
			usleep(10*1000);
		}
		else
		{
			g_sd_card_info.rdwr_flag = 1;
			g_sd_card_info.mount_flag = 1;

			strcpy(g_sd_dev_name, g_usb_dev_name1);

			goto CHECK_FORMAT;
		}
		
		i++;
	}while(i < 10 ); //最多尝试10次
	
	do
	{
		ret = mount(g_usb_dev_name2, g_sd_mount_name, "vfat", 0, NULL);
		if (ret != 0)
		{
		//	printf("Mount1(%s %s %d)(%d %s): Failed!\n", g_usb_dev_name2, g_sd_mount_name, ret, errno, strerror(errno));

			// Add the code by lvjh, 2008-05-14
			ret = mount(g_usb_dev_name2, g_sd_mount_name, "vfat", MS_RDONLY, NULL);
			if (ret != 0)
			{
				//printf("Mount2(%s %s %d)(%d %s): Failed!\n", g_usb_dev_name2, g_sd_mount_name, ret, errno, strerror(errno));
			}
			else
			{
				g_sd_card_info.rdwr_flag = 0;
				g_sd_card_info.mount_flag = 1;

				strcpy(g_sd_dev_name, g_usb_dev_name2);
				
				goto CHECK_FORMAT;
			}
			usleep(10*1000);
		}
		else
		{
			g_sd_card_info.rdwr_flag = 1;
			g_sd_card_info.mount_flag = 1;

			strcpy(g_sd_dev_name, g_usb_dev_name2);

			goto CHECK_FORMAT;
		}
		
		i++;
	}while(i < 10 ); //最多尝试10次

	strcpy(g_sd_dev_name, g_sd_dev_name1);
	

	return -1;

CHECK_FORMAT:
	ret = tm3k_HD_IsDiskFormated(g_sd_dev_name);
	if (ret < 0)
	{
		g_sd_card_info.formated = 0;
		
		//return -1;
	}

	
	system("mount -o remount,rw /record/hd00/00/");
	g_sd_card_info.formated = 1;

	return 0;
}

/******************************************************************************
* 函数名称：sd_umount
* 功能描述：UMOUNT分区
* 输入参数：char *target	硬盘分区分区UMOUNT目标名字
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_umount()
{
	int ret = -1;
	int i = 0;
	char cmd[128];

	// Add the code by Jerry, 2008-11-10
	if (g_sd_card_info.mount_flag == 0)
	{
		return 0;
	}
	
	sprintf(cmd, "umount %s", g_sd_mount_name);

	do
	{
		//ret = umount(g_sd_mount_name);
		ret = umount2(g_sd_mount_name, MNT_FORCE);
		if(ret != 0)
		{
			printf("UnMount(%s): Failed!\n", g_sd_mount_name);
			usleep(100*1000);
		}
		else
		{
			g_sd_card_info.mount_flag = 0;

			printf("sd_umount(): OK!\n");
			
			return 0;
		}

		i++;
	}while(i < 10 );	//最多尝试10次

	printf("sd_umount(): Failed!\n");

	return -1;
}

/******************************************************************************
* 函数名称：sd_get_info
* 功能描述：获取硬盘信息
* 输入参数：无
* 输出参数：SD_CARD_INFO
* 返 回 值：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_info(SD_CARD_INFO *sd_card_info)
{
	if (sd_card_info == NULL)
	{
		return -1;
	}
	if (sd_check() < 0)
	{
		return -1;
	}
	
	memcpy(sd_card_info, &g_sd_card_info, sizeof(SD_CARD_INFO) );
	
	return 0;
}

int sd_get_mount_flag()
{
	return g_sd_card_info.mount_flag;
}

int sd_get_readwrite_flag()
{
	return 1;
}

/******************************************************************************
* 函数名称：sd_get_disk_formated
* 功能描述：获取硬盘格式化标志
* 输入参数：无
* 输出参数：无
* 返 回 值：整型值，格式化标志
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_formated()
{
	return g_sd_card_info.formated;
}

/******************************************************************************
* 函数名称：sd_fdisk
* 功能描述：单个硬盘分区
* 输入参数：无
* 输出参数：无
* 返 回 值：成功:0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_fdisk()
{
	int ret = -1;

	ret = sd_check();
	if (ret != 0)
	{
		return -1;
	}

	ret = sd_umount();
	if (ret < 0)
	{
		return -1;
	}

	// 分区格式化该硬盘 
	ret = tm3k_HD_FDisk(g_sd_dev_name, 100, 0);
	if (ret < 0 )
	{
		sd_mount();
		
		return -1;
	}

	sd_mount();

	return 0;
}

/******************************************************************************
* 函数名称：sd_format
* 功能描述：格式化单一分区
* 输入参数：无
* 输出参数：无
* 返 回 值：成功:0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_format()
{
	int ret = -1;
	char buffer[128];

	ret = sd_check();
	if (ret != 0)
	{
		return -1;
	}
	// Add the code by lvjh, 2009-05-27
	printf("g_sd_card_info.rdwr_flag = %d\n", g_sd_card_info.rdwr_flag);
#if 0
	if (g_sd_card_info.rdwr_flag != 1)
	{
		printf("sd_format error!!\n");
		return -1;
	}		
#endif

	ret = sd_umount();
	if (ret < 0 )
	{
		return -1;
	}

	// 分区格式化该硬盘 
	//ret = tm3k_HD_Format(g_sd_dev_name, 1);
	sprintf(buffer, "/mnt/mtd/dvs/app/mkdosfs %s -I", g_sd_dev_name);	// Add the code by lvjh, 2008-04-26
	ret = system(buffer);
	if (ret < 0 )
	{
		sd_mount();
		
		return -1;
	}

	sd_mount();

	return 0;
}

/******************************************************************************
* 函数名称：sd_get_full
* 功能描述：检查硬盘是否满
* 输入参数：int disk_no				硬盘号
*			int partition_no		分区号
* 输出参数：无
* 返 回 值：未满:0; 满: 1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_full()
{
	DISKSIZE disk_size;
	
	tm3k_HD_GetDiskInfo(g_sd_dev_name, &disk_size);
	g_sd_card_info.availabe_size = disk_size.availablesize;

	printf("sd_get_full: %d %d\n", g_sd_card_info.total_size, g_sd_card_info.availabe_size);

	// 当硬盘空间小于总容量的5%,则认为这个硬盘没有空间了 
	//if (disk_size.availablesize < g_sd_card_info.total_size/20)
	//if (disk_size.availablesize < g_sd_card_info.total_size/3)  // test
	if (disk_size.availablesize < 100)  // add the code by lvjh, 2008-10-29
	{
		return 1;
	}

	return 0;
}

int sd_query_info()
{
	int ret = -1;
	unsigned long size = 0;
	DISKSIZE disk_size;

	ret = sd_check();
	if (ret < 0)
	{
		printf("sd_check(): Failed!\n");

		g_sd_card_info.total_size = 0;
		g_sd_card_info.used_size = 0;
		g_sd_card_info.availabe_size = 0;
		g_sd_card_info.used_parent = 0;

		return -1;
	}

	if (g_sd_card_info.mount_flag)
	{
		ret = tm3k_HD_GetDiskInfo(g_sd_dev_name, &disk_size);
	}
	else
	{
		ret = tm3k_HD_GetDiskInfo_ext(g_sd_dev_name, &size);	// Add the code by lvjh, 2008-05-14
	}

	if (ret < 0 )
	{
		printf("tm3k_HD_GetDiskInfo(%s): Failed!\n", g_sd_dev_name);
		return -1;
	}
	g_sd_card_info.rdwr_flag = 1;

	if (g_sd_card_info.mount_flag)	
	{
	//	if (g_sd_card_info.rdwr_flag == 1)	// Add the code by lvjh, 2009-05-27
		{
			g_sd_card_info.total_size = disk_size.totalsize;
			g_sd_card_info.used_size = disk_size.usedsize;
			g_sd_card_info.availabe_size = disk_size.availablesize;
			g_sd_card_info.used_parent = disk_size.usedpercent;
		}
#if 0		
		else
		{
			printf("SD Cord Only Read!\n");
			g_sd_card_info.total_size = disk_size.totalsize;
			g_sd_card_info.used_size = disk_size.usedsize;
			g_sd_card_info.availabe_size = 0;
			g_sd_card_info.used_parent = disk_size.usedpercent;
		}
#endif

	}
	else
	{
		g_sd_card_info.total_size = size;
		g_sd_card_info.used_size = 0;
		g_sd_card_info.availabe_size = 0;
		g_sd_card_info.used_parent = 0;
	}

	printf("SD: %d %d %d %d\n", g_sd_card_info.total_size, g_sd_card_info.used_size, g_sd_card_info.availabe_size, g_sd_card_info.used_parent);

	return 0;
}

/******************************************************************************
* 函数名称：sd_get_total_size
* 功能描述：获取分区总容量
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区总容量
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_total_size()
{
	DISKSIZE disk_size;

	tm3k_HD_GetDiskInfo(g_sd_dev_name, &disk_size);

	g_sd_card_info.total_size = disk_size.totalsize;

	return disk_size.totalsize;
}

/******************************************************************************
* 函数名称：sd_get_used_size
* 功能描述：获取分区已用空间
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区已用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_used_size()
{
	DISKSIZE disk_size;

	tm3k_HD_GetDiskInfo(g_sd_dev_name, &disk_size);

	g_sd_card_info.used_size = disk_size.usedsize;

	return disk_size.usedsize;
}

/******************************************************************************
* 函数名称：sd_get_available_size
* 功能描述：获取分区可用空间
* 输入参数：int disk_no			硬盘号
*			int partition_no	分区号
* 输出参数：无
* 返 回 值：整型值，分区可用空间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int sd_get_available_size()
{
	DISKSIZE disk_size;

	tm3k_HD_GetDiskInfo(g_sd_dev_name, &disk_size);

	g_sd_card_info.availabe_size = disk_size.availablesize;

	return disk_size.availablesize;
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
int sd_get_fdisk_format_process()
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
int sd_get_fdisk_format_status()
{
	return fdisk_format_status;
}

