#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "snapshot.h"
#include "param.h"
#include "session.h"
#include "iconv.h"

#ifdef RECORD
#include "include/recordSDK.h"
#endif

int g_snapshot_upload_mode = 0;
int g_snapshot_channel = 0;
char *g_snapshot_buffer = NULL;
int g_snapshot_buffer_size = 0;
pthread_mutex_t g_snapshot_buffer_mutex;
sem_t g_snapshot_sem;

int g_auto_snapshot_socket = -1;
int g_manual_snapshot_socket = -1;

int g_snapshot_run = 0;
int g_snapshot_pause = 0;

int g_alarm_info_type = 0;
int g_alarm_info_upload_mode = 0;
char *g_alarm_info[4] = 
{
	"NULL",
	"Video Lost",
	"Video Motion",
	"Detector",
};

int g_snapshot_email_type = 0; // 1:移动报警，2:探头报警

static int setSnapshotAutoUpload()
{
	int ret = -1;
	unsigned long nID = 0;
	char addr[16];
	
	if (addr == NULL)
	{
		return -1;
	}

	getJpegIPParam(addr);
	
	inet_pton(AF_INET, addr, &nID);
	
	ret = NETSDK_GetUserId(nID);
	if (ret > 0)
	{
		g_auto_snapshot_socket = ret;
		
		return 0;
	}
	else
	{
		g_auto_snapshot_socket = -1;
		
		return -1;
	}
}

int SNAPSHOT_Init()
{
	int ret = -1;
	
	g_snapshot_buffer = (char *)malloc(400*1024);
	if (g_snapshot_buffer == NULL)
	{
		return -1;
	}
			
	ret = pthread_mutex_init(&g_snapshot_buffer_mutex, NULL);
	if (ret < 0)
	{
		return -1;
	}
	
	ret = sem_init(&g_snapshot_sem, 0, 0);
	if (ret < 0)
	{
		return -1;
	}
			
	g_manual_snapshot_socket = -1;
	g_auto_snapshot_socket = -1;
	
	return 0;
}

int SNAPSHOT_Destroy()
{	
	if (g_snapshot_buffer)
	{
		free(g_snapshot_buffer);
		g_snapshot_buffer = NULL;
	}
			
	pthread_mutex_destroy(&g_snapshot_buffer_mutex);
	
	sem_destroy(&g_snapshot_sem);
				
	g_manual_snapshot_socket = -1;
	g_auto_snapshot_socket = -1;
	
	return 0;
}

int snapshotFun()
{
	int i = 0;
	int ret = -1;
	int flags = 0;

	NET_HEAD netHead;
	DEV_CMD_RETURN devCmdReturn;
	DATE_PARAM date;

#ifdef RECORD
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	while (g_snapshot_run)
	{
		if (g_snapshot_pause)
		{
			sleep(1);
			continue;
		}
		sem_wait(&g_snapshot_sem);
				

#ifdef RECORD
		// record SDK
		cmdParam.nChannel = g_snapshot_channel;
		cmdParam.nOpt = RSDKCMD_SEND_JPEG;
		cmdParam.param.frameBuffer = g_snapshot_buffer;
		ret = RECORDSDK_Operate(&cmdParam, NULL, &g_snapshot_buffer_size);
		//printf("ret = %d\n", ret);
#endif
		
		switch (g_alarm_info_upload_mode)
		{
		case EMAIL_UPLOAD:
			{
				char context[1024];
				
				EMAIL_PARAM param;

				getEmailParam(&param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				if (g_alarm_info_type<=0 || g_alarm_info_type>=4)
				{
					break;
				}

				sprintf(context, "Alarm Type: %s\nAlarm Date: %04d/%02d/%02d %02d:%02d:%02d\n", g_alarm_info[g_alarm_info_type], date.year, date.month, date.day, date.hour, date.minute, date.second);
				
				ret = SMTP_Send_Ext(param.strIP, param.strName, param.strPsw,
							param.strFrom, param.strTo, param.strCc, param.strBcc, NULL,
							"Alarm", context, NULL, NULL, 0);
							
				g_alarm_info_upload_mode = -1;

				printf("EMAIL_UPLOAD: %d\n %s\n", ret, context);	
			}
			break;

		case FTP_UPLOAD:
			{
				char fileName[128];
				char context[1024];
				FTP_PARAM param;

				getFtpParam(&param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				if (g_alarm_info_type<=0 || g_alarm_info_type>=4)
				{
					break;
				}

				sprintf(fileName, "Alarm_%04d%02d%02d%02d%02d%02d.txt", date.year, date.month, date.day, date.hour, date.minute, date.second);
				sprintf(context, "Alarm Type: %s\nAlarm Date: %04d/%02d/%02d %02d:%02d:%02d\n", g_alarm_info[g_alarm_info_type], date.year, date.month, date.day, date.hour, date.minute, date.second);

				printf("param.strIP = %s, 	param.nPort = %d\n", param.strIP, param.nPort);
				ret = FTP_PutFile_Ext(param.strIP, param.nPort, param.strName, param.strPsw, fileName, context, strlen(context), fileName);
				
				g_alarm_info_upload_mode = -1;

				printf("FTP_UPLOAD: %d %s\n", ret, fileName);
			}
			break;

		case TFTP_UPLOAD:
			{
				char fileName[128];
				char context[1024];
				int len = 0;
				TFTP_PARAM param;

				getTftpParam(&param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				if (g_alarm_info_type<=0 || g_alarm_info_type>=4)
				{
					break;
				}

				sprintf(fileName, "Alarm_%04d%02d%02d%02d%02d%02d.txt", date.year, date.month, date.day, date.hour, date.minute, date.second);
				sprintf(context, "Alarm Type: %s\nAlarm Date: %04d/%02d/%02d %02d:%02d:%02d\n", g_alarm_info[g_alarm_info_type], date.year, date.month, date.day, date.hour, date.minute, date.second);

				len = strlen(context);
				ret = TFTP_PutFile_Ext(param.strIP, param.nPort, fileName, context, &len);
				
				g_alarm_info_upload_mode = -1;
				
				printf("TFTP_UPLOAD: %d %s\n", ret, fileName);
			}
			break;
			
		default:
			g_alarm_info_upload_mode = -1;
			break;
		}

		switch (g_snapshot_upload_mode)
		{
		case SPECIAL_IP_MANUAL_UPLOAD:
			{
				printf("SPECIAL_IP_MANUAL_UPLOAD ...\n");
				manual_record_result(DEV_SNAPSHOT_RESULT);
				if (g_manual_snapshot_socket <= 0)
				{
					return -1;
				}

				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_DEV_DATA;
				netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DATE_PARAM)+g_snapshot_buffer_size;
				
				devCmdReturn.nCmdID = DEV_JPEG_DATA;
				devCmdReturn.nCmdLen = sizeof(DATE_PARAM)+g_snapshot_buffer_size;
				devCmdReturn.nReserve = g_snapshot_channel;
					
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				/*
				memcpy(g_snapshot_buffer, &netHead, sizeof(NET_HEAD));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &date, sizeof(DATE_PARAM));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(DATE_PARAM), g_snapshot_buffer, g_snapshot_buffer_size);

				ret = send(g_manual_snapshot_socket, g_snapshot_buffer, sizeof(NET_HEAD)+netHead.nBufSize, 0);
				*/
				ret = send(g_manual_snapshot_socket, &netHead, sizeof(NET_HEAD), 0);
				ret = send(g_manual_snapshot_socket, &devCmdReturn, sizeof(DEV_CMD_RETURN), 0);
				ret = send(g_manual_snapshot_socket, &date, sizeof(DATE_PARAM), 0);
				ret = send(g_manual_snapshot_socket, g_snapshot_buffer, g_snapshot_buffer_size, 0);

				g_manual_snapshot_socket = -1;
				g_snapshot_upload_mode = -1;

			}
			break;

		case SPECIAL_IP_AUTO_UPLOAD:
			{
				setSnapshotAutoUpload();
				
				if (g_auto_snapshot_socket <= 0)
				{
					return -1;
				}

				netHead.nCommand = NETCMD_DEV_DATA;
				netHead.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DATE_PARAM)+g_snapshot_buffer_size;
				
				devCmdReturn.nCmdID = DEV_JPEG_DATA;
				devCmdReturn.nCmdLen = sizeof(DATE_PARAM)+g_snapshot_buffer_size;
				devCmdReturn.nReserve = g_snapshot_channel;
					
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);
				/*
				memcpy(g_snapshot_buffer, &netHead, sizeof(NET_HEAD));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD), &devCmdReturn, sizeof(DEV_CMD_RETURN));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN), &date, sizeof(DATE_PARAM));
				memcpy(g_snapshot_buffer+sizeof(NET_HEAD)+sizeof(DEV_CMD_RETURN)+sizeof(DATE_PARAM), g_snapshot_buffer, g_snapshot_buffer_size);

				send(g_auto_snapshot_socket, g_snapshot_buffer, sizeof(NET_HEAD)+netHead.nBufSize, 0);
				*/

				ret = send(g_manual_snapshot_socket, &netHead, sizeof(NET_HEAD), 0);
				ret = send(g_manual_snapshot_socket, &devCmdReturn, sizeof(DEV_CMD_RETURN), 0);
				ret = send(g_manual_snapshot_socket, &date, sizeof(DATE_PARAM), 0);
				ret = send(g_manual_snapshot_socket, g_snapshot_buffer, g_snapshot_buffer_size, 0);
				
				g_snapshot_upload_mode = -1;

			}
			break;

		case FTP_UPLOAD:
			{
				char fileName[32];
				FTP_PARAM param;

				getFtpParam(&param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				sprintf(fileName, "%04d%02d%02d%02d%02d%02d.jpg",
					date.year, date.month, date.day, date.hour, date.minute, date.second);

				ret = FTP_PutFile_Ext(param.strIP, param.nPort, param.strName, param.strPsw, fileName, g_snapshot_buffer, g_snapshot_buffer_size, fileName);

				printf("FTP_UPLOAD: %d %s %d %s %s %s %d\n", ret, param.strIP, param.nPort, param.strName, param.strPsw, fileName, g_snapshot_buffer_size);
				
				g_snapshot_upload_mode = -1;
			}
			break;

		case TFTP_UPLOAD:
			{
				char fileName[32];
				TFTP_PARAM param;

				getTftpParam(&param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);

				sprintf(fileName, "%04d%02d%02d%02d%02d%02d.jpg", date.year, date.month, date.day, date.hour, date.minute, date.second);

				ret = TFTP_PutFile_Ext(param.strIP, param.nPort, fileName, g_snapshot_buffer, &g_snapshot_buffer_size);
				
				printf("TFTP_UPLOAD: %d %s %d %s %d\n", ret, param.strIP, param.nPort, fileName, g_snapshot_buffer_size);
				
				g_snapshot_upload_mode = -1;
			}
			break;

		//定时抓拍图片处理
		case SCHEDULE_EMAIL_UPLOAD:
			{
				char fileName[32];
				char sSend_content[200] = {0};
				OSD_PARAM  osd_param;
				EMAIL_PARAM param;
				PROBE_IN_ALARM_PARAM probe_in_alarm_param;

				getEmailParam(&param);
				getOsdParam(0, &osd_param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);
				getProbeInAlarmParam(0, &probe_in_alarm_param);
				sprintf(fileName, "%04d%02d%02d%02d%02d%02d.jpg", date.year, date.month, date.day, date.hour, date.minute, date.second);
				sprintf(sSend_content, "%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 抓拍到图片，请及时查看!");
				printf("SCHEDULE_EMAIL_UPLOAD:::param.strHeader = %s %s\n\n", param.strHeader, param.strFrom);
				ret = SMTP_Send_Ext(param.strIP, param.strName, param.strPsw,
							param.strFrom, param.strTo, param.strCc, param.strBcc, NULL,
							param.strHeader, sSend_content, fileName, g_snapshot_buffer, g_snapshot_buffer_size);
				g_snapshot_upload_mode = -1;
				printf("SCHEDULE_EMAIL_UPLOAD: %d %s %d\n", ret, fileName, g_snapshot_buffer_size);
					
			}
			break;
		case EMAIL_UPLOAD:
			{
				char fileName[32];
				char sSend_content[200] = {0};
				OSD_PARAM  osd_param;
				EMAIL_PARAM param;
				PROBE_IN_ALARM_PARAM probe_in_alarm_param;

				getEmailParam(&param);
				getOsdParam(0, &osd_param);
				getSystemTime(&date.year, &date.month, &date.day, &date.hour, &date.minute, &date.second);
				getProbeInAlarmParam(0, &probe_in_alarm_param);
				sprintf(fileName, "%04d%02d%02d%02d%02d%02d.jpg", date.year, date.month, date.day, date.hour, date.minute, date.second);
				flags = 0;
				#ifdef YIYUAN
					//亿源邮件
					if(g_snapshot_email_type == 1){
						if(osd_param.TitleOSD[0].bShow == 0){
								g_snapshot_email_type = 0;
								sprintf(sSend_content, "%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!侦测到你的监控区域\"通道1\"在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 发生视频移动报警, 请及时查看!");
							}
						else
							{
								sprintf(sSend_content, "%s%s%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!侦测到你的监控区域\"",osd_param.TitleOSD[0].sTitle, "\"在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 发生视频移动报警，请及时查看!");
							
							}
							flags = 1;
					}
					else if(g_snapshot_email_type == 2){
							if(strlen(probe_in_alarm_param.probeName) == 0){
								sprintf(sSend_content, "%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!侦测到你的监控区域\"通道1\"在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 发生红外探头报警，请及时查看!");
							}
							
							else
							{
								sprintf(sSend_content, "%s%s%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!侦测到你的监控区域\"",probe_in_alarm_param.probeName, "\"在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 发生红外探头报警，请及时查看!");
							}
							flags = 1;
					}
					else 
					{
						flags = 0;
						sprintf(sSend_content, "%s%d%s%d%s%d%s%d:%d:%d%s", "提醒!在",date.year, "年", date.month, "月", date.day, "日", date.hour, date.minute, date.second, ", 抓拍到图片，请及时查看!");
						
					}
					printf("EMAIL_UPLOAD:::param.strHeader = %s %s\n\n", param.strHeader, param.strFrom);
					ret = SMTP_Send_Ext(param.strIP, param.strName, param.strPsw,
								param.strFrom, param.strTo, param.strCc, param.strBcc, NULL,
								param.strHeader, sSend_content, fileName, g_snapshot_buffer, g_snapshot_buffer_size);
								

					#else
					ret = SMTP_Send_Ext(param.strIP, param.strName, param.strPsw,
								param.strFrom, param.strTo, param.strCc, param.strBcc, NULL,
								param.strHeader, "Snapshot", fileName, g_snapshot_buffer, g_snapshot_buffer_size);
					#endif

							
					printf("EMAIL_UPLOAD: %d %s %d\n", ret, fileName, g_snapshot_buffer_size);
					g_snapshot_upload_mode = -1;
					if(flags){
						#ifdef YIYUAN
						printf("yiyuan email sleep ...\n");
						sleep(40);
						#endif
					}	
				
			}
			break;

		case LOCAL_STORE:
			//printf("g_snapshot_upload_mode = %d\n", g_snapshot_upload_mode);
			videoJpegSnapShot(i, -1);
			g_snapshot_upload_mode = -1;
			break;

		default:
			//printf("g_snapshot_upload_mode = %d\n", g_snapshot_upload_mode);
			g_snapshot_upload_mode = -1;
			break;
		}
	
	}

	pthread_exit(NULL);

	return 0;
	}
	

int SNAPSHOT_Start()
{
	int ret = -1;
	pthread_t threadID;
	
	g_snapshot_run = 1;
	
	ret = pthread_create(&threadID, NULL, (void *)snapshotFun, NULL);
	if (ret < 0)
	{
		//printf("EMAIL_Start(): Failed!\n");

		g_snapshot_run = 0;
		return -1;
	}

	//printf("EMAIL_Start(): Ok!\n");

	return 0;
}

int SNAPSHOT_Stop()
{
	g_snapshot_run = 0;
	sem_post(&g_snapshot_sem);
	
	return 0;
}

int SNAPSHOT_Pause()
{
	g_snapshot_pause = 1;
	sem_post(&g_snapshot_sem);
	
	return 0;
}

int SNAPSHOT_Resume()
{
	g_snapshot_pause = 0;
	sem_post(&g_snapshot_sem);
	
	return 0;
}

int SNAPSHOT_Send(int nChannel, int nType, void *buffer, int size, int uploadType)
{
	int ret = -1;
	
	if (nChannel<0 || nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	if (nType<0 || nType>=MAX_CHANNEL_ENC_NUM)
	{
		return -1;	
	}
	if (buffer == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_snapshot_buffer_mutex);
	
	g_snapshot_upload_mode = uploadType;
	g_snapshot_channel = nChannel;
	g_snapshot_buffer_size = size;
	memcpy(g_snapshot_buffer, buffer, size);
	
	pthread_mutex_unlock(&g_snapshot_buffer_mutex);

	//printf("########Snapshot########: g_snapshot_upload_mode = %x\n", uploadType);
	
	sem_post(&g_snapshot_sem);
	
	return 0;
}

int SNAPSHOT_Manual(int socket)
{
	g_snapshot_upload_mode = SPECIAL_IP_MANUAL_UPLOAD;
	g_manual_snapshot_socket = socket;
	//g_snapshot_pause = 0;
	//sem_post(&g_snapshot_sem);
	
	return 0;
}

int SNAPSHOT_Automation()
{
	int ret = -1;
	unsigned long nID = 0;
	char addr[MAX_IP_LEN];
	
	getJpegIPParam(addr);
	
	inet_pton(AF_INET, addr, &nID);
	
	ret = NETSDK_GetUserId(nID);
	if (ret > 0)
	{
		g_snapshot_upload_mode = SPECIAL_IP_AUTO_UPLOAD;
		g_auto_snapshot_socket = ret;

		return 0;
	}
	else
	{
		g_auto_snapshot_socket = -1;
		
		return -1;
	}
}

int ALARM_Info_Upload(int nType, int nUploadType)
{
	int ret = -1;
	
	if (nType<=0 || nType>3)
	{
		return -1;
	}
	
	printf("ALARM_Info_Upload: %x %x\n", nType, nUploadType);

	pthread_mutex_lock(&g_snapshot_buffer_mutex);
	
	g_alarm_info_type = nType;
	g_alarm_info_upload_mode = nUploadType;
	
	pthread_mutex_unlock(&g_snapshot_buffer_mutex);
	
	sem_post(&g_snapshot_sem);
	
	return 0;
	
}


int manual_record_result(int nType)
{
	DVSNET_RECORD_RESULT *recordRecordInfo;
	DEV_CMD_HEADER *devCmdHeader;
	char buffer[1000];
	
	unsigned long cur_year = 0;
	unsigned long cur_month = 0;
	unsigned long cur_day = 0;
	unsigned long cur_week = 0;
	unsigned long cur_hour = 0;
	unsigned long cur_minute = 0;
	unsigned long cur_second = 0;
	devCmdHeader = (DEV_CMD_HEADER *)buffer;
	recordRecordInfo = (DVSNET_RECORD_RESULT *)(buffer+sizeof(DEV_CMD_HEADER));
	// 处理报警信息传送
	devCmdHeader->nCmdID = nType;
	devCmdHeader->nChannel = g_snapshot_channel;
	devCmdHeader->nCmdLen = sizeof(DVSNET_RECORD_RESULT);
	
	recordRecordInfo->nChannel = g_snapshot_channel;
	recordRecordInfo->nResult = 0;
	
	getSystemTimeExt(&cur_year, &cur_month, &cur_day, &cur_week, &cur_hour, &cur_minute, &cur_second);
	
	recordRecordInfo->Time.year = cur_year; //报警时间
	recordRecordInfo->Time.month = cur_month;
	recordRecordInfo->Time.day = cur_day;
	recordRecordInfo->Time.week = cur_week;
	recordRecordInfo->Time.hour = cur_hour;
	recordRecordInfo->Time.minute = cur_minute;
	recordRecordInfo->Time.second = cur_second; 	
				
	NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(DVSNET_RECORD_RESULT));
	printf("send DEV_SNAPSHOT_RESULT  success\n");
	
}

int set_Email_Alarm_status(int nType)
{
	g_snapshot_email_type = nType;
	return 0;
}

#if 1
int get_snapshot_buffer_size(char *snapshot_buffer, int *snapshot_size)
{
	if(snapshot_buffer == NULL)
	return -1;
	memcpy(snapshot_buffer, g_snapshot_buffer, g_snapshot_buffer_size);
	*snapshot_size = g_snapshot_buffer_size;
	//printf("##################################\n g_snapshot_buffer_size = %d snapshot_size  = %d\n", g_snapshot_buffer_size, *snapshot_size);
	return 0;
}
#endif


