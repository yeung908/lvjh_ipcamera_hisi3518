#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "alarmProc.h"
#include "util.h"
#include "vadcDrv.h"
#include "session.h"
#include "ptz.h"

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "videoEnc/videoEncModule.h"
#include "videoEnc/videoEnc.h"

#ifdef RECORD
#include "include/recordStruct.h"
#include "include/recordSDK.h"
#endif

// 全局变量
static int g_alarm_proc_run = 0;
static int g_motion_alarm_proc_run = 0;

static int g_alarm_proc_pause = 0;
static int g_probe_proc_alarm_pause = 0;



pthread_mutex_t g_video_lost_alarm_mutex;
pthread_mutex_t g_video_motion_alarm_mutex;
pthread_mutex_t g_probe_alarm_mutex;
pthread_mutex_t g_schedule_snapshot_mutex;

VIDEO_LOST_ALARM_PARAM g_video_lost_alarm_param[MAX_CHANNEL];
VIDEO_MOTION_ALARM_PARAM g_video_motion_alarm_param[MAX_CHANNEL];
PROBE_IN_ALARM_PARAM g_probe_alarm_param[MAX_PROBE_IN];
SCHEDULE_SNAPSHOT_PARAM g_schedule_snapshot_param[MAX_CHANNEL];

int g_video_lost_status[MAX_CHANNEL];
int g_video_motion_status[MAX_CHANNEL];
int g_probe_status[MAX_CHANNEL];

unsigned long cur_year = 0;
unsigned long cur_month = 0;
unsigned long cur_day = 0;
unsigned long cur_week = 0;
unsigned long cur_hour = 0;
unsigned long cur_minute = 0;
unsigned long cur_second = 0;

unsigned long cur_count =0;
unsigned long old_video_motion_count[MAX_CHANNEL];
unsigned long old_probe_count[MAX_CHANNEL];
unsigned long old_video_motion_count[MAX_CHANNEL];
unsigned long old_schedule_snapshot_count[MAX_CHANNEL];

int time_segment_compare(TIME_SEGMENT time_segment, int cur_time)
{
	int start_time = 0;
	int end_time = 0;

	#if 0
	printf("probeRecordParam.day.time_segment.start_hour = %d\n",time_segment.start_hour);
	printf("probeRecordParam.day.time_segment.end_hour = %d\n",time_segment.end_hour);
	printf("probeRecordParam.day.time_segment.start_minute = %d\n",time_segment.start_minute);
	printf("probeRecordParam.day.time_segment.end_minute = %d\n",time_segment.end_minute);
	#endif

	if (time_segment.start_hour>time_segment.end_hour || time_segment.end_hour>=24)
	{
		return -1;
	}
	if (time_segment.end_minute>=60)
	{
		return -1;
	}
	if (cur_time<0 || cur_time>=60*24)
	{
		return -1;
	}

	start_time = time_segment.start_hour*60+time_segment.start_minute;
	end_time = time_segment.end_hour*60+time_segment.end_minute;


#if 0
	printf("start_time = %d\n", start_time);
	printf("end_time = %d\n", end_time);
	printf("cur_time = %d\n", cur_time);
#endif

	if (cur_time >= start_time && cur_time <= end_time)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int videoLostAlarmProc()
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int flag = 0;
	int status = 0;
	int nLinkProbe = 0;
	int nLinkProbeTime = 0;
	int nSnapshotInterval = 0;
	int autoSnapshot = 0;

	// DVS报警信息返回
	char buffer[1000];
	ALARM_INFO *alarmInfo;
	DEV_CMD_HEADER *devCmdHeader;
	static int nNotice = 0;

	int cur_time = cur_hour*60 + cur_minute;

	devCmdHeader = (DEV_CMD_HEADER *)buffer;
	alarmInfo = (ALARM_INFO *)(buffer+sizeof(DEV_CMD_HEADER));

	for (i=0; i<get_channel_num(); i++)
	{
		flag = 0;
		nLinkProbeTime = 0;

		// 获取视频丢失状态
		status = getVideoStatus(i);
		g_video_lost_status[i] = status;

		/*
#ifdef CCD
		// 获取视频丢失状态
		status = getVideoStatus(i);
		g_video_lost_status[i] = status;
#endif

#ifdef VGA_CMOS
		status = 1;
		g_video_lost_status[i] = 1;
#endif

#ifdef HD_CMOS
		status = 1;
		g_video_lost_status[i] = 1;
#endif
		*/

		//printf("getVideoStatus: %d %d\n", status, nNotice);
		//if (status)
		if (status == 0)	// Add the code by Jerry, 2010-06-07
		{
			nNotice = 0;
			continue;
		}

		// 比较时间段
		pthread_mutex_lock(&g_video_lost_alarm_mutex);

		// 判断该时间段是否启用
		if (g_video_lost_alarm_param[i].day[cur_week].nOnFlag)
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_video_lost_alarm_param[i].day[cur_week].time_segment[j], cur_time);
				if (ret == 1)
				{
					//printf("Day: %\n", cur_week);
					flag = 1;
				}
			}
		}
		if (g_video_lost_alarm_param[i].day[0].nOnFlag) // 每天
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_video_lost_alarm_param[i].day[0].time_segment[j], cur_time);
				if (ret == 1)
				{
					//printf("Every Day\n", i);
					flag = 1;
				}
			}
		}
		nLinkProbe = g_video_lost_alarm_param[i].nLinkProbe;
		nLinkProbeTime = g_video_lost_alarm_param[i].nLinkProbeTime;
		autoSnapshot = g_video_lost_alarm_param[i].nLinkSnapshotUploadMode;
		nSnapshotInterval = g_video_lost_alarm_param[i].nLinkSnapshotInterval;

		pthread_mutex_unlock(&g_video_lost_alarm_mutex);

		// 视频丢失报警处理
		if (flag)
		{
			//printf("Video Lost Alarm!\n");

			// 处理报警信息传送
			devCmdHeader->nCmdID = DEV_ALARM_INFO;
			devCmdHeader->nChannel = i;
			devCmdHeader->nCmdLen = sizeof(ALARM_INFO);

			alarmInfo->nChannel = i;
			alarmInfo->nType = VIDEO_LOST_ALARM;
			alarmInfo->nStatus = 1;

			alarmInfo->Time.year = cur_year; //报警时间
			alarmInfo->Time.month = cur_month;
			alarmInfo->Time.day = cur_day;
			alarmInfo->Time.week = cur_week;
			alarmInfo->Time.hour = cur_hour;
			alarmInfo->Time.minute = cur_minute;
			alarmInfo->Time.second = cur_second;

			NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(ALARM_INFO));

			// 处理报警联锁
			for (j=0; j<MAX_PROBE_OUT; j++) // IO报警输出
			{
				//printf("Video Lost: %d\n", nLinkProbe);
				if (nLinkProbe&(0x1<<j))
				{
					setProbeOutStatus(i, nLinkProbeTime);
				}
			}

			// 报警信息上传
			if (nNotice == 0)
			{
				ALARM_Info_Upload(1, autoSnapshot);
				nNotice = 1;
			}

			// 录像报警联锁

		}
	}

	return 0;
}

int videoMotionAlarmProc()
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int flag = 0;
	int status = 0;
	int nLinkProbe = 0;
	int nLinkRecord = 0;
	int nLinkRecordTime = 0;
	int nLinkProbeTime = 0;
	int nLinkSnapshotUploadFlag = 0;
	int nSnapshotInterval = 0;
	int autoSnapshot = 0;
	int value = 0;
	RECORDSDK_CMD_PARAM cmdParam;

	// NVS报警信息返回
	char buffer[1000];
	ALARM_INFO *alarmInfo;
	DEV_CMD_HEADER *devCmdHeader;
	ALARM_STATUS_PARAM alarm_status_param;
	
	getAlarmStatusParam(&alarm_status_param);

	int cur_time = cur_hour*60 + cur_minute;

	devCmdHeader = (DEV_CMD_HEADER *)buffer;
	alarmInfo = (ALARM_INFO *)(buffer+sizeof(DEV_CMD_HEADER));
	//�жϵ�ǰ���ƶ����״̬λ

	for (i=0; i<get_channel_num(); i++)
	{
		flag = 0;
		nLinkProbeTime = 0;
		autoSnapshot = 0;

		//printf("alarm_status_param.nMotionStatus  = %d\n", alarm_status_param.nMotionStatus );
		if(alarm_status_param.nMotionStatus == 0)
		{
			cmdParam.nOpt = RSDKCMD_SET_VIDEO_STATUS;
			cmdParam.param.videoStatus = 0x01;
			RECORDSDK_Operate(&cmdParam, NULL, NULL);
			continue;
		}
		
		// 获取视频丢失状态
		status = getVideoInMotionStatus(i);
		g_video_motion_status[i] = status;
		if (!status)
		{
			cmdParam.nOpt = RSDKCMD_SET_VIDEO_STATUS;
			cmdParam.param.videoStatus = 0x01;
			RECORDSDK_Operate(&cmdParam, NULL, NULL);
			continue;
		}

	
		//printf("Starting Snapshot ::::::%d\n\n", status);

		// 比较时间段
		pthread_mutex_lock(&g_video_motion_alarm_mutex);

		// 判断该时间段是否启用
		//printf("cur_week = %d\n", cur_week);
		if (g_video_motion_alarm_param[i].day[cur_week].nOnFlag)
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_video_motion_alarm_param[i].day[cur_week].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}
		if (g_video_motion_alarm_param[i].day[0].nOnFlag) // 每天
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_video_motion_alarm_param[i].day[0].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}
		nLinkProbe = g_video_motion_alarm_param[i].nLinkProbe;
		nLinkProbeTime = g_video_motion_alarm_param[i].nLinkProbeTime;
		nLinkSnapshotUploadFlag = g_video_motion_alarm_param[i].nLinkSnapshotUploadFlag;
		autoSnapshot = g_video_motion_alarm_param[i].nLinkSnapshotUploadMode;
		nSnapshotInterval = g_video_motion_alarm_param[i].nLinkSnapshotInterval;
		nLinkRecord = g_video_motion_alarm_param[i].nLinkRecord;
		nLinkRecordTime = g_video_motion_alarm_param[i].nLinkRecordTime;


		pthread_mutex_unlock(&g_video_motion_alarm_mutex);

		// 视频丢失报警处理
		if (flag)
		{
			//printf("videoMotionAlarmProc flag = %d\n", videoMotionAlarmProc);
			ddnsYiYuanResume(2);
			if(nLinkRecord){
				cmdParam.nOpt = RSDKCMD_SET_VIDEO_STATUS;
				cmdParam.param.videoStatus = 0x02;
				RECORDSDK_Operate(&cmdParam, NULL, NULL);
				//printf(" send record status to record thread nLinkRecord = %d\n", nLinkRecord);
			}
			//printf("nLinkRecord = %d %d\n", nLinkRecord, cmdParam.param.videoStatus);
			
			//printf("Video Motion Alarm(%d)(%d)!\n", autoSnapshot, nLinkProbe);
			
		

			// 处理报警信息传送
			devCmdHeader->nCmdID = DEV_ALARM_INFO;
			devCmdHeader->nChannel = i;
			devCmdHeader->nCmdLen = sizeof(ALARM_INFO);

			alarmInfo->nChannel = i;
			alarmInfo->nType = VIDEO_MOTION_ALARM;
			alarmInfo->nStatus = 1;

			alarmInfo->Time.year = cur_year; //报警时间
			alarmInfo->Time.month = cur_month;
			alarmInfo->Time.day = cur_day;
			alarmInfo->Time.week = cur_week;
			alarmInfo->Time.hour = cur_hour;
			alarmInfo->Time.minute = cur_minute;
			alarmInfo->Time.second = cur_second;
			//ddnsYiYuanResume(2);
			ret = NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(ALARM_INFO));

			// ������������
			for (j=0; j<MAX_PROBE_OUT; j++) // IO�������
			{
				if (nLinkProbe&(0x1<<j))
				{
						//debugPrintf();
						setMotionProbeOutStatus(i, nLinkProbeTime);
				}
			}
			
			//if (cur_count-old_video_motion_count[i] >=  nSnapshotInterval)
			//{
				//printf("%s nLinkSnapshotUploadFlag = %d autoSnapshot = %d\n ", __func__, nLinkSnapshotUploadFlag, autoSnapshot);
				if(nLinkSnapshotUploadFlag){
					// 处理抓照
					switch (autoSnapshot)
					{
					case SPECIAL_IP_MANUAL_UPLOAD:
						videoJpegSnapShot(i, SPECIAL_IP_MANUAL_UPLOAD);
						old_video_motion_count[i] = cur_count;
						break;

					case SPECIAL_IP_AUTO_UPLOAD:
						videoJpegSnapShot(i, SPECIAL_IP_AUTO_UPLOAD);
						old_video_motion_count[i] = cur_count;
						break;

					case FTP_UPLOAD:
						videoJpegSnapShot(i, FTP_UPLOAD);
						old_video_motion_count[i] = cur_count;
						break;

					case TFTP_UPLOAD:
						videoJpegSnapShot(i, TFTP_UPLOAD);
						old_video_motion_count[i] = cur_count;
						break;

					case EMAIL_UPLOAD:
						//printf("EMAIL_UPLOAD motion\n");
						set_Email_Alarm_status(1);
						videoJpegSnapShot(i, EMAIL_UPLOAD);
						old_video_motion_count[i] = cur_count;
						break;

					// Add the code by lvjh, 2010-06-23
					default:
						videoJpegSnapShot(i, LOCAL_STORE);
						old_video_motion_count[i] = cur_count;
						break;
					}
				}
				else if(g_video_motion_alarm_param[i].nLinkSnapshot)
				{
					videoJpegSnapShot(i, LOCAL_STORE);
				}
			//}
		}
	}

	return 0;
}

int probeAlarmProc()
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int flag = 0;
	int status = 0;
	int nLinkProbe = 0;
	int nLinkRecord = 0;
	int nLinkProbeTime = 0;
	int nSnapshotInterval = 0;
	int autoSnapshot = 0;
	int	nLinkSnapshotUploadFlag = 0;
	int snapshotChn = 0;
	int nPresetChannel = 0;
	int nPresetPoint = 0;

	// DVS报警信息返回
	char buffer[1000];
	ALARM_INFO *alarmInfo;
	DEV_CMD_HEADER *devCmdHeader;

	int cur_time = cur_hour*60 + cur_minute;

	devCmdHeader = (DEV_CMD_HEADER *)buffer;
	alarmInfo = (ALARM_INFO *)(buffer+sizeof(DEV_CMD_HEADER));

	for (i=0; i<get_channel_num(); i++)
	{
		flag = 0;
		nLinkProbeTime = 0;
		autoSnapshot = 0;

		//printf("g_probe_proc_alarm_pause  = %d\n", g_probe_proc_alarm_pause);
		//�ж�̽ͷ�ĵ�ǰ״̬
		if(g_probe_proc_alarm_pause == 0){
			continue;
		}

		// 比较时间段
		pthread_mutex_lock(&g_probe_alarm_mutex);
		// 判断该时间段是否启用
		if (g_probe_alarm_param[i].day[cur_week].nOnFlag)
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_probe_alarm_param[i].day[cur_week].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}

		if (g_probe_alarm_param[i].day[0].nOnFlag) // 每天
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_probe_alarm_param[i].day[0].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}
		nLinkProbe = g_probe_alarm_param[i].nLinkProbe;
		nLinkRecord = g_probe_alarm_param[i].nLinkRecord;
		nLinkProbeTime = g_probe_alarm_param[i].nLinkProbeTime;
		nLinkSnapshotUploadFlag = g_probe_alarm_param[i].nLinkSnapshotUploadFlag;
		autoSnapshot = g_probe_alarm_param[i].nLinkSnapshotUploadMode;
		nSnapshotInterval = g_probe_alarm_param[i].nLinkSnapshotInterval;
		snapshotChn = g_probe_alarm_param[i].nLinkSnapshot;
		nPresetChannel = g_probe_alarm_param[i].bPresetNo[i*2];
		nPresetPoint = g_probe_alarm_param[i].bPresetNo[i*2+1];
		pthread_mutex_unlock(&g_probe_alarm_mutex);
		// 视频丢失报警处理
		if (flag)
		{
			// 处理报警信息传送


			devCmdHeader->nCmdID = DEV_ALARM_INFO;
			devCmdHeader->nChannel = i;
			devCmdHeader->nCmdLen = sizeof(ALARM_INFO);

			alarmInfo->nChannel = i;
			alarmInfo->nType = PROBE_IN_ALARM;
			alarmInfo->nStatus = 1;

			alarmInfo->Time.year = cur_year; //报警时间
			alarmInfo->Time.month = cur_month;
			alarmInfo->Time.day = cur_day;
			alarmInfo->Time.week = cur_week;
			alarmInfo->Time.hour = cur_hour;
			alarmInfo->Time.minute = cur_minute;
			alarmInfo->Time.second = cur_second;
			NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(ALARM_INFO));
		//	printf("Send Detector Alarm Information(%d)!\n", i);
			if(nLinkRecord == 1){
				set_detector_status(0, 1);
			}

			//printf("%d:: %d:: %d\n", cur_count, old_probe_count[i], nSnapshotInterval);
			//if (cur_count-old_probe_count[i] >=  nSnapshotInterval)
			//{
				// 抓照
				//printf("autoSnapshot= %d nLinkSnapshotUploadFlag = %d\n", autoSnapshot, nLinkSnapshotUploadFlag);
				if(nLinkSnapshotUploadFlag){
					switch (autoSnapshot)
					{
					case SPECIAL_IP_MANUAL_UPLOAD:
						videoJpegSnapShot(i, SPECIAL_IP_MANUAL_UPLOAD);
						old_probe_count[i] = cur_count;
						break;

					case SPECIAL_IP_AUTO_UPLOAD:
						videoJpegSnapShot(i, SPECIAL_IP_AUTO_UPLOAD);
						old_probe_count[i] = cur_count;
						break;

					case FTP_UPLOAD:
						videoJpegSnapShot(i, FTP_UPLOAD);
						old_probe_count[i] = cur_count;
						break;

					case TFTP_UPLOAD:
						videoJpegSnapShot(i, TFTP_UPLOAD);
						old_probe_count[i] = cur_count;
						break;

					case EMAIL_UPLOAD:
						printf("probe Email_upload\n");
						set_Email_Alarm_status(2);
						videoJpegSnapShot(i, EMAIL_UPLOAD);
						old_probe_count[i] = cur_count;
						break;

					// Add the code by lvjh, 2010-06-23
					default:
						videoJpegSnapShot(i, LOCAL_STORE);
						old_probe_count[i] = cur_count;
						break;
					}
				}
				else if(g_probe_alarm_param[i].nLinkSnapshot)
				{
					videoJpegSnapShot(i, LOCAL_STORE);
				}
			}
				// 处理报警联锁
			//printf("func(%s)::nLinkProbe = %d\n", __func__,nLinkProbe);
			
			for (j=0; j<MAX_PROBE_OUT; j++) // IO�������
			{
				if (nLinkProbe&(0x1<<j))
				{
					setMotionProbeOutStatus(0, nLinkProbeTime);
				}
			}
			probeNetAlarmStop();
		//}
	}

	return 0;
}

int scheduleSnapshotProc()
{
	int i = 0;
	int j = 0;
	int ret = 0;
	int flag = 0;
	int nSnapshotInterval = 0;
	int autoSnapshot = 0;

	int cur_time = cur_hour*60 + cur_minute;

	for (i=0; i<get_channel_num(); i++)
	{
		flag = 0;

		// 比较时间段
		pthread_mutex_lock(&g_schedule_snapshot_mutex);

		// 判断该时间段是否启用
		if (g_schedule_snapshot_param[i].day[cur_week].nOnFlag == 1)
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_schedule_snapshot_param[i].day[cur_week].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}
		if (g_schedule_snapshot_param[i].day[0].nOnFlag == 1) // 每天
		{
			for (j=0; j<MAX_TIME_SEGMENT; j++)
			{
				ret = time_segment_compare(g_schedule_snapshot_param[i].day[0].time_segment[j], cur_time);
				if (ret == 1)
				{
					flag = 1;
				}
			}
		}

		autoSnapshot = g_schedule_snapshot_param[i].nSnapshotUploadMode;
		nSnapshotInterval = g_schedule_snapshot_param[i].nSnapshotInterval;

		pthread_mutex_unlock(&g_schedule_snapshot_mutex);

		// 视频丢失报警处理
		if (flag)
		{
			if (cur_count-old_schedule_snapshot_count[i] >=  nSnapshotInterval)
			{
				// 抓照
				switch (autoSnapshot)
				{
				case SPECIAL_IP_MANUAL_UPLOAD:
					videoJpegSnapShot(i, SPECIAL_IP_MANUAL_UPLOAD);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				case SPECIAL_IP_AUTO_UPLOAD:
					videoJpegSnapShot(i, SPECIAL_IP_AUTO_UPLOAD);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				case FTP_UPLOAD:
					videoJpegSnapShot(i, FTP_UPLOAD);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				case TFTP_UPLOAD:
					videoJpegSnapShot(i, TFTP_UPLOAD);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				case EMAIL_UPLOAD:
					videoJpegSnapShot(i, SCHEDULE_EMAIL_UPLOAD);
					//videoJpegSnapShot(i, EMAIL_UPLOAD);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				// Add the code by lvjh, 2010-06-02
				case LOCAL_STORE:
					videoJpegSnapShot(i, LOCAL_STORE);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				default:
					videoJpegSnapShot(i, LOCAL_STORE);
					old_schedule_snapshot_count[i] = cur_count;
					break;

				}
			}

			// 录像报警联锁

		}
	}

	return 0;
}

int talkRequestProc()
{
	int ret = -1;
	int status = 0;

	char talkClientIP[16];
	unsigned long talkRequestIP = 0;

	char buffer[1000];
	TALK_REQUEST_INFO *talkQuestInfo;
	DEV_CMD_HEADER *devCmdHeader;
	NET_PARAM netParam;

	devCmdHeader = (DEV_CMD_HEADER *)buffer;
	talkQuestInfo = (TALK_REQUEST_INFO *)(buffer+sizeof(DEV_CMD_HEADER));

	status = getTalkRequestStatus();
	if (status)
	{
		getRemoteTalkIPParam(talkClientIP);
		ret = inet_pton(AF_INET, talkClientIP, &talkRequestIP);
		if (ret == 1)
		{
			getNetParam(&netParam);

			devCmdHeader->nCmdID = DEV_TALK_REQUEST;
			devCmdHeader->nChannel = 0;
			devCmdHeader->nCmdLen = sizeof(TALK_REQUEST_INFO);

			memcpy(talkQuestInfo->strDevIP, netParam.byServerIp, MAX_IP_LEN);

			NETSDK_SendSpecMsg(talkRequestIP, buffer, sizeof(DEV_CMD_HEADER)+sizeof(TALK_REQUEST_INFO));
		}
	}

	return 0;
}

int MotionAlarmProcFun()
{
	while (g_motion_alarm_proc_run)
	{
		getSystemTimeExt(&cur_year, &cur_month, &cur_day, &cur_week, &cur_hour, &cur_minute, &cur_second);
		cur_count = cur_hour*3600+cur_minute*60+cur_second;
		videoMotionAlarmProc();
		usleep(1000);
	}

	pthread_exit(NULL);

	return 0;
}


int alarmProcFun()
{
#ifdef RECORD
	int i = 0;
	unsigned long value =0;
	unsigned long status =0;
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	sleep(10);
	while (g_alarm_proc_run)
	{
		if (g_alarm_proc_pause)
		{
			sleep(1);
			continue;
		}

		// 获取系统时间
		getSystemTimeExt(&cur_year, &cur_month, &cur_day, &cur_week, &cur_hour, &cur_minute, &cur_second);
		cur_count = cur_hour*3600+cur_minute*60+cur_second;
		//videoLostAlarmProc();
		videoMotionAlarmProc();
		probeAlarmProc();
		talkRequestProc();
		scheduleSnapshotProc();

		usleep(10);
	}

	pthread_exit(NULL);

	return 0;
}


int alarmProcModuleInit()
{
	int i = 0;

	g_alarm_proc_run = 0;
	g_alarm_proc_pause = 0;

	pthread_mutex_init(&g_video_lost_alarm_mutex, NULL);
	pthread_mutex_init(&g_video_motion_alarm_mutex, NULL);
	pthread_mutex_init(&g_probe_alarm_mutex, NULL);
	pthread_mutex_init(&g_schedule_snapshot_mutex, NULL);

	for (i=0; i<MAX_CHANNEL; i++)
	{
		getVideoLostAlarmParam(i, &g_video_lost_alarm_param[i]);
		getVideoMotionAlarmParam(i, &g_video_motion_alarm_param[i]);
		getProbeInAlarmParam(i, &g_probe_alarm_param[i]);
		getScheduleSnapshotParam(i, &g_schedule_snapshot_param[i]);
	}

	return 0;
}

int alarmProcModuleDeInit()
{
	int i = 0;

	g_alarm_proc_run = 0;
	g_alarm_proc_pause = 0;

	pthread_mutex_destroy(&g_video_lost_alarm_mutex);
	pthread_mutex_destroy(&g_video_motion_alarm_mutex);
	pthread_mutex_destroy(&g_probe_alarm_mutex);
	pthread_mutex_destroy(&g_schedule_snapshot_mutex);

	for (i=0; i<MAX_CHANNEL; i++)
	{
		memset(&g_video_lost_alarm_param[i], 0, sizeof(VIDEO_LOST_ALARM_PARAM));
		memset(&g_video_motion_alarm_param[i], 0, sizeof(VIDEO_MOTION_ALARM_PARAM));
		memset(&g_probe_alarm_param[i], 0, sizeof(PROBE_IN_ALARM_PARAM));
		memset(&g_schedule_snapshot_param[i], 0, sizeof(SCHEDULE_SNAPSHOT_PARAM));
	}

	return 0;
}

int alarmProcStart()
{
	int ret = -1;
	pthread_t threadID;
	pthread_t motion_alarm_threadID;
	

	g_alarm_proc_run = 1;
	g_motion_alarm_proc_run  = 1;
	g_probe_proc_alarm_pause = 0;

	ret = pthread_create(&threadID, NULL, (void *)alarmProcFun, NULL);
	if (ret)
	{
		g_alarm_proc_run = 0;
		return -1;
	}
	
#if 0
	ret = pthread_create(&motion_alarm_threadID, NULL, (void *)MotionAlarmProcFun, NULL);
	if (ret)
	{
		g_motion_alarm_proc_run  = 0;
		return -1;
	}
	
#endif

	return 0;
}

int alarmProcStop()
{
	g_alarm_proc_run = 0;

	return 0;
}

int alarmProcPause()
{
	g_alarm_proc_pause = 1;

	return 0;
}

int alarmProcResume()
{
	g_alarm_proc_pause = 0;

	return 0;
}


int probeNetAlarmStart()
{

	g_probe_proc_alarm_pause = 1;
	return 0;
}

int probeNetAlarmStop()
{

	g_probe_proc_alarm_pause = 0;
	return 0;
}


int videoLostAlarmSetup(int nChannel, VIDEO_LOST_ALARM_PARAM param)
{
	int i = 0;

	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	pthread_mutex_lock(&g_video_lost_alarm_mutex);

	memcpy(&g_video_lost_alarm_param[nChannel], &param, sizeof(VIDEO_LOST_ALARM_PARAM));

	pthread_mutex_unlock(&g_video_lost_alarm_mutex);

	/*
	for (i=0; i<8; i++)
	{
		printf("Time Segment: %d \n", g_video_lost_alarm_param[nChannel].day[i].nOnFlag);
	}
	*/

	return 0;
}

int videoMotionAlarmSetup(int nChannel, VIDEO_MOTION_ALARM_PARAM param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	pthread_mutex_lock(&g_video_motion_alarm_mutex);

	memcpy(&g_video_motion_alarm_param[nChannel], &param, sizeof(VIDEO_MOTION_ALARM_PARAM));

	pthread_mutex_unlock(&g_video_motion_alarm_mutex);

	return 0;
}

int probeAlarmSetup(int nChannel, PROBE_IN_ALARM_PARAM param)
{
	if (nChannel<0 || nChannel>=MAX_PROBE_IN)
	{
		return -1;
	}

	pthread_mutex_lock(&g_probe_alarm_mutex);

	memcpy(&g_probe_alarm_param[nChannel], &param, sizeof(PROBE_IN_ALARM_PARAM));

	pthread_mutex_unlock(&g_probe_alarm_mutex);

	return 0;
}

int scheduleSnapshotSetup(int nChannel, SCHEDULE_SNAPSHOT_PARAM param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}

	pthread_mutex_lock(&g_schedule_snapshot_mutex);

	memcpy(&g_schedule_snapshot_param[nChannel], &param, sizeof(SCHEDULE_SNAPSHOT_PARAM));

	pthread_mutex_unlock(&g_schedule_snapshot_mutex);

	return 0;
}






