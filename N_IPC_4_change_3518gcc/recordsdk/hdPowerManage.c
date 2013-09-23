/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：hdPowerManage.c
* 文件说明：该文件描述了操作硬盘的函数的定义
* 函数列表：
*    函数1：
*        函数名称：set_hd_APM
*        函数功能：设置硬盘高级电源管理
*    函数2：
*        函数名称：set_hd_standby
*        函数功能：设置硬盘的状态为挂起状态
*    函数3：
*        函数名称：set_hd_sleep
*        函数功能：设置硬盘的状态为睡眠状态
*    函数4：
*        函数名称：set_hd_idle
*        函数功能：设置硬盘的状态为空闲状态
*    函数5：
*        函数名称：restore_hd_ready_force
*        函数功能：强制恢复硬盘就绪状态
*    函数6：
*        函数名称：soft_reset_hd
*        函数功能：软复位硬盘(ATAPI)
*    函数7：
*        函数名称：hard_reset_hd
*        函数功能：硬复位硬盘(ATAPI)
*    函数8：
*        函数名称：get_hd_power_status
*        函数功能：获取硬盘电源的状态
*    函数9：
*        函数名称：set_idle_switch_standby
*        函数功能：设置硬盘的状态自动切换(空闲-挂起)
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
*        函数功能：定时查询硬盘信息(mainMenuDlg.c在定时器中调用)
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
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <linux/types.h>
#include <linux/hdreg.h>

#include "hdPowerManage.h"

static HD_POWER_MANAGE_INFO g_hd_power_manage_info;

void *hd_power_manage_proc();

/******************************************************************************
* 函数名称：set_hd_APM
* 功能描述：设置硬盘高级电源管理
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1

*			unsigned int level	高级电源管理的级别：1-255，255：取消高级电源管理
*								功能，数据越高，性能越好
* 输出参数：无
* 返 回 值：成功：0， 失败：-1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_hd_APM(const char *hd_name, unsigned int level)
{	
	#ifndef SETFEATURES_EN_APM
		#define SETFEATURES_EN_APM	0x05
	#endif
	#ifndef SETFEATURES_DIS_APM
		#define SETFEATURES_DIS_APM 0x85
	#endif
	
	int hd_fd = -1;
	struct stat hd_stat;
	unsigned int hd_APM_level = 255;
	unsigned char param[4] = {WIN_SETFEATURES,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 硬盘高级电源管理的级别:1-255
	if (level < 1)
	{
		hd_APM_level = 1;
	}
	else
	{
		if (level > 255)
		{
			hd_APM_level = 255;
		}
		else
		{
			hd_APM_level = level;
		}
	}
	
	// 设置硬盘高级电源管理的参数
	if (hd_APM_level == 255)
	{
		param[2] = SETFEATURES_DIS_APM;
	}
	else
	{
		param[1] = hd_APM_level;
		param[2] = SETFEATURES_EN_APM;
	}

	// 设置高级电源管理参数到硬盘上
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param))
	{
		printf("Can not set APM parameter to hard disk(%s).\n", hd_name);
		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：set_hd_standby
* 功能描述：设置硬盘的状态为挂起状态
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_hd_standby(const char *hd_name)
{
	#ifndef WIN_STANDBYNOW1
		#define WIN_STANDBYNOW1 0xE0
	#endif
	#ifndef WIN_STANDBYNOW2
		#define WIN_STANDBYNOW2 0x94
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param1[4] = {WIN_STANDBYNOW1,0,0,0};
	unsigned char param2[4] = {WIN_STANDBYNOW2,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 设置硬盘的状态为挂起状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1) && ioctl(hd_fd, HDIO_DRIVE_CMD, &param2))
	{
		printf("Can not set standby to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：set_hd_sleep
* 功能描述：设置硬盘的状态为睡眠状态
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_hd_sleep(const char *hd_name)
{
	#ifndef WIN_SLEEPNOW1
		#define WIN_SLEEPNOW1 0xE6
	#endif
	#ifndef WIN_SLEEPNOW2
		#define WIN_SLEEPNOW2 0x99
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param1[4] = {WIN_SLEEPNOW1,0,0,0};
	unsigned char param2[4] = {WIN_SLEEPNOW2,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 设置硬盘的状态为睡眠状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1) && ioctl(hd_fd, HDIO_DRIVE_CMD, &param2))
	{
		printf("Can not set sleep to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：set_hd_idle
* 功能描述：设置硬盘的状态为空闲状态
* 输入参数：const char *hd_name		硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_hd_idle(const char *hd_name)
{
	#ifndef WIN_SETIDLE1
		#define WIN_SETIDLE1 0xE3
	#endif
	#ifndef WIN_SETIDLE2
		#define WIN_SETIDLE2 0x97
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param1[4] = {WIN_SETIDLE1,0,0,0};
	unsigned char param2[4] = {WIN_SETIDLE2,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 设置硬盘的状态为空闲状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1) && ioctl(hd_fd, HDIO_DRIVE_CMD, &param2))
	{
		printf("Can not set idle to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：restore_hd_ready_force
* 功能描述：强制恢复硬盘就绪状态
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int restore_hd_ready_force(const char *hd_name)
{
	#ifndef WIN_IDLEIMMEDIATE
		#define WIN_IDLEIMMEDIATE 0xE1
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param[4] = {WIN_IDLEIMMEDIATE,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 强制恢复硬盘就绪状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param))
	{
		printf("Can not restore ready to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：soft_reset_hd
* 功能描述：软复位硬盘(ATAPI)
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int soft_reset_hd(const char *hd_name)
{
	#ifndef WIN_DEVICE_RESET
		#define WIN_DEVICE_RESET 0x08
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param[4] = {WIN_DEVICE_RESET,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 软复位硬盘
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param))
	{
		printf("Can not soft reset to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：hard_reset_hd
* 功能描述：硬复位硬盘(ATAPI)
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int hard_reset_hd(const char *hd_name)
{
	#ifndef HDIO_DRIVE_RESET
		#define HDIO_DRIVE_RESET 0x031c
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 硬复位硬盘
	if (ioctl(hd_fd, HDIO_DRIVE_RESET, NULL))
	{
		printf("Can not hard reset to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：get_hd_power_status
* 功能描述：获取硬盘电源的状态
* 输入参数：const char *hd_name	硬盘的名字，如：/dev/hda1
* 输出参数：无
* 返 回 值：失败：-1
*			成功：ACTIVE	活动
*				  STANDBY	挂起
*				  SLEEP		睡眠
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int get_hd_power_status(const char *hd_name)
{	
	#ifndef WIN_CHECKPOWERMODE1
		#define WIN_CHECKPOWERMODE1	0xE5
	#endif
	#ifndef WIN_CHECKPOWERMODE2
		#define WIN_CHECKPOWERMODE2 0x98
	#endif
	
	int hd_fd = -1;
	struct stat hd_stat;
	//unsigned int hd_APM_level = 255;
	unsigned char param1[4] = {WIN_CHECKPOWERMODE1,0,0,0};
	unsigned char param2[4] = {WIN_CHECKPOWERMODE1,0,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}
	
	// 获取硬盘电源的状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1) && ioctl(hd_fd, HDIO_DRIVE_CMD, &param2))	
	{
		/*
		if (errno!=EIO || param1[0]!=0 || param1[1]!=0 || param2[0]!=0 || param2[1]!=0)
		{
			printf("Can not get power status for hard disk (%s).\n", hd_name);	
			return -1;
		}
		else
		{
			return SLEEP;
		}
		*/

		close(hd_fd);
		
		return SLEEP;		
	}
	else
	{
		close(hd_fd);
		
		if (param1[2] == 255 || param2[2] == 255)
		{
			return ACTIVE;
		}
		else
		{
			return STANDBY;
		}
	}
}

/******************************************************************************
* 函数名称：set_idle_switch_standby
* 功能描述：设置硬盘的状态自动切换(空闲-挂起)
* 输入参数：const char *hd_name		硬盘的名字，如：/dev/hda1
*			unsigned char timeout	空闲超时时间
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int set_idle_switch_standby(const char *hd_name, unsigned char timeout)
{
	#ifndef WIN_SETIDLE1
		#define WIN_SETIDLE1 0xE3
	#endif
	#ifndef WIN_SETIDLE2
		#define WIN_SETIDLE2 0x97
	#endif

	int hd_fd = -1;
	struct stat hd_stat;
	unsigned char param1[4] = {WIN_SETIDLE1,timeout,0,0};
	unsigned char param2[4] = {WIN_SETIDLE2,timeout,0,0};
	
	// 判断硬盘设置是否存在
	if (stat(hd_name, &hd_stat))
	{
		printf("The device: %s not exist!\n", hd_name);
		return -1;
	}
	
	// 打开硬盘设备
	hd_fd = open(hd_name, O_RDONLY|O_NONBLOCK);
	if (hd_fd == -1)
	{
		printf("Can not open the device: %s!\n", hd_name);
		return -1;
	}

	// 设置硬盘的状态为空闲状态
	if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1) && ioctl(hd_fd, HDIO_DRIVE_CMD, &param2))
	//if (ioctl(hd_fd, HDIO_DRIVE_CMD, &param1))
	{
		printf("Can not set standby timeout to hard disk(%s).\n", hd_name);

		close(hd_fd);
		
		return -1;
	}
	else
	{
		close(hd_fd);
		
		return 0;
	}
}

/******************************************************************************
* 函数名称：get_hd_num
* 功能描述：获取系统中的硬盘个数
* 输入参数：无
* 输出参数：无
* 返 回 值：0-8,硬盘个数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
unsigned int get_hd_num()
{
	int i;
	int fd;
	int hd_num = 0;
	char hd_name[32];

	for (i = 0; i <MAX_DISK_NUM; i++)
	{
		sprintf(hd_name, "/dev/hd%c", 'a'+i);
         
		if ((fd = open(hd_name, O_RDWR)) >0)   
		{
			hd_num++;

			close(fd);
		}	          
	}
    
	return hd_num;
}

/******************************************************************************
* 函数名称：init_all_hd_power_status
* 功能描述：系统硬盘电源管理初始化
* 输入参数：无
* 输出参数：无
* 返 回 值：无
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
void init_all_hd_power_status()
{
	int fd;
	unsigned int hd_num = 0;
	char hd_name[32];
	int cur_hd_no = -1;
	int i;
	int status;

	//cur_hd_no = 0;
	//hd_num = 2;
	cur_hd_no = get_cur_disk_no();
	hd_num = get_hard_disk_num();

	memset(&g_hd_power_manage_info, 0, sizeof(HD_POWER_MANAGE_INFO));
	g_hd_power_manage_info.cur_active_disk_no = -1;
	g_hd_power_manage_info.prev_active_disk_no = -1;
	g_hd_power_manage_info.idle_disk_num = 0;
	g_hd_power_manage_info.active_disk_num = 1;
	g_hd_power_manage_info.standby_disk_num = 0;
	g_hd_power_manage_info.hd_number = hd_num;

	hd_num = 0;
	
	for (i=0; i<MAX_DISK_NUM; i++)
	{
		sprintf(hd_name, "/dev/hd%c", 'a'+i);		
		
		//判断硬盘是否存在
		if ((fd = open(hd_name, O_RDWR)) >0) 
		{
			//在设置硬盘参数之前,先硬复位硬盘,否则参数设置会失败
			printf("init_all_hd_power_status(): %s hard reset.\n", hd_name);
			hard_reset_hd(hd_name);

			g_hd_power_manage_info.hd_power_info[hd_num].hd_no = hd_num;
			strcpy(g_hd_power_manage_info.hd_power_info[hd_num].hd_name, hd_name);

			//设置硬盘自动休眠的时间
			if (set_idle_switch_standby(hd_name, STANDBY_TIMEOUT))
			{
				printf("Disable hard disk(%s) standby feature.\n", hd_name);
				g_hd_power_manage_info.hd_power_info[hd_num].standby_timeout = 0;
			}
			else
			{
				printf("Set hard disk(%s) standby timeout: %d s\n", hd_name, STANDBY_TIMEOUT*5);
				g_hd_power_manage_info.hd_power_info[hd_num].standby_timeout = STANDBY_TIMEOUT*5;
			}

			/*	
			//设置硬盘高级电源管理
			if (set_hd_APM(hd_name, APM_LEVEL))
			{
				g_hd_power_manage_info.hd_power_info[hd_num].APM_flag = 0;
				g_hd_power_manage_info.hd_power_info[hd_num].APM_level = 255;
			}
			else
			{
				g_hd_power_manage_info.hd_power_info[hd_num].APM_flag = 1;
				g_hd_power_manage_info.hd_power_info[hd_num].APM_level = APM_LEVEL;
			}
			*/
			
			//设置所有硬盘的电源状态
			if (i == cur_hd_no)
			{
				//hard_reset_hd(hd_name);
				g_hd_power_manage_info.cur_active_disk_no = hd_num;
			}
			else
			{
				//非当前盘都设置成休眠的状态
				//if (set_hd_sleep(hd_name))
				if (set_hd_standby(hd_name))
				{
					status = get_hd_power_status(hd_name);
					if (status == -1)
					{
						g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = SLEEP;
						g_hd_power_manage_info.sleep_disk_num++;
					}
					else
					{						
						switch (status)
						{
						case ACTIVE:
							g_hd_power_manage_info.active_disk_num++;
							g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = ACTIVE;
							break;

						case STANDBY:
							g_hd_power_manage_info.standby_disk_num++;
							g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = STANDBY;
							break;

						case IDLE:
							g_hd_power_manage_info.idle_disk_num++;
							g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = IDLE;
							break;

						default:
							g_hd_power_manage_info.sleep_disk_num++;
							g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = SLEEP;
							break;
						}
					}
				}
				else
				{
					g_hd_power_manage_info.hd_power_info[hd_num].cur_power_status = SLEEP;
					g_hd_power_manage_info.sleep_disk_num++;
				}
			}			
			
			memset(g_hd_power_manage_info.hd_power_info[hd_num].reserved, 0, 8*sizeof(unsigned int));

			hd_num++;
		}	
		
	}	
}

/******************************************************************************
* 函数名称：start_hd_power_manage
* 功能描述：启动硬盘电源管理(线程)
* 输入参数：无
* 输出参数：无
* 返 回 值：成功: 0; 失败: -1
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int start_hd_power_manage()
{
	pthread_t hd_power_manage_thread;
	pthread_attr_t hd_power_manage_thread_attr;
	
	printf("Start_hd_power_manage....\n");
	printf("Init all hard disk power control.....\n");
	
	init_all_hd_power_status();
	
	if (pthread_attr_init(&hd_power_manage_thread_attr) == 0)
	{		
		pthread_attr_setschedpolicy(&hd_power_manage_thread_attr, HD_POWER_MANAGE_PRIORITY);
	}
	
  	if (pthread_create(&hd_power_manage_thread, &hd_power_manage_thread_attr, hd_power_manage_proc, NULL) == 0)
	{
		printf("Hard disk power manage server: start.\n");
		pthread_attr_destroy(&hd_power_manage_thread_attr);
		return 0;
	}
	else
	{
		printf("Hard disk power manage server: stop.\n");
		pthread_attr_destroy(&hd_power_manage_thread_attr);
		return -1;
	}
	
}	

void hd_pre_ative(int hd_no)
{
	if (hd_no >= 0 && hd_no <= 7)
	{
		g_hd_power_manage_info.prev_active_disk_no = hd_no;
	}
}

int get_pre_ative_hd_no()
{
	return g_hd_power_manage_info.prev_active_disk_no;
}

/******************************************************************************
* 函数名称：hd_power_manage_proc
* 功能描述：硬盘电源管理函数
* 输入参数：无
* 输出参数：无
* 返 回 值：无
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
void *hd_power_manage_proc()
{
	int i;
	int hd_pre_active_flag = 0;
	int old_hd_no = -1;
	int hd_status;
	int cur_hd_no = -1;
	

	while (1)
	{
		//获取系统里所有硬盘电源状态信息
		for (i=0; i<g_hd_power_manage_info.hd_number; i++)
		{
			hd_status = get_hd_power_status(g_hd_power_manage_info.hd_power_info[i].hd_name);
			if (hd_status>=ACTIVE && hd_status<=SLEEP)
			{
				g_hd_power_manage_info.hd_power_info[i].cur_power_status = hd_status;
				printf("Get hard disk(%s) power status: %d\n", g_hd_power_manage_info.hd_power_info[i].hd_name,
						 hd_status);
			}
		}

		//预激活下一块硬盘
		if (g_hd_power_manage_info.hd_number >= 1)
		{	
			if (g_hd_power_manage_info.prev_active_disk_no != -1 && hd_pre_active_flag == 0)
			{
				hd_pre_active_flag = 1;
				old_hd_no = g_hd_power_manage_info.cur_active_disk_no;
				hard_reset_hd(g_hd_power_manage_info.hd_power_info[g_hd_power_manage_info.prev_active_disk_no].hd_name);		
				printf("Active next hard disk: %s.\n",	
						g_hd_power_manage_info.hd_power_info[g_hd_power_manage_info.prev_active_disk_no].hd_name);
			}

			if (hd_pre_active_flag == 1)
			{
				//cur_hd_no = get_cur_disk_no();
				if (cur_hd_no == g_hd_power_manage_info.prev_active_disk_no)
				{
					//set_hd_sleep(g_hd_power_manage_info.hd_power_info[old_hd_no].hd_name);
					set_hd_standby(g_hd_power_manage_info.hd_power_info[old_hd_no].hd_name);
					g_hd_power_manage_info.cur_active_disk_no = cur_hd_no;
					g_hd_power_manage_info.prev_active_disk_no = -1;
					hd_pre_active_flag = 0;
				}
				
				//预防再次休眠
				if (g_hd_power_manage_info.hd_power_info[i].cur_power_status == STANDBY)
				{
					hard_reset_hd(g_hd_power_manage_info.hd_power_info[g_hd_power_manage_info.prev_active_disk_no].hd_name);
				}
			}			
		}
		
		sleep(60);
	}
}




