#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "fileFormat.h"
#include "recordStruct.h"
#include "fileFormat.h"
#include "fileBak.h"
#include "fdisk.h"
#include "fileDelete.h"
#include "fileQuery.h"
#include "fileIndex.h"
#include "indexFile.h"
#include "hardDisk.h"
#include "hdPowerManage.h"
#include "mp4File.h"
#include "prerecord.h"
#include "recordFile.h"
#include "recordCtrl.h"
#include "util.h"

#ifdef SD_STORAGE
#include "sdCard.h"
#endif

static int get_record_file_info(char *fileName, REC_FILE_INFO *pRecFileInfo, void *pRecFileIndex, int *indexSize);

// 全局变量
static int g_hd_query_run_flag = 0;
static int g_hd_query_pause_flag = 0;

// 录像SDK打开
int RECORDSDK_Open(int nChnNum)
{
	if (nChnNum<=0 || nChnNum>MAX_AV_CHANNEL)
	{
		return -1;
	}

	set_av_channel_num(nChnNum);
	
	return 0;
}

// 录像SDK关闭
int RECORDSDK_Close()
{
	return 0;
}

// 获取录像SDK参数
int RECORDSDK_Setup(RECORD_SETUP setup)
{
	int ret = -1;

	ret = set_all_record_param(&setup);
	
	return ret;
}

// 设置SDK参数
int RECORDSDK_GetSetup(RECORD_SETUP *setup)
{
	int ret = -1;

	ret = get_all_record_param(setup);
	
	return ret;
}

// 启动录像SDK
int RECORDSDK_Start()
{
	int i = 0;
	int ret = -1;
	int nChnNum = 0;

	nChnNum = get_av_channel_num();

#ifdef SD_STORAGE
	// 卸载所有硬盘分区
	sd_umount();
	
	// 挂载所有硬盘分区
	ret = sd_mount();
	if (ret)
	{
		printf("sd_mount(): Failed!\n");
		//return -1;
	}
	
#else
	
	// 卸载所有硬盘分区
	hd_umount_all_partition();
	
	// 挂载所有硬盘分区
	ret = hd_mount_all_partition();
	if (ret)
	{
		printf("hd_mount_all_partition(): Failed!\n");
		//return -1;
	}
	
#endif
	printf("RECORDSDK_Start(%d) ...\n", nChnNum);
	// 启动录像线程
	for (i=0; i<nChnNum; i++)
	{
		ret = record_channel_start(i);
		if (ret)
		{
			printf("record_channel_start(%d): Failed!\n", i);
			return -1;
		}
	}
	
	// 启动录像处理线程
	ret = record_control_start();
	if (ret)
	{
		printf("record_control_start(): Failed!\n");
		return -1;
	}
	
	// 启动录像覆盖线程
	ret = hd_query_start();
	if (ret)
	{
		printf("hd_query_start(): Failed!\n");
		return -1;
	}
	
	// 启动硬盘电源管理线程

	printf("RECORDSDK_Start(): OK!\n");

	return 0;		
}

// 停止录像SDK
int RECORDSDK_Stop()
{
	int i = 0;
	int ret = -1;
	int nChnNum = 0;

	nChnNum = get_av_channel_num();
	
	ret = record_control_stop();
	if (ret)
	{
		return -1;
	}
	
	// 停止录像线程
	for (i=0; i<nChnNum; i++)
	{
		ret = record_channel_stop(i);
		if (ret)
		{
			return -1;
		}		
	}

	ret = hd_query_stop();
	if (ret)
	{
		return 0;
	}

	return 0;
}

// 暂停录像SDK
int RECORDSDK_Pause()
{
	int i = 0;
	int ret = -1;
	int nChnNum = 0;

	nChnNum = get_av_channel_num();

	// Delete the code by lvjh, 2008-08-27
	/*
	ret = hd_query_pause();
	if (ret)
	{
		return -1;
	}
	*/
	
	ret = record_control_pause();
	if (ret)
	{
		return -1;
	}

	// 停止录像线程
	for (i=0; i<nChnNum; i++)
	{
		ret = record_channel_pause(i);
		if (ret)
		{
			return -1;
		}
	}

	printf("RECORDSDK_Pause: OK!\n");
	
	return 0;
}

// 恢复录像SDK
int RECORDSDK_Resume()
{
	int i = 0;
	int ret = -1;
	int nChnNum = 0;

	nChnNum = get_av_channel_num();

	// 停止录像线程
	for (i=0; i<nChnNum; i++)
	{
		ret = record_channel_resume(i);
		if (ret)
		{
			return -1;
		}		
	}

	ret = record_control_resume();
	if (ret)
	{
		return -1;
	}

	/*
	ret = hd_query_resume();
	if (ret)
	{
		return -1;
	}
	*/
	
	return 0;
}

// 录像SDK控制操作
int RECORDSDK_Operate(void *inParam, void *outParam, int *outSize)
{
	int ret = -1;
	RECORDSDK_CMD_PARAM cmdParam;

	if (inParam == NULL)
	{
	
		return -1;	
	}
	
	memcpy(&cmdParam, inParam, sizeof(RECORDSDK_CMD_PARAM));
	
	switch (cmdParam.nOpt)
	{
	case RSDKCMD_SEND_FRAME:		// 发送帧数据去录像
       	{
			FRAME_HEADER * frameHead;

			if (cmdParam.param.frameBuffer == NULL)
			{
				return -1;
			}
			
			frameHead = (FRAME_HEADER *)cmdParam.param.frameBuffer;

			ret = send_one_frame_to_recorder(cmdParam.nChannel, cmdParam.param.frameBuffer,
						sizeof(FRAME_HEADER)+frameHead->nVideoSize+frameHead->nAudioSize);
		}
		break;

	case RSDKCMD_SEND_JPEG:		// 发送帧数据去录像
       	{


			#if 1
 		struct timeval tpstart,tpend; 
		float timeuse; 
		gettimeofday(&tpstart,NULL); 
		
		#endif
       		int nResult = -1;
			FILE *fp = NULL;
			int nJpegSize = 0;
			char fileName[256];
			char indexName[64];
			int year, month, day, hour, minute, second;
			REC_FILE_INFO recFileInfo;
			
			if (cmdParam.param.frameBuffer == NULL)
			{
				return -1;
			}
			
			nJpegSize = *(int *)outSize;
			if (nJpegSize<=0 || nJpegSize>200*1024)
			{
				return -1;
			}

			get_system_time(&year, &month, &day, &hour, &minute, &second);
			
			memset(fileName, 0, 256);
			memset(indexName, 0, 64);
			
			// Add the code by lvjh, 2011-03-30
#ifdef SD_STORAGE
			if (sd_get_mount_flag() != 1)
			{
				return -1;
			}
			if (sd_get_readwrite_flag() != 1)
			{
				return -1;
			}
#endif

			nResult = get_snapshot_file_name(cmdParam.nChannel, fileName);
			if (nResult < 0)
			{
				return -1;
			}

			fp = fopen(fileName, "w+b");
			if (fp == NULL)
			{
				return -1;
			}

			ret = fwrite(cmdParam.param.frameBuffer, 1, nJpegSize, fp);
			if (ret != nJpegSize)
			{
				fclose(fp);

				return -1;
			}
			fflush(fp);
			fclose(fp);

			memset(indexName, 0, 64);
			memset(&recFileInfo, 0, sizeof(REC_FILE_INFO));
			get_snapshot_index_name(cmdParam.nChannel, indexName);
			recFileInfo.nSize = nJpegSize;
			recFileInfo.nPlayTime = 0;
			strncpy(recFileInfo.szFileName, fileName, 64);
			write_jpeg_index_file(indexName, recFileInfo);

			
#if 0
			gettimeofday(&tpend,NULL); 
			timeuse=1000000*(tpend.tv_sec-tpstart.tv_sec)+ 
			tpend.tv_usec-tpstart.tv_usec; 
			timeuse/=1000000; 
			printf("Used Time:%fn\n",timeuse); 
#endif
			
			ret = 0;
		}
		break;
		
	case RSDKCMD_PAUSE_RECORD:	// 暂停录像
		{
			ret = RECORDSDK_Pause();
		}
		break;
		
	case RSDKCMD_RESUME_RECORD:
		{
			ret = RECORDSDK_Resume();
		}
		break;
		
	case RSDKCMD_QUERY_RECDATE:	// 按日期进行查询，检查在指定的月份下，每天录像情况
		{       
			int bSuccess = 0;
			int i = 0;
			int day_num = 0;
		   	int record_type = 0;
			int year = 0;
			int month = 0;
			int channel_num = get_av_channel_num();
			ACK_RECDATE recDate;

			memset(&recDate, 0, sizeof(ACK_RECDATE));
			
			year = cmdParam.param.recDate.nYear;
			month = cmdParam.param.recDate.nMonth;
			record_type = cmdParam.param.recDate.nType;
			//if (!record_type)
			{
				record_type = RECORD_TYPE_TIME | RECORD_TYPE_VIDEO_MOVE | RECORD_TYPE_ALARM_INPUT | RECORD_TYPE_NORMAL;
			}
			
			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			do
			{
				// 检查参数是否合法
				if (cmdParam.nChannel < 0 && cmdParam.nChannel >= channel_num)
				{
					break;
				}
				if (year < 1970 && year > 2050)
				{
					break;
				}
				if (month < 1 && month > 12)
				{
					break;
				}
				bSuccess = 1;
			} while(0);
			
			if (0 == bSuccess)
			{
				return -1;
			}

			recDate.nYear = year;
			recDate.nMonth = month;
			recDate.nType = record_type;
			
			day_num = get_day_num(year, month);
			for (i=0; i<day_num; i++)
			{
				//if (find_record_file_by_type(cmdParam.nChannel, year, month, i+1, record_type)) // 查询录像
				if (find_record_file_by_type_ext(cmdParam.nChannel, year, month, i+1, record_type) == 1) // Add the code by lvjh, 2010-11-30
				{
					recDate.nDay[i] = 1;
				}
				else
				{
					recDate.nDay[i] = 0;
				}
				//printf("Record: %d %d\n", i, recDate.nDay[i]);
			}

			memcpy(outParam, &recDate, sizeof(ACK_RECDATE));
			*outSize = sizeof(ACK_RECDATE);

			ret = 0;
	       }
		break;


	case RSDKCMD_QUERY_RECFILE:	// 查询录像文件, 以单向链表的方式，来存诸录像文件信息
		{
#if 1
			int record_type = 0;
			int year = 0;
			int month = 0;
			int day = 0;

			year = cmdParam.param.recFile.nYear;
			month = cmdParam.param.recFile.nMonth;
			day = cmdParam.param.recFile.nDay;
			record_type = cmdParam.param.recFile.nType;
			if (!record_type)
			{
				record_type = RECORD_TYPE_TIME | RECORD_TYPE_VIDEO_MOVE | RECORD_TYPE_ALARM_INPUT | RECORD_TYPE_NORMAL;
			}

			ret = open_file_search_by_index(cmdParam.nChannel, year, month, day,
							0, 0, 0, 23, 59, 59, 
							record_type, 
							outParam, outSize);
			if (ret < 0)
			{
				printf("open_file_search_by_index: Failed!\n");
			}
			else
			{
				printf("open_file_search_by_index: OK!\n");
			}
#else
			FILE_LIST *pFileList = NULL;
			REC_FILE_INFO *pRecFileInfo = NULL;
			int i = 0;
			int record_type = 0;
			int year = 0;
			int month = 0;
			int day = 0;
			int fileCount = 0;
			int channel_num = get_av_channel_num();

			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}

			year = cmdParam.param.recFile.nYear;
			month = cmdParam.param.recFile.nMonth;
			day = cmdParam.param.recFile.nDay;
			record_type = cmdParam.param.recFile.nType;
			if (!record_type)
			{
				record_type = RECORD_TYPE_TIME | RECORD_TYPE_VIDEO_MOVE | RECORD_TYPE_ALARM_INPUT | RECORD_TYPE_NORMAL;
			}

			pFileList = open_file_search(cmdParam.nChannel, 0, year, month, day,
									0, 0, 0, 23, 59, 59);
			if (pFileList == NULL)
			{
				break;
			}

			fileCount = get_file_count(pFileList);
			if (!fileCount)
			{
				*outSize = 0;
				close_file_search(pFileList);
				ret = 0;
			}
			else
			{		
				pRecFileInfo = (REC_FILE_INFO *)malloc(sizeof(REC_FILE_INFO)*fileCount);
				if (pRecFileInfo == NULL)
				{
					close_file_search(pFileList);
					return -1;
				}

				for (i=0; i<fileCount; i++)
				{
					get_file_attr_by_index(pFileList, pRecFileInfo[i].szFileName, &(pRecFileInfo[i].nSize), &(pRecFileInfo[i].nPlayTime), i);
				}

				memcpy(outParam, pRecFileInfo, sizeof(REC_FILE_INFO)*fileCount);
				*outSize = sizeof(REC_FILE_INFO)*fileCount;
				
				free(pRecFileInfo);
				close_file_search(pFileList);

				ret = 0;
			}
#endif
		}
		break;

	case RSDKCMD_QUERY_JPEGDATE:	// 按日期进行查询，检查在指定的月份下，每天录像情况
		{       
			int bSuccess = 0;
			int i = 0;
			int day_num = 0;
		   	int record_type = 0;
			int year = 0;
			int month = 0;
			int channel_num = get_av_channel_num();
			ACK_RECDATE recDate;

			memset(&recDate, 0, sizeof(ACK_RECDATE));
			
			year = cmdParam.param.recDate.nYear;
			month = cmdParam.param.recDate.nMonth;
			record_type = cmdParam.param.recDate.nType;
			//if (!record_type)
			{
				record_type = RECORD_TYPE_TIME | RECORD_TYPE_VIDEO_MOVE | RECORD_TYPE_ALARM_INPUT | RECORD_TYPE_NORMAL;
			}
			
			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			do
			{
				// 检查参数是否合法
				if (cmdParam.nChannel < 0 && cmdParam.nChannel >= channel_num)
				{
					break;
				}
				if (year < 1970 && year > 2050)
				{
					break;
				}
				if (month < 1 && month > 12)
				{
					break;
				}
				bSuccess = 1;
			} while(0);
			
			if (0 == bSuccess)
			{
				return -1;
			}

			recDate.nYear = year;
			recDate.nMonth = month;
			recDate.nType = record_type;
			
			day_num = get_day_num(year, month);
			for (i=0; i<day_num; i++)
			{
				if (find_jpeg_file_by_type(cmdParam.nChannel, year, month, i+1, record_type)) // 查询录像
				{
					recDate.nDay[i] = 1;
				}
				else
				{
					recDate.nDay[i] = 0;
				}
				//printf("Record: %d %d\n", i, recDate.nDay[i]);
			}

			memcpy(outParam, &recDate, sizeof(ACK_RECDATE));
			*outSize = sizeof(ACK_RECDATE);

			ret = 0;
	       }
		break;


	case RSDKCMD_QUERY_JPEGFILE:
		{
			int record_type = 0;
			int year = 0;
			int month = 0;
			int day = 0;
			int hour = 30;

			year = cmdParam.param.recFile.nYear;
			month = cmdParam.param.recFile.nMonth;
			day = cmdParam.param.recFile.nDay;
			hour = cmdParam.param.recFile.nReserve;
			record_type = cmdParam.param.recFile.nType;
			if (!record_type)
			{
				record_type = RECORD_TYPE_TIME | RECORD_TYPE_VIDEO_MOVE | RECORD_TYPE_ALARM_INPUT | RECORD_TYPE_NORMAL;
			}
			printf("Hour: %d\n", cmdParam.param.recFile.nReserve);
			*outSize = 0;
			ret = open_jpeg_search_by_index(cmdParam.nChannel, year, month, day,
							hour, 0, 0, hour, 59, 59, 
							record_type, 
							outParam, outSize);
			if (ret < 0)
			{
				printf("open_jpeg_search_by_index(%d %d %d %d): Failed!\n", year, month, day, hour);
			}
			else
			{
				printf("open_jpeg_search_by_index(%d %d %d %d): %d OK!\n", year, month, day, hour, *outSize);
			}
		}
		break;
	
	case RSDKCMD_OPEN_RECFILE:	// 打开录像文件
	
		break;
		
	case RSDKCMD_CLOSE_RECFILE:	// 关闭录像文件
	
		break;


	case RSDKCMD_GET_RECFILE_INFO:	// 读取录像文件
		{
		   	REC_FILE_INFO recFileInfo;
			char recFileIndex[10*1024];
			int indexSize = 0;

			if (cmdParam.param.recFileInfo == NULL)
			{
				return -1;
			}				
			ret = get_record_file_info(cmdParam.param.recFileName, &recFileInfo, recFileIndex, &indexSize);
			if (ret)
			{
				return -1;
			}

			memcpy(cmdParam.param.recFileInfo, &recFileInfo, sizeof(REC_FILE_INFO));
			memcpy(cmdParam.param.recFileInfo+sizeof(REC_FILE_INFO), recFileIndex, indexSize);
		}
		break;
		
	case RSDKCMD_DELETE_RECFILE:	// 删除录像文件
		//ret = del_oldest_record_file();	// 只能删除最老的一天的录像文件		
		break;
		
	case RSDKCMD_BACKUP_RECFILE_LOCAL:	// 本地备份录像文件
		{
			FILE_SEGMENT backupFile;
		
			memcpy(&backupFile, &cmdParam.param.fileSegment, sizeof(FILE_SEGMENT));
		
			ret = start_backup_proc(cmdParam.nChannel, backupFile.nYear, backupFile.nMonth, backupFile.nDay,
								backupFile.nStartHour, backupFile.nStartMin, backupFile.nEndHour, backupFile.nEndMin,
								0, backupFile.nDisk);
		}
		break;

		
	case RSDKCMD_BACKUP_RECFILE_REMOTE:	// 远程备份录像文件
		{

		}
		break;		
		
	case RSDKCMD_GET_RECORD_PARAM:	// 获取录像参数
		{
			RECORD_CHANNEL_PARAM *param = (RECORD_CHANNEL_PARAM *)outParam;

			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			ret = get_record_param(cmdParam.nChannel, param);
		}
		break;
	
	case RSDKCMD_GET_MANUAL_RECORD_PARAM:	// 获取手动录像参数
		
		break;
		
	case RSDKCMD_GET_TIMER_RECORD_PARAM:		// 获取定时录像参数
		{
			TIMER_RECORD_CHANNEL_PARAM *param = (TIMER_RECORD_CHANNEL_PARAM *)outParam;

			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			ret = get_timer_record_param(cmdParam.nChannel, param);
		}
		break;
		
	case RSDKCMD_GET_DETECTOR_RECORD_PARAM:	// 获取探头报警录像参数
		{
			DETECTOR_RECORD_CHANNEL_PARAM *param = (DETECTOR_RECORD_CHANNEL_PARAM *)outParam;

			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			ret = get_detector_record_param(cmdParam.nChannel, param);
		}
		break;
		
	case RSDKCMD_GET_VIDEOMOVE_RECORD_PARAM:	// 获取视频移动报警录像参数
		{
			VIDEOMOTION_RECORD_CHANNEL_PARAM *param = (VIDEOMOTION_RECORD_CHANNEL_PARAM *)outParam;

			if (outParam==NULL || outSize==NULL)
			{
				return -1;	
			}
			
			ret = get_videomotion_record_param(cmdParam.nChannel, param);
		}
		break;	
	
	case RSDKCMD_GET_RECORD_STATUS:
	
		break;

#ifdef SD_STORAGE
	case RSDKCMD_GET_HARDDISK_INFO:
		{
			SD_CARD_INFO sdCardInfo;
			HARD_DISK_INFO *param = (HARD_DISK_INFO *)outParam;
			
			if (outParam==NULL || outSize==NULL)
			{
				return -1;
			}
			
			ret = sd_get_info(&sdCardInfo);
			if (ret < 0)
			{
				memset(param, 0, sizeof(HARD_DISK_INFO));

				*outSize = sizeof(HARD_DISK_INFO);
			}
			else
			{
				memset(param, 0, sizeof(HARD_DISK_INFO));

				param->cur_disk_no = 1;
				param->cur_partition_no = 1;
				param->hard_disk_num = 1;

				param->disk_info[0].nPartitionNum = 1;
				param->disk_info[0].partition_info[0].format_flag = sdCardInfo.formated;
				param->disk_info[0].partition_info[0].mount_flag = sdCardInfo.mount_flag;
				param->disk_info[0].partition_info[0].total_size = sdCardInfo.total_size;
				param->disk_info[0].partition_info[0].used_size = sdCardInfo.used_size;
				param->disk_info[0].partition_info[0].availabe_size = sdCardInfo.availabe_size;
				param->disk_info[0].partition_info[0].used_parent = sdCardInfo.used_parent;
				param->disk_info[0].partition_info[0].type = 1;
			
				param->disk_info[0].partition_info[1].type = 2;

				*outSize = sizeof(HARD_DISK_INFO);
			}
		}
		break;
#else
	case RSDKCMD_GET_HARDDISK_INFO:	// 获取硬盘信息
		{
			HARD_DISK_INFO *param = (HARD_DISK_INFO *)outParam;
			if (outParam==NULL || outSize==NULL)
			{
				return -1;
			}
			ret = get_hard_disk_info(param);
			if (ret < 0)
			{
				*outSize = 0;
			}
			else
			{
				*outSize = sizeof(HARD_DISK_INFO);
			}
		}
		break;
#endif

	case RSDKCMD_SET_RECORD_PARAM:	// 设置录像参数		
		ret = set_record_param(cmdParam.nChannel, &cmdParam.param.recordParam);
		break;
		
	case RSDKCMD_SET_MANUAL_RECORD_PARAM:	// 设置手动参数	
		ret = set_manual_record_param(cmdParam.nChannel, cmdParam.param.manualRecord.nTime, cmdParam.param.manualRecord.nOnFlag);
		break;	
		
	case RSDKCMD_SET_TIMER_RECORD_PARAM:	// 设置定时录像参数	
		ret = set_timer_record_param(cmdParam.nChannel, &cmdParam.param.timerRecord);
		break;	
		
	case RSDKCMD_SET_DETECTOR_RECORD_PARAM:	// 设置探头报警录像参数	
		printf("RSDKCMD_SET_DETECTOR_RECORD_PARAM\n");
		ret = set_detector_record_param(cmdParam.nChannel, &cmdParam.param.detectorRecord);
		break;	
		
	case RSDKCMD_SET_VIDEOMOVE_RECORD_PARAM:	// 设置视频移动报警录像参数	
		ret = set_videomotion_record_param(cmdParam.nChannel, &cmdParam.param.videoMotionRecord);
		break;

#ifdef SD_STORAGE
	case RSDKCMD_SET_HARDDISK_FDISK:	// 分区
		RECORDSDK_Pause();
		ret = sd_fdisk();
		RECORDSDK_Resume();
		break;
		
	case RSDKCMD_SET_HARDDISK_FORMAT:	// 格式化
		RECORDSDK_Pause();
		sleep(5);
		ret = sd_format();
		RECORDSDK_Resume();
		sd_query_info();
		break;

	case RSDKCMD_GET_FDISK_PROCESS:
		{
			FILE *fp = NULL;
			unsigned long process = 0;
			
			HD_FORMAT_PROGRESS *param = (HD_FORMAT_PROGRESS *)outParam;
			if (outParam==NULL || outSize==NULL)
			{
				return -1;
			}

			memset(param, 0, sizeof(HD_FORMAT_PROGRESS));
			
			//param->nProgress = 100;
			
			// Add the code by lvjh, 2011-04-02
			fp = fopen("/format.hex", "rb");
			if (fp == NULL)
			{
				param->nProgress = 0;
			}
			else
			{
				process = 0;
				fread(&process, 1, sizeof(unsigned long), fp);
				printf("HD_FORMAT_PROGRESS: %d\n", process);
				fclose(fp);
				if (process > 100)
				{
					process = 100;
				}
				param->nProgress = process;
			}

			*outSize = sizeof(HD_FORMAT_PROGRESS);
		}
		break;
#else		
	case RSDKCMD_SET_HARDDISK_FDISK:	// 分区
		{
			int cur_disk = 0;

			cur_disk = get_cur_disk_no();
			if (cur_disk == cmdParam.param.fdisk.nDiskNo)
			{
				RECORDSDK_Pause();
			}
			ret = hd_fdisk(cmdParam.param.fdisk.nDiskNo, cmdParam.param.fdisk.nDataPartionSize, cmdParam.param.fdisk.nBackupPartionSize);
			if (cur_disk == cmdParam.param.fdisk.nDiskNo)
			{
				RECORDSDK_Resume();
			}
		}
		break;
		
	case RSDKCMD_SET_HARDDISK_FORMAT:	// 格式化
		{
			int cur_disk = 0;

			cur_disk = get_cur_disk_no();
			if (cur_disk == cmdParam.param.fdisk.nDiskNo)
			{
				RECORDSDK_Pause();
			}
			ret = hd_format(cmdParam.param.format.nDiskNo, cmdParam.param.format.nPartionNo);
			if (cur_disk == cmdParam.param.fdisk.nDiskNo)
			{
				RECORDSDK_Resume();
			}
		}
		break;

	case RSDKCMD_GET_FDISK_PROCESS:
		{
			HD_FORMAT_PROGRESS *param = (HD_FORMAT_PROGRESS *)outParam;
			if (outParam==NULL || outSize==NULL)
			{
				return -1;
			}

			memset(param, 0, sizeof(HD_FORMAT_PROGRESS));
			param->nProgress = 100;

			*outSize = sizeof(HARD_DISK_INFO);
		}
		break;
#endif
		
	case RSDKCMD_SET_VIDEO_STATUS:	// 设置视频状态
		//printf("RSDKCMD_SET_VIDEO_STATUS\n");
		ret = set_video_status(cmdParam.nChannel, cmdParam.param.videoStatus);
		break;
		
	case RSDKCMD_SET_DETECTOR_STATUS:	// 设置探头状态
		ret = set_detector_status(cmdParam.nChannel, cmdParam.param.detectorStatus);
		break;
				
	default:
		ret = -1;
		break;
	}
	
	return ret;
}

int hd_query_fun()
{
	int disk_num = 0;
	int i = 0;
	int j = 61;
	int old_second = 0;
	int new_second = 0;
	int formatFlag = 0;
	int ret = 0;
	int nChnNum = 0;
	RECORD_CHANNEL_PARAM param;
	int flag = 0;

	nChnNum = get_av_channel_num();

#ifdef SD_STORAGE
	sd_query_info();
#else
	hd_query_disk_info_ext();
#endif
	
	while (g_hd_query_run_flag)
	{
	//	del_oldest_file_ext_fun();
					
		if (g_hd_query_pause_flag)
		{
			sleep(1);
			continue;
		}

		//printf("[test]: hd_query_fun()\n");

#ifdef SD_STORAGE
		new_second = get_second();
		if (new_second < old_second || new_second%60 == 0)
		{
			/*
			ret = sd_get_formated();
			if (ret != 1)
			{
				sleep(60);
				continue;
			}
			*/

			// Add the code by lvjh, 2008-11-01
			ret = sd_check();
			if (ret == -1)
			{
				sleep(60);
				continue;
			}
			
			// Add the code by lvjh, 2008-11-04
			ret = sd_get_mount_flag();
			if (ret == 0)
			{
				sleep(60);
				continue;
			}

			sd_query_info();

			for (i=0; i<nChnNum; i++)
			{
				switch_record_file(i);
			}

			// 获取录像参数
			get_record_param(0, &param);
			
			if (!param.nCoverMode)
			{
				if (sd_get_full())
				{
					RECORDSDK_Pause();
					printf("SD Full, pause record!\n");
					flag = 1;
				}
				else
				{
					RECORDSDK_Resume();
					flag = 0;
				}
			}
			else
			{
				if (flag)
				{
					RECORDSDK_Resume();
					flag = 0;
				}

				while (sd_get_full())
				{
					//删除最老文件
					printf("SD Full, delete the oldest record file!\n");
					//del_oldest_record_file_ext();
					//del_oldest_file_fun();
					del_oldest_file_ext_fun();
					
					// Add the code by lvjh, 2009-03-27
					sleep(10);
				}
			}
		}
		old_second = new_second;

		sleep(1);
#else
		/*
		if (j > 60)
		{
			disk_num = get_hard_disk_num();
			if (disk_num > 0)
			{
				for (i=0; i<disk_num; i++)
				{
					ret = hd_get_disk_formated(i);
					if (!ret)
					{
						formatFlag++; 
					}
				}

				if (formatFlag >= disk_num)
				{
					sleep(60);
					j = 0;
					continue;
				}
				
				ret = hd_query_disk_info();
				switch (ret)
				{
				case 0: // deleting the oldest record files
					printf("[test]: deleting the oldest record files!\n");
					break;

				case 1: // alarm and pause record
					printf("[test]: hard disk full and pause record!\n");
					break;

				case 2: // delete the oldest record files and record
					printf("[test]: delete the oldest record files and record!\n");
					break;

				default:
					break;
				}
			}
			else
			{
				RECORDSDK_Stop();
			}
			
			j = 0;
		}
		*/
		new_second = get_second();
		if (new_second < old_second || new_second%60 == 0)
		{
			int ret = -1;

			HARD_DISK_INFO hdInfo;
			
			for (i=0; i<nChnNum; i++)
			{
				switch_record_file(i);
			}

			hd_query_disk_info_ext();
			ret = get_hard_disk_info(&hdInfo);
			if (ret)
			{
				printf("[test]: nvsRecorderGetHdInfo Failed!\n");
			}
			else
			{
				printf("[test]: HD00(%d %d %d %d)\n",
					hdInfo.disk_info[0].partition_info[0].total_size, hdInfo.disk_info[0].partition_info[0].used_size,
					hdInfo.disk_info[0].partition_info[0].availabe_size, hdInfo.disk_info[0].partition_info[0].used_parent);
				printf("[test]: HD01(%d %d %d %d)\n",
					hdInfo.disk_info[0].partition_info[1].total_size, hdInfo.disk_info[0].partition_info[1].used_size,
					hdInfo.disk_info[0].partition_info[1].availabe_size, hdInfo.disk_info[0].partition_info[1].used_parent);
			}
			
		}
		old_second = new_second;

		j++;
		sleep(1);
#endif
	}
	
	pthread_exit(NULL);
	
	return 0;	
}

int hd_query_start()
{
	int ret = -1;
	pthread_t threadID;

	g_hd_query_run_flag = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)hd_query_fun, NULL);
	if (ret)
	{
		g_hd_query_run_flag = 0;
		
		return -1;
	}

	return 0;
}

int hd_query_stop()
{
	g_hd_query_run_flag = 0;

	return 0;
}

int hd_query_pause()
{
	g_hd_query_pause_flag = 1;

	return 0;
}

int hd_query_resume()
{
	g_hd_query_pause_flag = 0;

	return 0;
}

static int get_record_file_info(char *fileName, REC_FILE_INFO *pRecFileInfo, void *pRecFileIndex, int *indexSize)
{
	int ret = -1;
	FILE *fd = NULL;
	int count = 0;
	TDS_FILEHEADER fileHead;

	if (fileName==NULL || pRecFileInfo==NULL ||pRecFileIndex==NULL)
	{
		return -1;
	}

	fd = fopen(fileName, "r+b");
	if (fd < 0)
	{
		return -1;
	}

	ret = fread(&fileHead, 1, sizeof(TDS_FILEHEADER), fd);
	if (ret < sizeof(TDS_FILEHEADER))
	{
		fclose(fd);
		
		return -1;
	}

	memcpy(pRecFileInfo->szFileName, fileName, 64);
	pRecFileInfo->nSize = fileHead.File_Size;
	pRecFileInfo->nPlayTime = fileHead.Play_Duration;
	pRecFileInfo->nReserve = 0;

	fseek(fd, fileHead.Index_Position, SEEK_SET);

	while (!feof(fd))
	{
		ret = fread(pRecFileIndex+count*sizeof(TDS_INDEXENTRIES), 1, sizeof(TDS_INDEXENTRIES), fd);
		if (ret <= 0)
		{	
			break;
		}			

		count++;
	}

	fclose(fd);

	*indexSize = count*sizeof(TDS_INDEXENTRIES);

	return 0;
}

