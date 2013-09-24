
#ifndef __INDEX_FILE_H_
#define __INDEX_FILE_H_

#define MAX_RECORD_FILE_NUM	60

typedef struct __RECORD_FILE_INDEX
{
	int file_type[MAX_RECORD_FILE_NUM];		// 数组的索引代表时间的分钟数 
}RECORD_FILE_INDEX;

int write_record_index_file(char *file_name,int minute,int file_type);
int search_record_index_file_by_type(char *file_name,int file_type);
int read_record_index_file(char *file_name,RECORD_FILE_INDEX *index);

#endif

