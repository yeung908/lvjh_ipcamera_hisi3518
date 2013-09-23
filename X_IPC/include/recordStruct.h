#ifndef __RECORD_STRUCT_H_
#define __RECORD_STRUCT_H_

#include "baseType.h"
#include "global.h"

#define RECORD_SDK_CMD      						20000

#define RSDKCMD_QUERY_RECDATE    					RECORD_SDK_CMD+1
#define RSDKCMD_QUERY_RECFILE    					RECORD_SDK_CMD+2
#define RSDKCMD_OPEN_RECFILE     					RECORD_SDK_CMD+3
#define RSDKCMD_CLOSE_RECFILE    					RECORD_SDK_CMD+4
#define RSDKCMD_READ_RECFILE     					RECORD_SDK_CMD+5
#define RSDKCMD_SEND_FRAME	  	   				RECORD_SDK_CMD+6
#define RSDKCMD_PAUSE_RECORD 	   				RECORD_SDK_CMD+7
#define RSDKCMD_RESUME_RECORD     				RECORD_SDK_CMD+8
#define RSDKCMD_DELETE_RECFILE					RECORD_SDK_CMD+9
#define RSDKCMD_BACKUP_RECFILE_LOCAL				RECORD_SDK_CMD+10
#define RSDKCMD_BACKUP_RECFILE_REMOTE			RECORD_SDK_CMD+11
#define RSDKCMD_QUERY_RECFILE_LIST				RECORD_SDK_CMD+12
#define RSDKCMD_GET_RECFILE_INFO					RECORD_SDK_CMD+13
#define RSDKCMD_SEND_JPEG							RECORD_SDK_CMD+14
#define RSDKCMD_QUERY_JPEGDATE						RECORD_SDK_CMD+15
#define RSDKCMD_QUERY_JPEGFILE						RECORD_SDK_CMD+16

#define RSDKCMD_GET_RECORD_PARAM				RECORD_SDK_CMD+101
#define RSDKCMD_GET_MANUAL_RECORD_PARAM		RECORD_SDK_CMD+102
#define RSDKCMD_GET_TIMER_RECORD_PARAM			RECORD_SDK_CMD+103
#define RSDKCMD_GET_DETECTOR_RECORD_PARAM		RECORD_SDK_CMD+104
#define RSDKCMD_GET_VIDEOMOVE_RECORD_PARAM	RECORD_SDK_CMD+105
#define RSDKCMD_GET_RECORD_STATUS				RECORD_SDK_CMD+106
#define RSDKCMD_GET_HARDDISK_INFO				RECORD_SDK_CMD+107
#define RSDKCMD_GET_FDISK_PROCESS				RECORD_SDK_CMD+108

#define RSDKCMD_SET_RECORD_PARAM				RECORD_SDK_CMD+201
#define RSDKCMD_SET_MANUAL_RECORD_PARAM		RECORD_SDK_CMD+202
#define RSDKCMD_SET_TIMER_RECORD_PARAM			RECORD_SDK_CMD+203
#define RSDKCMD_SET_DETECTOR_RECORD_PARAM		RECORD_SDK_CMD+204
#define RSDKCMD_SET_VIDEOMOVE_RECORD_PARAM	RECORD_SDK_CMD+205
#define RSDKCMD_SET_HARDDISK_FDISK				RECORD_SDK_CMD+206
#define RSDKCMD_SET_HARDDISK_FORMAT				RECORD_SDK_CMD+207
#define RSDKCMD_SET_VIDEO_STATUS					RECORD_SDK_CMD+208
#define RSDKCMD_SET_DETECTOR_STATUS				RECORD_SDK_CMD+209

// 时间段
typedef struct
{
	unsigned char start_hour;		//开始时间-小时
	unsigned char start_minute;		//开始时间-分钟
	unsigned char end_hour;			//结束时间-小时
	unsigned char end_minute;		//结束时间-分钟

}TIME_SEGMENT;

// 一天中的时间段
typedef struct
{
	unsigned long nOnFlag;			//启用的标志
	TIME_SEGMENT time_segment[MAX_TIME_SEGMENT];

}TIME_SEGMENTS;						//每天有两个时间段

// 手动录像
typedef struct
{
	unsigned long nOnFlag;			//启动-停止录像
	unsigned long nTime;			//录像时间,时间单位为分钟
	unsigned long nReserve;	
	
}MANUAL_RECORD_PARAM;

// 定时录像通道参数
typedef struct
{
	TIME_SEGMENTS day[8];

}TIMER_RECORD_CHANNEL_PARAM;

// 视频移动报警录像通道参数
typedef struct
{
	unsigned long nOnFlag;		//启动-停止录像
	unsigned long nTime;			//录像时间,时间单位为分钟
	unsigned long nChnBits;		//联动录像的通道位
	TIME_SEGMENT time_segment[MAX_TIME_SEGMENT];

}VIDEOMOTION_TIME_SEGMENTS;

typedef struct
{
	VIDEOMOTION_TIME_SEGMENTS day[8];

}VIDEOMOTION_RECORD_CHANNEL_PARAM;

// 探头报警录像通道参数
typedef struct
{
	unsigned long nOnFlag;		//启动-停止录像
	unsigned long nTime;			//录像时间,时间单位为分钟
	unsigned long nChnBits;		//联动录像的通道位
	TIME_SEGMENT time_segment[MAX_TIME_SEGMENT];

}DETECTOR_TIME_SEGMENTS;

typedef struct
{
	DETECTOR_TIME_SEGMENTS day[8];

}DETECTOR_RECORD_CHANNEL_PARAM;

/*
typedef struct
{
	unsigned long	nVideoEncType;
	unsigned long	nImageWidth;
	unsigned long	nImageHeight;
	unsigned long	nAudioEncType;
	unsigned long	nAudioChannels;
	unsigned long	nAudioSamples;
	unsigned long 	nAudioBits;
}AV_FORMAT, *PAV_FORMAT;
*/

typedef struct
{
	unsigned long	nVideoEncType;		//视频编码类型
	unsigned long	nImageWidth;		//视频宽度
	unsigned long	nImageHeight;		//视频高度
	unsigned long	nVideoBitRate;		//视频比特率
	unsigned long	nFrameRate;			//视频帧率
	unsigned long	nAudioEncType;		//音频编码类型 0x55-MP3 0x25-G711 0x65-722 0x64-G726
	unsigned long	nAudioChannels;		//音频通道数
	unsigned long	nAudioSamples;		//音频采样率
	unsigned long	nAudioBitRate;		//音频比特率
	unsigned long	nReserve;			//保留，设置为0

}AV_FORMAT, *PAV_FORMAT;

typedef struct
{
	unsigned long nCoverMode; 	// 0:覆盖,1:不覆盖
	unsigned long nAudioFlag;		// 0:不带音频数据 ,1:带音频数据

	AV_FORMAT avFormat;
	
	unsigned long nReserve;
	
}RECORD_CHANNEL_PARAM;

// 录像总参数
typedef struct
{
	RECORD_CHANNEL_PARAM record_param[MAX_AV_CHANNEL];
	TIMER_RECORD_CHANNEL_PARAM timer_record_param[MAX_AV_CHANNEL];
	VIDEOMOTION_RECORD_CHANNEL_PARAM videomotion_record_param[MAX_AV_CHANNEL];
	DETECTOR_RECORD_CHANNEL_PARAM detector_record_param[MAX_AV_CHANNEL];
	
}RECORD_SETUP;

// 查询指定通道指定月份录像情况
typedef struct
{
	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nType;
	unsigned long nReserve;
	
}QUERY_RECDATE;

// 返回指定通道指定月份录像情况
typedef struct
{
	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nType;
	unsigned long nDay[31];
	unsigned long nReserve;
	
}ACK_RECDATE;

// 查询指定通道指定日期录像情况
typedef struct
{
 	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nDay;
	unsigned long nType;
	unsigned long nReserve;
	
}QUERY_RECFILE;

// 返回指定通道指定月份录像情况
typedef struct
{
 	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nDay;
	unsigned long nType;
	unsigned long nNum;
	unsigned long nReserve;
	
}ACK_RECFILE;

// 录像文件的信息
typedef struct
{
	unsigned long nSize;
	unsigned long nPlayTime;
	char szFileName[64];
	unsigned long nReserve;
	
}REC_FILE_INFO;

// 文件时间段
typedef struct
{
	unsigned long nYear;
	unsigned long nMonth;
	unsigned long nDay;
	unsigned long nStartHour;
	unsigned long nStartMin;
	unsigned long nStartSecond;
	unsigned long nEndHour;
	unsigned long nEndMin;
	unsigned long nEndSecond;
	unsigned long nDisk;
	
}FILE_SEGMENT;

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

// 分区参数
typedef struct
{
	unsigned long nDiskNo;
	unsigned long nDataPartionSize;
	unsigned long nBackupPartionSize;
	unsigned long nReserve;
	
}HD_FDISK_PARAM;

// 格式化参数
typedef struct
{
	unsigned long nDiskNo;
	unsigned long nPartionNo;
	unsigned long nReserve;
	
}HD_FORMAT_PARAM;

typedef struct
{
	unsigned long nProgress;
	unsigned long nReserve;

}HD_FORMAT_PROGRESS;

typedef struct
{
	char szFileName[64];
	unsigned long nReadPos;
	unsigned long nReadLen;
	
}READ_REC_FILE;

typedef struct
{
	unsigned long nChannel;
	unsigned long nOpt;
	
	union 
	{
		char recFileName[64];
		
		QUERY_RECDATE recDate;
		QUERY_RECFILE recFile;

		FILE_SEGMENT fileSegment;
				
		unsigned char *frameBuffer;
		unsigned char *recFileInfo;
		
		MANUAL_RECORD_PARAM manualRecord;
		RECORD_CHANNEL_PARAM recordParam;
		TIMER_RECORD_CHANNEL_PARAM timerRecord;
		DETECTOR_RECORD_CHANNEL_PARAM detectorRecord;
		VIDEOMOTION_RECORD_CHANNEL_PARAM videoMotionRecord;
		
		HD_FDISK_PARAM fdisk;		
		HD_FORMAT_PARAM format;
		
		unsigned long videoStatus;
		unsigned long detectorStatus;
	}param;

	unsigned long nReserve;
}RECORDSDK_CMD_PARAM;

#endif

