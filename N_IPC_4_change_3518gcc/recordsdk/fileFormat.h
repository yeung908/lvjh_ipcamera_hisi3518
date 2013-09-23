/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：fileFormat.h
* 文件说明：该文件描述了录像文件格式
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

#ifndef __FILE_FORMAT_H_
#define __FILE_FORMAT_H_

#include "baseType.h"

#define PACK_SIZE			1024*4

#define TDSFLAG				0x53564448
#define INDEXFLAG			0xDDDDDDDD

//视频编码类型
#define ENCODE_VIDEO_DIVX	0x58564944
#define ENCODE_VIDEO_XVID	0x44495658
#define ENCODE_VIDEO_HISI	0x49534948
#define ENCODE_VIDEO_H264	0x34363248


//音频编码类型
#define ENCODE_AUDIO_MP3	0x55
#define ENCODE_AUDIO_G722 	0x65
#define ENCODE_AUDIO_G711	0x6

//#define ENCODE_AUDIO_G711	0x25
#define ENCODE_AUDIO_G726	0x45

#define HDVSF_HASINDEX	0x01
#define HDVSF_HASVIDEO	0x02
#define HDVSF_HASAUDIO	0x04

typedef struct
{
	unsigned long  nTimeTick;
	unsigned long  nVideoSize;
	unsigned short nAudioSize;
	unsigned short nImgWidth;
	unsigned short nImgHeight;
	unsigned short nKeyFrame;
}FRAME_HEADER, *PFRAME_HEADER;

typedef struct
{
	WORD	nYear;
	BYTE	nMonth;
	BYTE	nDay;
	WORD nHour;
	BYTE	nMinute;
	BYTE	nSecond;
}TDS_FILETIME, *PTDS_FILETIME;

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

/*
typedef struct
{
	unsigned long	nVideoEncType;
	unsigned long	nImageWidth;
	unsigned long	nImageHeight;
	unsigned long	nAudioEncType;
	unsigned long	nAudioChannels;
	unsigned long	nAudioSamples;
	unsigned long nAudioBits;
}AV_FORMAT, *PAV_FORMAT;
*/

typedef struct
{
	DWORD	File_Flag;
	DWORD	File_Size;
	TDS_FILETIME	Creation_Date;
	DWORD	Play_Duration;		//播放时间，毫秒单位，
	DWORD	BeginTimeTick;
	DWORD	EndTimeTick;
	DWORD	File_Type;			//文件类型

	DWORD Index_Position;
	AV_FORMAT AV_Format;
	char Title[32];
	BYTE Reserved[32];	
}TDS_FILEHEADER, *PTDS_FILEHEADER;

typedef struct
{
	DWORD Index_Flag;		//=0xDDDDDDDD
	DWORD Index_Count;
}TDS_INDEXOBJECT, *PTDS_INDEXOBJECT;

typedef struct
{
	DWORD Index_TimeTick;
	DWORD Index_Offset;
}TDS_INDEXENTRIES, *PTDS_INDEXENTRIES;

typedef struct
{
	unsigned char bIsDataHead;
	unsigned char nChannel;
	unsigned short nBufSize;
	unsigned long bPacketID;
	FRAME_HEADER FrameHeader;
	unsigned char PackData[PACK_SIZE];
}DATA_PACKET, *PDATA_PACKET;

#endif

