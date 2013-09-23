/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：fileQuery.h
* 文件说明：该文件描述了录像文件查询的函数声明
*           包括：
*           1．录像文件节点的数据结构定义
*           2．录像文件链表的数据结构定义
*           3．录像文件查询的函数声明
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

#ifndef __FILE_QUERY_H__
#define __FILE_QUERY_H__

#ifndef MAX_PATH
#define MAX_PATH 	256
#endif

//#define PTHREAD_MUTEX_FAST_NP 0

typedef struct FILE_NODE
{
    char fullname[128];			//文件全名,包含路径
    char name[16];				//文件名,不包含路径
    int  start_hour;			//开始时间-小时
    int  end_hour;				//结束时间-小时
    int	 start_minute;			//开始时间-分钟
    int	 end_minute;			//结束时间-分钟
    int	 size;					//文件的大小
    struct  FILE_NODE	*prev;	//上一个数据项
    struct  FILE_NODE  	*next;  //下一个数据项
}FILE_NODE, *PFILE_NODE;

typedef struct FILES_LIST
{
    int item_count;		//文件的个数
    FILE_NODE *cur_file_node;	//链表的当前节点
    FILE_NODE *file_node;		//链表的头节点

    pthread_mutex_t	mutex;			//链表操作的互锁量
}FILE_LIST;

typedef struct
{
 	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nDay;
	unsigned long nType;
	unsigned long nNum;
	unsigned long nReserve;
	
}ACK_RECFILE;

/* export functions for file search when play record */
//FILE_LIST *open_file_search(int channel,int year, int month, int day, int start_hour,int start_minute,int start_second,
//                            int end_hour,int end_minute,int end_second);
//FILE_LIST * open_file_search(char *path,int start_hour,int start_minute,int start_second,
//			int end_hour,int end_minute,int end_second);

FILE_LIST *open_file_search(int channel, int play_backup_files, int year, int month, int day, int start_hour,int start_minute,int start_second,
                            int end_hour,int end_minute,int end_second);
FILE_LIST *open_file_search_by_type(int channel,int year,int month,int day,unsigned int record_type);
FILE_LIST *open_bak_file_search(int disk_type, int disk_no, int channel, int year, int month, int day, unsigned int record_type);
FILE_LIST * open_decoder_protocal_file_search();

int get_file_count(FILE_LIST *file_list);
int get_file_size_by_index(FILE_LIST *file_list,int *file_size,int index);
int get_file_name_by_index(FILE_LIST *file_list,char *file_name,int index);
int get_first_file_name(FILE_LIST *file_list,char *file_name);
int get_last_file_name(FILE_LIST *file_list,char *file_name);
int get_prev_file_name(FILE_LIST *file_list,char *file_name);
int get_next_file_name(FILE_LIST *file_list,char *file_name);
int get_file_index_by_name(FILE_LIST *file_list,char *file_name);
int close_file_search(FILE_LIST *file_list);
int find_record_file_by_type(int channel,int year,int month,int day,unsigned int record_type);
int get_file_attr_by_index(FILE_LIST *file_list, char *file_name, int *file_size, int *play_time, int index);

#endif /* __FILE_QUERY_H__ */

