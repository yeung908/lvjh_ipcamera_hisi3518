/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：recordFile.h
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

#ifndef __RECORDFILE_H_
#define __RECORDFILE_H_

// 录像类型定义
#define RECORD_TYPE_TIME			0x00000001	// 定时录像
#define RECORD_TYPE_VIDEO_MOVE		0x00000002	// 视频移动
#define RECORD_TYPE_VIDEO_LOST		0x00000004	// 视频丢失
#define RECORD_TYPE_ALARM_INPUT		0x00000008	// 报警输入
#define RECORD_TYPE_NORMAL			0x00000010	// normal record

#define RECORD_TYPE_NUM				5			// 录像类型数
#define MAX_CAPTION_LEN				128

//int set_record_video_param(int channel, int mode, int width, int height);
int set_record_video_param(int channel, int mode, int width, int height, int bitrate, int framerate);
int set_record_audio_param(int channel, int mode, int channels, int rate, int bits);
int send_one_frame_to_recorder(int channel, unsigned char *buffer, int size);
int get_channel_record_type(int channel);
int get_record_file_name(int channel, char *file_name);
int record_channel_pause(int channel);
int record_channel_resume(int channel);
int record_channel_start(int channel);
int record_channel_stop(int channel);
int start_record(int channel, unsigned int type);
int stop_record(int channel, unsigned int type);
int switch_record_file(int channel);
int get_record_status(int channel);

#endif
