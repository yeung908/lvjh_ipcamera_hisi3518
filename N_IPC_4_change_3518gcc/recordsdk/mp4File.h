/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：mp4File.h
* 文件说明：该文件描述了录像文件操作
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-01-29
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/
 
#ifndef __MP4FILE_H_
#define __MP4FILE_H_

#include <pthread.h>
#include "fileFormat.h"

#define PACK_SIZE   				1024

#define TMRECORD_ERROR_BASE_ADDR	-501
#define TMRECORD_SUCCESS			0
#define TMRECORD_ERROR				-1

typedef void (* NetStreamCallBackFun)(void * context, DWORD ip, WORD channel, LPVOID pBuffer, DWORD size, BYTE flag);
typedef void (* NetStreamCallBackFun2)(void * context, DWORD ip, WORD channel, WORD port, LPVOID pBuffer, DWORD size, BYTE flag);

typedef struct _MP4_RECORD_INSTANCE
{
	DWORD first_time_code;
	DWORD first_time_tick;
	BOOL first_i_frame;
	DWORD current_time_code;
	DWORD write_pos;
	DWORD frame_size;
	DWORD file_size;
	DWORD play_time;

	DWORD video_width;
	DWORD video_height;

	WORD audio_channel;
	DWORD audio_sample_rate;
	DWORD audio_bit_rate;

	BYTE *in_buffer;
	DWORD ip;
	WORD channel;
	WORD port;

	TDS_FILEHEADER file_header;
	FRAME_HEADER frame_header;

	NetStreamCallBackFun pcallback_fun;
	NetStreamCallBackFun2 pcallback_fun2;

	BOOL seekable;
	DWORD index_cur_pos;
	DWORD index_max_pos;

	void *context;
	BYTE *p_buffer;
	BYTE *index_buf;

	pthread_mutex_t mutex;

}MP4_RECORD_INSTANCE;


typedef struct _FILE_INFO
{
	TDS_FILEHEADER file_header;
	FRAME_HEADER cur_frame_header;
	
	DWORD cur_frame_offset;
	DWORD old_frame_offset;
	
	DWORD cur_time_tick;
	DWORD old_time_tick;
	
	DWORD back_play_time_tick;
	
	DWORD file_type;
	DWORD cur_key_frame_no;
	DWORD key_frame_num;
	DWORD index_pos;
	DWORD cur_file_pos;
	FILE *file;
	
	pthread_mutex_t mutex;
	
}FILE_INFO;

 // 录MP4文件
MP4_RECORD_INSTANCE *mp4_record_create_instance();
int mp4_record_release_instance(MP4_RECORD_INSTANCE *instance);

int mp4_record_set_record_type(MP4_RECORD_INSTANCE *instance, DWORD record_type);
int mp4_record_set_video_format(MP4_RECORD_INSTANCE *instance, DWORD encType, DWORD width, DWORD height, DWORD bitrate, DWORD framerate);
int mp4_record_set_audio_format(MP4_RECORD_INSTANCE *instance, DWORD encType, DWORD channel, DWORD sample_rate, DWORD bit_rate);

int mp4_record_start_record(MP4_RECORD_INSTANCE *instance, DWORD record_type);
int mp4_record_stop_record(MP4_RECORD_INSTANCE *instance);

int mp4_record_send_one_pack(MP4_RECORD_INSTANCE *instance, void * buf);
int mp4_record_send_one_frame(MP4_RECORD_INSTANCE *instance, void *buf, DWORD size);

int mp4_record_set_callback_fun(MP4_RECORD_INSTANCE *instance, NetStreamCallBackFun fun, 
										 void * context, DWORD ip, WORD channel);
int mp4_record_set_callback_fun2(MP4_RECORD_INSTANCE *instance, NetStreamCallBackFun2 fun, 
										  void *context, DWORD ip, WORD channel, WORD port);

// 分析MP4文件
FILE_INFO *mp4_record_open_file(char *file_name);
int mp4_record_close_file(FILE_INFO *file_info);

int mp4_record_get_file_cur_pos(FILE_INFO *file_info);
int mp4_record_get_file_end_pos(FILE_INFO *file_info);

int mp4_record_get_file_len(FILE_INFO *file_info);

int mp4_record_get_first_timecode(FILE_INFO *file_info);
int mp4_record_get_last_timecode(FILE_INFO *file_info);

int mp4_record_get_play_time(FILE_INFO *file_info);
TDS_FILETIME mp4_record_get_create_time(FILE_INFO *file_info);
int mp4_record_get_key_frame_count(FILE_INFO *file_info);
int mp4_record_get_cur_key_frame_no(FILE_INFO *file_info);

int mp4_record_get_audio_channel(FILE_INFO *file_info);
int mp4_record_get_audio_sample(FILE_INFO *file_info);
int mp4_record_get_audio_avg_byte(FILE_INFO *file_info);	

int mp4_record_get_video_width(FILE_INFO *file_info);
int mp4_record_get_video_height(FILE_INFO *file_info);

int mp4_record_get_cur_frame_timecode(FILE_INFO *file_info);

int mp4_record_get_n_key_frame(FILE_INFO *file_info, void *frame, DWORD n, DWORD *len);
int mp4_record_get_first_key_frame(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_last_key_frame(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_prev_key_frame(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_next_key_frame(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_key_frame_by_time(FILE_INFO *file_info, void *frame, DWORD *len, DWORD misec);

int mp4_record_get_next_frame(FILE_INFO *file_info, void *frame, DWORD *len);

// 回放MP4文件
int mp4_record_get_prev_key_frame_Ex(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_next_frame_Ex(FILE_INFO *file_info, void *frame, DWORD *len);
int mp4_record_get_record_type(char *file_name);
int mp4_record_update_data(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header);
int mp4_record_update_file_header(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header);
int mp4_record_update_index_header(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header);

#endif
