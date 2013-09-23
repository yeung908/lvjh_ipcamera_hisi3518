/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：mp4File.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>

#include "global.h"
#include "fileFormat.h"
#include "mp4File.h"

/******************************************************************************
* 函数名称：mp4_record_create_instance
* 功能描述：创建录像文件的句柄
* 输入参数：无
* 输出参数：录像文件的句柄
* 返 回 值：失败：NULL；成功：录像文件的句柄指针
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
MP4_RECORD_INSTANCE *mp4_record_create_instance()
{
	BYTE *buffer = NULL;
	pthread_mutexattr_t attr;
	MP4_RECORD_INSTANCE *instance = NULL;
	TDS_INDEXOBJECT * indexObject = NULL;
	INT32 len = 0;
	
	len = sizeof(MP4_RECORD_INSTANCE);

	instance = (MP4_RECORD_INSTANCE *)malloc(len);
	if (instance == NULL)
	{
		return NULL;
	}

	memset(instance, 0, len);

	instance->file_header.File_Flag = TDSFLAG;
	instance->file_header.File_Type = 0x00;

	buffer = (BYTE *)malloc(MAX_FRAME_SIZE);
	if (buffer == NULL)
	{
		free(instance);
		return NULL;
	}
	instance->p_buffer = buffer;

	buffer = (BYTE *)malloc(MAX_FRAME_SIZE);
	if (buffer == NULL)
	{
		free(instance->p_buffer);
		free(instance);
		return NULL;
	}
	instance->in_buffer = buffer;

	//instance->index_max_pos = sizeof(TDS_INDEXOBJECT) + sizeof(TDS_INDEXENTRIES) * 1024;
	instance->index_max_pos = sizeof(TDS_INDEXOBJECT) + sizeof(TDS_INDEXENTRIES) * 1800;	// 如果I帧间隔是1S的话，那么一分钟就有1800个索引信息
	buffer = (BYTE *)malloc(instance->index_max_pos);
	if (buffer == NULL)
	{
		free(instance->p_buffer);
		free(instance->in_buffer);
		free(instance);
		return NULL;
	}
	instance->index_buf = buffer;
	
	// Add the code by lvjh, 2009-02-25
	indexObject = (TDS_INDEXOBJECT * )instance->index_buf;
	indexObject->Index_Flag = INDEXFLAG;
	indexObject->Index_Count = 0;

	// 创建锁
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&instance->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	
	return instance;
}

/******************************************************************************
* 函数名称：mp4_record_release_instance
* 功能描述：释放录像文件的句柄
* 输入参数：录像文件的句柄
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_release_instance(MP4_RECORD_INSTANCE *instance)
{
	if (instance == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&instance->mutex);
	
	if (instance->in_buffer != NULL)
	{
		free(instance->in_buffer);
		instance->in_buffer = NULL;
	}
	if (instance->p_buffer != NULL)
	{
		free(instance->p_buffer);
		instance->p_buffer = NULL;
	}
	if (instance->index_buf != NULL)
	{
		free(instance->index_buf);
		instance->index_buf = NULL;
	}	
	
	pthread_mutex_unlock(&instance->mutex);
	pthread_mutex_destroy(&instance->mutex);
	
	free(instance);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_set_record_type
* 功能描述：设置录像文件的类型
* 输入参数：录像文件的句柄
*			录像文件的类型
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_set_record_type(MP4_RECORD_INSTANCE *instance, DWORD record_type)
{
	if (instance == NULL)
	{
		return -1;
	}
		
	pthread_mutex_lock(&instance->mutex);
	instance->file_header.File_Type |= (record_type<<16);
	pthread_mutex_unlock(&instance->mutex);
	
	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_set_video_size
* 功能描述：设置录像文件的视频数据宽高
* 输入参数：录像文件的句柄
*			视频的宽度
*			视频的高度
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_set_video_format(MP4_RECORD_INSTANCE *instance, DWORD encType, DWORD width, DWORD height, DWORD bitrate, DWORD framerate)
{
	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	instance->video_width = width;
	instance->video_height = height;
	instance->file_header.AV_Format.nVideoEncType = encType;
	instance->file_header.AV_Format.nImageWidth= width;
	instance->file_header.AV_Format.nImageHeight= height;
	instance->file_header.AV_Format.nVideoBitRate= bitrate;
	instance->file_header.AV_Format.nFrameRate= framerate;
	instance->file_header.File_Type = instance->file_header.File_Type | HDVSF_HASVIDEO;
	
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_set_audio_format
* 功能描述：设置录像文件的音频数据参数
* 输入参数：录像文件的句柄
*			音频的通道数
*			音频的采样率
*			音频的码流
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_set_audio_format(MP4_RECORD_INSTANCE *instance, DWORD encType, DWORD channel, DWORD sample_rate, DWORD bit_rate)
{
	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	instance->audio_channel = channel;
	instance->audio_sample_rate = sample_rate;
	instance->audio_bit_rate = bit_rate;
	instance->file_header.AV_Format.nAudioEncType = encType;
	instance->file_header.AV_Format.nAudioChannels= channel;
	instance->file_header.AV_Format.nAudioSamples= sample_rate;
	instance->file_header.AV_Format.nAudioBitRate= bit_rate;
	instance->file_header.File_Type = instance->file_header.File_Type | HDVSF_HASAUDIO;
	
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_start_record
* 功能描述：开始录像
* 输入参数：录像文件的句柄
*			录像文件的类型
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_start_record(MP4_RECORD_INSTANCE *instance, DWORD record_type)
{
	time_t timep;
	struct tm *p;
	TDS_INDEXOBJECT *indexObject = NULL;
	
	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	if (instance->pcallback_fun==NULL && instance->pcallback_fun2==NULL)
	{
		pthread_mutex_unlock(&instance->mutex);
		return -1;
	}

	if (!(record_type & HDVSF_HASVIDEO))		// 只有音频
	{
		instance->seekable = 0;
	}
	else
	{
		instance->seekable = 1;
	}

	// 初始化录像文件的句柄
	instance->file_size = sizeof(instance->file_header);
	instance->play_time = 0;
	instance->first_time_tick = 0;
	instance->frame_size = 0;
	instance->write_pos = 0;
	instance->file_header.File_Size = 0;
	instance->file_header.Index_Position = 0;
	instance->file_header.Play_Duration = 0;
	instance->index_cur_pos = sizeof(TDS_INDEXOBJECT);
	instance->first_i_frame = 0;

	// Add the code by lvjh, 2009-02-25
	indexObject = (TDS_INDEXOBJECT * )instance->index_buf;
	indexObject->Index_Flag = INDEXFLAG;
	indexObject->Index_Count = 0;
	
	time(&timep);
	p = localtime(&timep);
	instance->file_header.Creation_Date.nYear = p->tm_year + 1900;
	instance->file_header.Creation_Date.nMonth = p->tm_mon + 1 ;
	instance->file_header.Creation_Date.nDay = p->tm_mday;
	instance->file_header.Creation_Date.nHour = p->tm_hour;
	instance->file_header.Creation_Date.nMinute = p->tm_min;
	instance->file_header.Creation_Date.nSecond = p->tm_sec;
	
	// 调用回调函数写MP4文件
	if (instance->pcallback_fun != NULL)
	{
		instance->pcallback_fun(instance->context, instance->ip, instance->channel,
		                        &instance->file_header, sizeof(TDS_FILEHEADER), 1);
	}
	/*
	if (instance->pcallback_fun2 != NULL)
	{
		instance->pcallback_fun2(instance->context,instance->ip, instance->channel, instance->port,
					&instance->file_header, sizeof(TDS_FILEHEADER), 1);
	}
	*/
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_stop_record
* 功能描述：停止录像
* 输入参数：录像文件的句柄
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_stop_record(MP4_RECORD_INSTANCE *instance)
{
	TDS_INDEXOBJECT *object = NULL;

	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	// 写索引数据到MP4文件中
	if (instance->index_buf!=NULL && instance->index_cur_pos>sizeof(TDS_INDEXOBJECT))
	{
		if (instance->pcallback_fun != NULL)
		{
			instance->pcallback_fun(instance->context, instance->ip, instance->channel,
									instance->index_buf, instance->index_cur_pos, 0);
		}
		/*
		if (instance->pcallback_fun2 != NULL)
		{
			instance->pcallback_fun2(instance->context, instance->ip, instance->channel,
									 instance->port, instance->index_buf, instance->index_cur_pos, 0);
		}
		*/
	}

	// 索引信息恢复初始值
	if (instance->index_buf != NULL)
	{
		object = (TDS_INDEXOBJECT *)(instance->index_buf);
		object->Index_Flag = INDEXFLAG;
		object->Index_Count = 0;
		instance->index_cur_pos = sizeof(TDS_INDEXOBJECT);
	}

	instance->file_header.File_Size = instance->file_size + instance->index_cur_pos;
	instance->file_header.Index_Position = instance->file_size;
	instance->file_header.Play_Duration  = instance->current_time_code - instance->first_time_code;
	instance->file_header.BeginTimeTick = instance->first_time_code;
	instance->file_header.EndTimeTick = instance->current_time_code;
	instance->file_header.File_Type = instance->file_header.File_Type | HDVSF_HASINDEX;

	// 重写TM4文件头
	if(instance->pcallback_fun != NULL)
	{		
		instance->pcallback_fun(instance->context, instance->ip, instance->channel,
								&instance->file_header, sizeof(TDS_FILEHEADER), 2);
	}
	/*
	if (instance->pcallback_fun2 != NULL)
	{
		instance->pcallback_fun2(instance->context, instance->ip, instance->channel, instance->port,
								 &instance->file_header, sizeof(TDS_FILEHEADER), 2);
	}
	*/						 
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_send_one_pack
* 功能描述：送一个数据包
* 输入参数：录像文件的句柄
*			数据包指针
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_send_one_pack(MP4_RECORD_INSTANCE *instance, void *buf)
{
	PDATA_PACKET pack = NULL;
	
	if (instance == NULL)
	{
		return -1;
	}
	if (buf == NULL)
	{
		return -1;
	}

	pack = (DATA_PACKET * )buf;
	
	pthread_mutex_lock(&instance->mutex);
	
	if (pack->bIsDataHead)		// 第一个
	{
		instance->write_pos = 0;
		instance->frame_size = pack->FrameHeader .nAudioSize + pack->FrameHeader.nVideoSize 
							    + sizeof(pack->FrameHeader);
		
		memcpy(instance->in_buffer+instance->write_pos, &pack->FrameHeader, sizeof(FRAME_HEADER));
		instance->write_pos += sizeof(FRAME_HEADER);
		memcpy(instance->in_buffer+instance->write_pos, pack->PackData, pack->nBufSize);
		instance->write_pos += pack->nBufSize;
	}
	else
	{
		if (instance->write_pos+pack->nBufSize <= instance->frame_size)	
		{
			memcpy(instance->in_buffer + instance->write_pos, pack->PackData, pack->nBufSize);
			instance->write_pos += pack->nBufSize;
		}
		else					// 包超大
		{
			instance->write_pos = 0;
			instance->frame_size = 0;
			pthread_mutex_unlock(&instance->mutex);
			return -1;
		}
	}
	
	if (instance->write_pos == instance->frame_size)
	{
		mp4_record_send_one_frame(instance, instance->in_buffer, instance->frame_size);
		instance->write_pos = 0;
		instance->frame_size = 0;
	}

	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_send_one_frame
* 功能描述：送一个数据帧
* 输入参数：录像文件的句柄
*			数据包指针
*			数据包大小
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_send_one_frame(MP4_RECORD_INSTANCE *instance, void *buf, DWORD size)
{
	BYTE *pAV = NULL;

	if (instance == NULL)
	{
		return -1;
	}
	if (buf == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	memcpy(&(instance->frame_header), buf, sizeof(instance->frame_header));
		
	if (!instance->first_i_frame)
	{
		if (!instance->frame_header.nKeyFrame)
		{
			pthread_mutex_unlock(&instance->mutex);
			return -1;
		}
		instance->first_i_frame = 1;
		instance->first_time_code = instance->frame_header.nTimeTick;
	}

	instance->current_time_code = instance->frame_header.nTimeTick;

	pAV = (BYTE *)buf;
	pAV = pAV + sizeof(instance->frame_header);
	
	//printf("File_Type: %x\n", instance->file_header.File_Type);
	switch ((instance->file_header.File_Type & 0x06)>>0x01)
	{
	case HDVSF_HASAUDIO:		// 只有音频
		instance->frame_header.nVideoSize = 0;

		memcpy(instance->p_buffer, &(instance->frame_header), sizeof(instance->frame_header));
			
		if (instance->frame_header.nAudioSize > 0)
		{
			memcpy(&(instance->p_buffer[sizeof(instance->frame_header)]),
					pAV + instance->frame_header.nVideoSize, instance->frame_header.nAudioSize);
			
			if (instance->pcallback_fun != NULL)
			{
				instance->pcallback_fun(instance->context, instance->ip, instance->channel, instance->p_buffer,
										sizeof(instance->frame_header) + instance->frame_header.nAudioSize, 0);
			}
			/*
			if (instance->pcallback_fun2 != NULL)
			{
				instance->pcallback_fun2(instance->context, instance->ip, instance->channel, instance->port,
										 instance->p_buffer, sizeof(instance->frame_header) + instance->frame_header.nAudioSize, 0);
			}
			*/
			if (instance->seekable)
			{
				mp4_record_update_data(instance,&(instance->frame_header));
			}
		}
		break;
		
	case HDVSF_HASVIDEO:		// 只有视频
		instance->frame_header.nAudioSize = 0;

		memcpy(instance->p_buffer, &(instance->frame_header), sizeof(instance->frame_header));
			
		if(instance->frame_header.nVideoSize > 0)
		{
			memcpy(&(instance->p_buffer[sizeof(instance->frame_header)]), pAV, instance->frame_header.nVideoSize);
			
			if (instance->pcallback_fun != NULL)
			{
				instance->pcallback_fun(instance->context,instance->ip, instance->channel, instance->p_buffer,
										sizeof(instance->frame_header) + instance->frame_header.nVideoSize, 0);
			}
			/*
			if (instance->pcallback_fun2 != NULL)
			{
				instance->pcallback_fun2(instance->context, instance->ip, instance->channel, instance->port,
										 instance->p_buffer, sizeof(instance->frame_header) + instance->frame_header.nVideoSize, 0);
			}
			*/
			if (instance->seekable)
			{
				mp4_record_update_data(instance, &(instance->frame_header));
			}
		}
		break;
		
	case 0x03:		// 音视频
		if (instance->pcallback_fun != NULL)
		{
			instance->pcallback_fun(instance->context, instance->ip, instance->channel, buf, size, 0);
		}
		/*
		if (instance->pcallback_fun2 != NULL)
		{
			instance->pcallback_fun2(instance->context, instance->ip, instance->channel, instance->port, buf, size, 0);
		}
		*/
		if (instance->seekable)
		{
			mp4_record_update_data(instance, &(instance->frame_header));
		}
		break;
	}
	
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_set_callback_fun
* 功能描述：设置录像回调函数
* 输入参数：录像文件的句柄
*			回调函数
*			回调函数参数
*			IP
*			通道号
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_set_callback_fun(MP4_RECORD_INSTANCE *instance, NetStreamCallBackFun fun, 
								  void * context, DWORD ip, WORD channel)
{
	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	instance->pcallback_fun = fun;
	instance->context = context;
	instance->ip = ip;
	instance->channel = channel;
	
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_set_callback_fun2
* 功能描述：设置录像回调函数
* 输入参数：录像文件的句柄
*			回调函数
*			回调函数参数
*			IP
*			通道号
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_set_callback_fun2(MP4_RECORD_INSTANCE *instance,NetStreamCallBackFun2 fun, 
								   void * context ,DWORD ip, WORD channel,WORD port)
{
	if (instance == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&instance->mutex);
	
	instance->pcallback_fun2 = fun;
	instance->context = context;
	instance->ip = ip;
	instance->port = port;
	instance->channel = channel;
	
	pthread_mutex_unlock(&instance->mutex);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_update_data
* 功能描述：更新录像文件的头和索引
* 输入参数：录像文件的句柄
*			帧头
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_update_data(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header)
{
	if (instance == NULL)
	{
		return -1;
	}

	mp4_record_update_index_header(instance, frame_header);
	mp4_record_update_file_header(instance, frame_header);

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_update_file_header
* 功能描述：更新录像文件的头
* 输入参数：录像文件的句柄
*			帧头
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_update_file_header(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header)
{
	if (instance == NULL)
	{
		return -1;
	}

	instance->file_size = instance->file_size + sizeof(FRAME_HEADER) + frame_header->nVideoSize + frame_header->nAudioSize;
	
	if (instance->first_time_tick == 0)
	{
		instance->first_time_tick = frame_header->nTimeTick;
	}

	instance->play_time = frame_header->nTimeTick - instance->first_time_tick;
	
	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_update_index_header
* 功能描述：更新录像文件的索引
* 输入参数：录像文件的句柄
*			帧头
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_update_index_header(MP4_RECORD_INSTANCE *instance, PFRAME_HEADER frame_header)
{
	TDS_INDEXOBJECT *object;
	TDS_INDEXENTRIES *index;

	if (instance == NULL)
	{
		return -1;
	}

	if (frame_header->nKeyFrame)
	{
		if (instance->index_buf == NULL)	// 第一次为索引分配内存
		{
			// 每次分配1K
			//instance->index_max_pos = sizeof(TDS_INDEXOBJECT) + sizeof(TDS_INDEXENTRIES) * 1024 ;
			instance->index_max_pos = sizeof(TDS_INDEXOBJECT) + sizeof(TDS_INDEXENTRIES) * 1800 ;
			instance->index_buf = (BYTE *)malloc(instance->index_max_pos);

			if (instance->index_buf != NULL)
			{
				object  = (TDS_INDEXOBJECT *)instance->index_buf;
				object->Index_Count = 0;
				object->Index_Flag = INDEXFLAG;
				instance->index_cur_pos = sizeof(TDS_INDEXOBJECT);
			}
			else
			{
				instance->index_cur_pos = 0;
				instance->index_max_pos = 0;
				return -1;
			}
		}

		if (instance->index_cur_pos >= instance->index_max_pos) // 索引内存不够用，重新分配内存
		{
			instance->index_buf = (BYTE *)realloc (instance->index_buf, instance->index_max_pos	+ sizeof(TDS_INDEXENTRIES) * 1024);

			if (instance->index_buf == NULL)
			{
				instance->index_cur_pos = 0;
				instance->index_max_pos = 0;
				return -1;
			}

			instance->index_max_pos = instance->index_max_pos + sizeof(TDS_INDEXENTRIES) * 1024;
		}

		object = (TDS_INDEXOBJECT *)instance->index_buf;
		object->Index_Count++ ;
		object->Index_Flag = INDEXFLAG;		// Add the code by lvjh, 2009-02-25

		index = (TDS_INDEXENTRIES *)&(instance->index_buf[instance->index_cur_pos]);
		index->Index_TimeTick = frame_header->nTimeTick;
		index->Index_Offset = instance->file_size;
		
		instance->index_cur_pos += sizeof(TDS_INDEXENTRIES);
	}

	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_open_file
* 功能描述：打开一个MP4文件
* 输入参数：文件名
* 输出参数：NULL
* 返 回 值：失败：NULL；成功：FILE_INFO
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
FILE_INFO *mp4_record_open_file(char *file_name)
{
	pthread_mutexattr_t attr;
	DWORD num = 0;
	FILE_INFO *file_info = NULL;
	TDS_INDEXOBJECT index_object;
	DWORD file_type = 0;
	DWORD index_pos = 0;

	if (file_name == NULL)
	{
		return NULL;
	}

	file_info = (FILE_INFO *)malloc(sizeof(FILE_INFO));
	if (file_info == NULL)
	{
		return NULL;
	}
		
	memset(file_info, 0, sizeof(FILE_INFO));

	// 只读打开文件
	file_info->file = fopen(file_name, "rb");
	if (file_info->file == NULL)
	{
		free(file_info);
		return NULL;
	}

	// 读取文件信息
	num = fread(&(file_info->file_header), 1, sizeof(file_info->file_header), file_info->file);
	if (num < sizeof(file_info->file_header))
	{
		free(file_info);
		return NULL;
	}
	
	// 判断文件类型
	file_type = file_info->file_header.File_Type;
	if (file_type > 0x00)
	{
		index_pos = file_info->file_header.Index_Position;

		fseek(file_info->file, index_pos, SEEK_SET);
		num = fread(&index_object, sizeof(TDS_INDEXOBJECT), 1, file_info->file);
	
		file_info->file_type = file_type;
		file_info->index_pos = index_pos;
		file_info->key_frame_num = index_object.Index_Count;
		file_info->cur_key_frame_no = 0;
		
		fseek(file_info->file, sizeof(file_info->file_header), SEEK_SET);
	}
	else
	{
		file_info->file_type = file_type;
		file_info->index_pos = 0;
		file_info->key_frame_num = 0;
		file_info->cur_key_frame_no = 0;
	}
	
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&file_info->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	
	return file_info;
}

/******************************************************************************
* 函数名称：mp4_record_close_file
* 功能描述：关闭MP4文件
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_close_file(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	fclose(file_info->file);

	pthread_mutex_destroy(&file_info->mutex);

	free(file_info);

	return 0;
}

int mp4_record_get_file_cur_pos(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	return file_info->cur_file_pos;
}

int mp4_record_get_file_end_pos(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	return file_info->index_pos;
}

int mp4_record_get_file_len(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	return file_info->file_header.File_Size;
}

int mp4_record_get_first_timecode(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	return file_info->file_header.BeginTimeTick;
}

int mp4_record_get_last_timecode(FILE_INFO *file_info)
{
	if (file_info == NULL)
	{
		return -1;
	}

	return file_info->file_header.EndTimeTick;
}

/******************************************************************************
* 函数名称：mp4_record_get_play_time
* 功能描述：获取MP4文件的播放时间
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：失败：-1；成功：播放时间
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_play_time(FILE_INFO *file_info)
{
	DWORD play_time = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	play_time = file_info->file_header.Play_Duration;
	pthread_mutex_unlock(&file_info->mutex);
	
	return play_time;
}

/******************************************************************************
* 函数名称：mp4_record_get_create_time
* 功能描述：获取MP4文件创建时间
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：TMDATE
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
TDS_FILETIME mp4_record_get_create_time(FILE_INFO *file_info)
{
	TDS_FILETIME date;

	memset(&date, 0, sizeof(TDS_FILETIME));

	if (file_info == NULL)
	{
		return date;
	}

	pthread_mutex_lock(&file_info->mutex);
	date = file_info->file_header.Creation_Date;	
	pthread_mutex_unlock(&file_info->mutex);
	
	return date;
}

/******************************************************************************
* 函数名称：mp4_record_get_key_frame_count
* 功能描述：获取MP4文件中关键帧的个数
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：关键帧的个数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_key_frame_count(FILE_INFO *file_info)
{
	DWORD key_frame_count = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	key_frame_count = file_info->key_frame_num;
	pthread_mutex_unlock(&file_info->mutex);
	
	return key_frame_count;
}

/******************************************************************************
* 函数名称：mp4_record_get_cur_key_frame_no
* 功能描述：获取MP4文件中当前的关键帧
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：当前的关键帧
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_cur_key_frame_no(FILE_INFO *file_info)
{
	DWORD cur_key_frame = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	cur_key_frame = file_info->key_frame_num;
	pthread_mutex_unlock(&file_info->mutex);
	
	return cur_key_frame;
}

/******************************************************************************
* 函数名称：mp4_record_get_audio_channel
* 功能描述：获取MP4文件中的音频通道数
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：失败：-1；成功：音频通道数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_audio_channel(FILE_INFO *file_info)
{
	int channel = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	channel = file_info->file_header.AV_Format.nAudioChannels;
	pthread_mutex_unlock(&file_info->mutex);
	
	return channel;
}

/******************************************************************************
* 函数名称：mp4_record_get_audio_sample
* 功能描述：获取MP4文件中的音频采样率
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：失败：-1；成功：音频采样率
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_audio_sample(FILE_INFO *file_info)
{
	int sample = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	sample = file_info->file_header.AV_Format.nAudioSamples;
	if (sample == 44000)
	{
		sample = 44100;
	}
	pthread_mutex_unlock(&file_info->mutex);
	
	return sample;
}

/******************************************************************************
* 函数名称：mp4_record_get_audio_avg_byte
* 功能描述：获取MP4文件中的每AV数据量
* 输入参数：文件信息
* 输出参数：NULL
* 返 回 值：失败：-1；成功：音频通道数
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_audio_avg_byte(FILE_INFO *file_info)
{
	int avg_byte = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	avg_byte = file_info->file_header.AV_Format.nAudioBitRate;
	pthread_mutex_unlock(&file_info->mutex);
	
	return avg_byte;
}

int mp4_record_get_video_width(FILE_INFO *file_info)
{
	int width = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	width = file_info->file_header.AV_Format.nImageWidth;
	pthread_mutex_unlock(&file_info->mutex);
	
	return width;
}

int mp4_record_get_video_height(FILE_INFO *file_info)
{
	int height = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	height = file_info->file_header.AV_Format.nImageHeight;
	pthread_mutex_unlock(&file_info->mutex);
	
	return height;
}

int mp4_record_get_record_type(char *file_name)
{
	int num;
	TDS_FILEHEADER file_header;
	FILE *file = NULL;
	
	// 打开MP4文件
	file = fopen(file_name, "rb");
	if (file == NULL)
	{
		return -1;
	}

	// 读取文件头
	num = fread(&file_header, 1, sizeof(file_header), file);
	if (num < sizeof(file_header))
	{
		fclose(file);
		return -1;
	}
	fclose(file);
	
	return file_header.File_Type >> 16;
}

int mp4_record_get_cur_frame_timecode(FILE_INFO *file_info)
{
	int nTimeTick = 0;

	if (file_info == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	nTimeTick = file_info->cur_frame_header.nTimeTick;
	pthread_mutex_unlock(&file_info->mutex);
	
	return nTimeTick;
}

/******************************************************************************
* 函数名称：mp4_record_get_n_key_frame
* 功能描述：获取MP4文件中的指定关键帧的数据
* 输入参数：文件信息
*			指定的关键帧
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_n_key_frame(FILE_INFO *file_info, void *frame, DWORD n, DWORD *len)
{
	DWORD file_type = 0;
	DWORD index_pos = 0; 
	DWORD key_frame_num = 0; 
	FRAME_HEADER frame_header;
	TDS_INDEXENTRIES index_data;
	DWORD offset = 0;
	DWORD size = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	
	file_type = file_info->file_type;
	index_pos = file_info->index_pos;
	key_frame_num = file_info->key_frame_num;
	
	if (file_type < 0x02 || n < 0 || n > key_frame_num - 1)
	{
		pthread_mutex_unlock(&file_info->mutex);
		return -1;
	}

	fseek(file_info->file, index_pos+sizeof(TDS_INDEXOBJECT)+sizeof(TDS_INDEXENTRIES)*n, SEEK_SET);
	fread(&index_data, sizeof(TDS_INDEXENTRIES), 1, file_info->file);
	
	offset = index_data.Index_Offset;
	
	fseek(file_info->file, offset, SEEK_SET);
	ret = fread(&frame_header, 1, sizeof(FRAME_HEADER), file_info->file);
	if (ret <sizeof(FRAME_HEADER))
	{
		pthread_mutex_unlock(&file_info->mutex);
		return -1;
	}
	
	memcpy(&file_info->cur_frame_header, &frame_header, sizeof(frame_header));
	
	size = sizeof(FRAME_HEADER) + frame_header.nVideoSize + frame_header.nAudioSize;
	
	if(*len < size)
	{
		*len = size;
		pthread_mutex_unlock(&file_info->mutex);
		return -1;
	}
	*len = size;
		
	fseek(file_info->file, offset, SEEK_SET);
	ret = fread(frame, 1, size, file_info->file);
	if(ret < size)
	{
		pthread_mutex_unlock(&file_info->mutex);
		return -1;
	}
		
	file_info->cur_key_frame_no = n;
	file_info->old_frame_offset = file_info->cur_frame_offset;
	file_info->cur_frame_offset = offset;
	file_info->old_time_tick = file_info->cur_time_tick;
	file_info->cur_time_tick = frame_header.nTimeTick;
	file_info->cur_file_pos = ftell(file_info->file);
	
	pthread_mutex_unlock(&file_info->mutex);
	
	return 0;		
}

/******************************************************************************
* 函数名称：mp4_record_get_first_key_frame
* 功能描述：获取MP4文件中的第一关键帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_first_key_frame(FILE_INFO *file_info, void *frame, DWORD *len)
{
	int key_frame_num = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	key_frame_num = file_info->key_frame_num;
	pthread_mutex_unlock(&file_info->mutex);

	if(key_frame_num <= 0)
	{
		ret = -1;
	}
	else
	{
		ret = mp4_record_get_n_key_frame(file_info, frame, 0, len);
	}
	
	return ret;
}

/******************************************************************************
* 函数名称：mp4_record_get_first_key_frame
* 功能描述：获取MP4文件中的最后关键帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_last_key_frame(FILE_INFO *file_info, void *frame, DWORD *len)
{
	int key_frame_num = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	key_frame_num = file_info->key_frame_num;
	pthread_mutex_unlock(&file_info->mutex);

	if(key_frame_num <= 0)
	{
		ret = -1;
	}
	else
	{
		ret = mp4_record_get_n_key_frame(file_info, frame, key_frame_num-1, len);
	}
	
	return ret;
}

/******************************************************************************
* 函数名称：mp4_record_get_first_key_frame
* 功能描述：获取MP4文件中的前一关键帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_prev_key_frame(FILE_INFO *file_info, void *frame, DWORD *len)
{
	int cur_key_frame_no = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	cur_key_frame_no = file_info->cur_key_frame_no;
	pthread_mutex_unlock(&file_info->mutex);

	if(cur_key_frame_no <= 0)
	{
		ret = -1;
	}
	else
	{
		ret = mp4_record_get_n_key_frame(file_info, frame, cur_key_frame_no-1, len);
	}
	
	return ret;
}

/******************************************************************************
* 函数名称：mp4_record_get_first_key_frame
* 功能描述：获取MP4文件中的下一关键帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_next_key_frame(FILE_INFO *file_info,void *frame,DWORD *len)
{
	int cur_key_frame_no = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	cur_key_frame_no = file_info->cur_key_frame_no;
	pthread_mutex_unlock(&file_info->mutex);

	if(cur_key_frame_no <= 0)
	{
		ret = -1;
	}
	else
	{
		ret = mp4_record_get_n_key_frame(file_info, frame, cur_key_frame_no+1, len);
	}
	
	return ret;
}

/******************************************************************************
* 函数名称：mp4_record_get_first_key_frame
* 功能描述：根据时间获取MP4文件中的关键帧的数据
* 输入参数：文件信息
*			时间
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_key_frame_by_time(FILE_INFO *file_info, void *frame, DWORD *len, DWORD misec)
{
	DWORD file_type = 0;
	DWORD index_pos = 0; 
	DWORD key_frame_num = 0; 
	TDS_INDEXENTRIES index_data;
	DWORD first_time_tick = 0;
	DWORD i = 0;
	DWORD frame_no = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	
	file_type = file_info->file_type;
	index_pos = file_info->index_pos;
	key_frame_num = file_info->key_frame_num;
	if (file_type < 0x02)
	{
		pthread_mutex_unlock(&file_info->mutex);
		return -1;
	}
	
	for (i=0; i<key_frame_num; i++)
	{
		fseek(file_info->file, index_pos+sizeof(TDS_INDEXOBJECT)+sizeof(TDS_INDEXENTRIES)*i, SEEK_SET);
		fread(&index_data, sizeof(TDS_INDEXENTRIES), 1, file_info->file);
		
		if (i == 0)
		{
			frame_no = 0;
			first_time_tick = index_data.Index_TimeTick;
		}
		else
		{
			if (misec - (index_data.Index_TimeTick - first_time_tick) < 0)
			{
				frame_no = i-1;
				break;		
			}
			frame_no = i;
		}
	}

	pthread_mutex_unlock(&file_info->mutex);

	ret = mp4_record_get_n_key_frame(file_info, frame, frame_no, len);
	
	return ret;		
}

/******************************************************************************
* 函数名称：mp4_record_get_next_frame
* 功能描述：MP4文件中的下一帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_next_frame(FILE_INFO *file_info, void *frame, DWORD *len)
{
	DWORD size = 0;
	DWORD offset = 0;
	int ret = 0;
	FRAME_HEADER frame_header;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	
	offset = ftell(file_info->file);
	fread(&frame_header, sizeof(FRAME_HEADER), 1, file_info->file);
	
	memcpy(&file_info->cur_frame_header,&frame_header,sizeof(frame_header));
	
	size = sizeof(FRAME_HEADER) + frame_header.nVideoSize + frame_header.nAudioSize;
	if (*len < size)
	{
		fseek(file_info->file,offset,SEEK_SET);
		pthread_mutex_unlock(&file_info->mutex);
		*len = size;

		return -1;
	}
	*len = size;
		
	fseek(file_info->file, offset, SEEK_SET);
	ret = fread(frame,1,size,file_info->file);
	if (ret < size)
	{
		fseek(file_info->file, offset, SEEK_SET);
		pthread_mutex_unlock(&file_info->mutex);
		*len = ret;

		return -1;
	}	
		
	if (frame_header.nKeyFrame)
	{
		file_info->cur_key_frame_no++;
	}
		
	file_info->old_frame_offset = file_info->cur_frame_offset;
	file_info->cur_frame_offset = offset;
	file_info->old_time_tick = file_info->cur_time_tick;
	file_info->cur_time_tick = frame_header.nTimeTick;
	file_info->cur_file_pos = ftell(file_info->file);
	
	pthread_mutex_unlock(&file_info->mutex);
	
	return 0;
}

/******************************************************************************
* 函数名称：mp4_record_get_prev_key_frame_Ex
* 功能描述：MP4文件中的前一关键帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_prev_key_frame_Ex(FILE_INFO *file_info, void *frame, DWORD *len)
{
	DWORD cur_key_frame_no = 0;
	int ret = 0;

	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&file_info->mutex);
	
	file_info->back_play_time_tick = file_info->cur_time_tick;
	cur_key_frame_no = file_info->cur_key_frame_no;

	pthread_mutex_unlock(&file_info->mutex);

	ret = mp4_record_get_n_key_frame(file_info,frame,cur_key_frame_no,len);

	pthread_mutex_lock(&file_info->mutex);

	if(file_info->back_play_time_tick < file_info->cur_time_tick)
	{
		fseek(file_info->file,file_info->old_frame_offset,SEEK_SET);
		pthread_mutex_unlock(&file_info->mutex);

		return 0;
	}
	else if(file_info->back_play_time_tick == file_info->cur_time_tick)
	{
		if(cur_key_frame_no <= 0)
		{
			fseek(file_info->file,file_info->cur_frame_offset,SEEK_SET);
			pthread_mutex_unlock(&file_info->mutex);
			return 0;
		}
		else
		{
			cur_key_frame_no--;

			pthread_mutex_unlock(&file_info->mutex);
			
			ret = mp4_record_get_n_key_frame(file_info,frame,cur_key_frame_no,len);
			
			pthread_mutex_lock(&file_info->mutex);

			fseek(file_info->file,file_info->cur_frame_offset,SEEK_SET);
			
			pthread_mutex_unlock(&file_info->mutex);
			return 1;
		}
	}
	else 
	{
		fseek(file_info->file,file_info->cur_frame_offset,SEEK_SET);
		
		pthread_mutex_unlock(&file_info->mutex);
		
		return 1;
	}
}

/******************************************************************************
* 函数名称：mp4_record_get_next_frame_Ex
* 功能描述：MP4文件中的下一帧的数据
* 输入参数：文件信息
* 输出参数：帧数据
*			帧数据的大小
* 返 回 值：失败：-1；成功：0
* 修改记录: 无
* 其他说明: 无
********************************************************************************/
int mp4_record_get_next_frame_Ex(FILE_INFO *file_info,void *frame,DWORD *len)
{
	int ret = 0;
	
	if (file_info==NULL || frame==NULL || len==NULL)
	{
		return -1;
	}
	
	ret = mp4_record_get_next_frame(file_info,frame,len);

	pthread_mutex_lock(&file_info->mutex);

	if (file_info->cur_time_tick >= file_info->back_play_time_tick)
	{
		file_info->cur_time_tick = file_info->old_time_tick;
		file_info->cur_frame_offset = file_info->old_frame_offset;
		fseek(file_info->file, file_info->old_frame_offset, SEEK_SET);

		pthread_mutex_unlock(&file_info->mutex);

		return 0;
	}
	else
	{
		pthread_mutex_unlock(&file_info->mutex);

		return 1;
	}	
}


