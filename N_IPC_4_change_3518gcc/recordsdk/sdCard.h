/* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：hardDisk.h
* 文件说明：该文件描述了操作硬盘的函数声明
*           包括：
*           1．FDISK_INFO数据结构的定义
*			2. PARTITION_INFO数据结构的定义
*			3. DISK_INFO数据结构的定义
*			4. HARD_DISK_INFO数据结构的定义
*           5．操作硬盘的函数声明
* 作    者：庄惠斌
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

#ifndef __SD_CARD_H_
#define __SD_CARD_H_

typedef struct
{
	unsigned int formated;			// 是否已经格式化0:no 1:yes
	unsigned int mount_flag;		// MOUNT标志
	unsigned int rdwr_flag;			// 读写标志，0：可读可写，1：只读
	unsigned int total_size;		// 总有容量，单位为M
	unsigned int used_size;			// 已用容量，单位为M
	unsigned int availabe_size;		// 可用容量，单位为M
	unsigned int used_parent;

}SD_CARD_INFO, *PSD_CARD_INFO;

int sd_get_mount_flag();
int sd_get_total_size();
int sd_get_used_size();
int sd_get_available_size();

int sd_mount();
int sd_umount();

int sd_fdisk();
int sd_format();
int sd_get_info(SD_CARD_INFO *sd_card_info);
int sd_get_formated();

int sd_get_fdisk_format_process();
int sd_get_fdisk_format_status();

int sd_query_info();

#endif


