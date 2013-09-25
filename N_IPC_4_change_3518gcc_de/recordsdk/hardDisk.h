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

#ifndef __HARD_DISK_H_
#define __HARD_DISK_H_

#define KILOBYTE			1024	//K  1024
#define MILOBYTE			1048576	//M  1024 * 1024

#define MAX_DISK_NUM		8
#define MAX_PARTITION_NUM	2

typedef struct __FDISK_INFO
{
	int type;						//0: fdisk  1:format
	int disk_no;
	int partition_no;
	int data_partition;
	int backup_partition;
}FDISK_INFO,*PFDISK_INFO;

// 系统所有硬盘及分区信息
typedef struct __PARTITION_INFO
{
	unsigned long mount_flag;		// MOUNT标志
	unsigned long format_flag;		// 是否已经格式化0:no 1:yes
	unsigned long total_size;		// 总有容量，单位为M
	unsigned long used_size;		// 已用容量，单位为M
	unsigned long availabe_size;	// 可用容量，单位为M
	unsigned long used_parent;		//
	unsigned long type;   			// 1: 数据分区 2: 备份分区 3: 其它
}PARTITION_INFO,*PPARTITION_INFO;

typedef struct __DISK_INFO
{
	unsigned long nPartitionNum;	// 分区数，add the code by lvjh, 2008-04-26
	PARTITION_INFO	partition_info[MAX_PARTITION_NUM];
}DISK_INFO,*PDISK_INFO;

typedef struct __HARD_DISK_INFO
{
	int cur_disk_no; 				// 整个系统最多8块硬盘 :0-7
	int cur_partition_no; 			// 一个硬盘最多2个分区: 0-1
	int hard_disk_num; 				// 系统总共有效硬盘数
	DISK_INFO disk_info[MAX_DISK_NUM];
}HARD_DISK_INFO,*PHARD_DISK_INFO;

int set_cur_disk_no(int disk_no);
int set_cur_partition_no(int partition_no);
int get_cur_disk_no();
int get_cur_partition_no();
int get_hard_disk_num();
int hd_get_mount_flag(int disk_no,int partition_no);
int hd_get_disk_total_size();
int hd_get_disk_used_size();
int hd_get_disk_available_size();
int hd_get_partition_total_size(int disk_no,int partition_no);
int hd_get_partition_used_size(int disk_no,int partition_no);
int hd_get_partition_available_size(int disk_no,int partition_no);

/* 路径格式 /record/hd##/##/... */
/* 系统总共最多8个硬盘,每个硬盘最多2个分区(数据区/备份区) */
int hd_mount_partition(char *source,char *target);
int hd_umount_partition(char *target);

/* mount 所有硬盘的所有分区 */
int hd_mount_all_partition();
/* umount 所有硬盘的所有分区 */
int hd_umount_all_partition();

int hd_fdisk(int disk_no,int data_partition,int backup_partition);
int hd_format(int disk_no,int partition_no);				// partition_no = 0, 1 cyg
int hd_cur_partition_full(int disk_no,int partition_no);
int hd_cur_partition_full(int disk_no,int partition_no);
int hd_find_empty_partition(int *disk_no,int *partition_no);
int hd_query_disk_info();
int hd_get_backup_partition_num(); 							// 获得备份分区数(已mount成功的备份区)
int hd_get_backup_disk_no(int backup_partition_no); 		// 通过备份分区号来获得备份硬盘号 
int hd_get_disk_formated(int disk_no, int partition_no);	// 获得硬盘是否已经格式化0:no   1:yes
int get_hard_disk_info(HARD_DISK_INFO *hard_disk_info); 	// 获得所有硬盘的信息

int get_fdisk_format_process();								// 获得分区格式化操作的进度
int get_fdisk_format_status();								// 获得分区格式化操作的结果

//获得已mount目录的剩余空间
//	ex. get_disk_avail_size("/"), get_disk_avail_size("/usb1"), get_disk_avail_size("/record/hd00/00")
//返回值: >= 0   空间大小,M为单位
//错误:   < 0    mount_point不存在等
int get_disk_avail_size(char *mount_point);

int hd_query_disk_info_ext();

#endif


