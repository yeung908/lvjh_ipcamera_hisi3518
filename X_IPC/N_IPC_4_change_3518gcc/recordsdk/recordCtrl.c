#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "recordFile.h"
#include "recordCtrl.h"

#define DEV_RECORD_RESULT 0x10000+498


// 录像参数
static RECORD_SETUP g_record_setup_param;
static int g_record_ctrl_run_flag = 0;
static int g_record_ctrl_pause_flag = 0;

// 录像的标志
static int g_timer_record_flag[MAX_AV_CHANNEL];
static int g_timer_record_time[MAX_AV_CHANNEL];

static int g_manual_record_flag[MAX_AV_CHANNEL];
static int g_manual_record_time[MAX_AV_CHANNEL];

static int g_video_motion_record_flag[MAX_AV_CHANNEL];
static int g_video_motion_record_time[MAX_AV_CHANNEL];
static int g_video_status[MAX_AV_CHANNEL];

static int g_detector_record_flag[MAX_AV_CHANNEL];
static int g_detector_record_time[MAX_AV_CHANNEL];
static int g_detector_status[MAX_AV_CHANNEL];

// 获取当前时间，单位为秒
static int get_cur_tick_count()
{
	int seconds = time( (time_t *)NULL );

	if (seconds > 0 )
	{
		return seconds;
	}
	else
	{
		return 0;
	}
}

static int time_segment_compare(TIME_SEGMENT time_segment, int cur_time)
{
	int start_time = 0;
	int end_time = 0;

	#if 0	
	printf("probeRecordParam.day.time_segment.start_hour = %d\n",time_segment[0].start_hour);
	printf("probeRecordParam.day.time_segment.end_hour = %d\n",time_segment[0].end_hour);
	printf("probeRecordParam.day.time_segment.start_minute = %d\n",time_segment[0].start_minute);
	printf("probeRecordParam.day.time_segment.end_minute = %d\n",time_segment[0].end_minute);
	#endif	

	if (time_segment.start_hour>time_segment.end_hour || time_segment.end_hour>=24)
	{
		return -1;
	}
	if (time_segment.start_minute>time_segment.end_minute || time_segment.end_minute>=60)
	{
		return -1;
	}
	if (cur_time<0 || cur_time>=60*24)
	{
		return -1;
	}

	start_time = time_segment.start_hour*60+time_segment.start_minute;
	end_time = time_segment.end_hour*60+time_segment.end_minute;

	printf("start_time = %d", start_time);
	printf("end_time = %d", end_time);
	printf("cur_time = %d", cur_time);
	
	if (cur_time >= start_time  && cur_time <= end_time)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int time_segment_compare_ext(TIME_SEGMENTS time_segments, int cur_time)
{
	int ret = 0;
	int i = 0;

	if (time_segments.nOnFlag)
	{
		for (i=0; i<MAX_TIME_SEGMENT; i++)
		{
			ret = time_segment_compare(time_segments.time_segment[i], cur_time);
			if (ret == 1)
			{
				return 1;
			}
		}
	}

	return 0;
}

// 设置视频移动报警录像参数
int set_videomotion_record_param(int channel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	memcpy(&g_record_setup_param.videomotion_record_param[channel], param, sizeof(VIDEOMOTION_RECORD_CHANNEL_PARAM));

	return 0;
}

// 获取视频移动报警录像参数
int get_videomotion_record_param(int channel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>get_av_channel_num())
	{
		return -1;
	}	
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(param, &g_record_setup_param.videomotion_record_param[channel], sizeof(VIDEOMOTION_RECORD_CHANNEL_PARAM));
	
	return 0;	
}

// 设置定时录像参数
int set_timer_record_param(int channel, TIMER_RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(&g_record_setup_param.timer_record_param[channel], param, sizeof(TIMER_RECORD_CHANNEL_PARAM));

	return 0;	
}

// 获取定时录像参数
int get_timer_record_param(int channel, TIMER_RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}	
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(param, &g_record_setup_param.timer_record_param[channel], sizeof(TIMER_RECORD_CHANNEL_PARAM));
	
	return 0;	
}

// 设置探头报警录像参数
int set_detector_record_param(int channel, DETECTOR_RECORD_CHANNEL_PARAM *param)
{
	printf("set_detector_record_param\n");
	if (channel<0 || channel>=12)
	{
		printf("set_detector_record_param 2\n");
		return -1;
	}	
	if (param == NULL)
	{
		printf("set_detector_record_param 3\n");
		return -1;	
	}
	
	memcpy(&g_record_setup_param.detector_record_param[channel], param, sizeof(DETECTOR_RECORD_CHANNEL_PARAM));
	
	return 0;	
}

// 获取探头报警录像参数
int get_detector_record_param(int channel, DETECTOR_RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>get_av_channel_num())
	{
		return -1;
	}	
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(param, &g_record_setup_param.detector_record_param[channel], sizeof(DETECTOR_RECORD_CHANNEL_PARAM));
	
	return 0;	
}

// 设置录像参数
int set_record_param(int channel, RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}	
	if (param == NULL)
	{
		return -1;
	}
	
	memcpy(&g_record_setup_param.record_param[channel], param, sizeof(RECORD_CHANNEL_PARAM));

	// 
	set_record_video_param(channel, param->avFormat.nVideoEncType, param->avFormat.nImageWidth, param->avFormat.nImageHeight, param->avFormat.nVideoBitRate, param->avFormat.nFrameRate);
	set_record_audio_param(channel, param->avFormat.nAudioEncType, param->avFormat.nAudioChannels, param->avFormat.nAudioSamples, param->avFormat.nAudioBitRate);
	
	return 0;	
}

// 获取录像参数
int get_record_param(int channel, RECORD_CHANNEL_PARAM *param)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}	
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(param, &g_record_setup_param.record_param[channel], sizeof(RECORD_CHANNEL_PARAM));
	
	return 0;	
}

// 设置录像器所有参数
int set_all_record_param(RECORD_SETUP *param)
{
	int i = 0;
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(&g_record_setup_param, param, sizeof(RECORD_SETUP));

	// Add the code by lvjh, 2008-03-13
	for (i=0; i<MAX_AV_CHANNEL; i++)
	{
		set_record_param(i, &param->record_param[i]);
	}

	/*
	printf("RECORD_SETUP: %d\n", sizeof(RECORD_SETUP));
	printf("Timer Record: %d %d %d %d %d %d %d %d %d\n", 
			g_record_setup_param.timer_record_param[0].day[0].nOnFlag,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[0].start_hour,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[0].start_minute,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[0].end_hour,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[0].end_minute,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[1].start_hour,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[1].start_minute,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[1].end_hour,
			g_record_setup_param.timer_record_param[0].day[0].time_segment[1].end_minute);
	*/
	return 0;	
}

// 获取录像器所有参数
int get_all_record_param(RECORD_SETUP *param)
{
	if (param == NULL)
	{
		return -1;	
	}
	
	memcpy(param, &g_record_setup_param, sizeof(RECORD_SETUP));
	
	return 0;	
}

int record_remount_fun()
{
	while(1){
			sleep(10);
			system("mount -o remount,rw /record/hd00/00/");
	}
}


// 处理各种录像线程,包括手动录像、视频移动报警录像、探头报警录像和定时录像
int record_control_fun()
{
	int ret = -1;
	int last_time = 0;	
	int i = 0;
	int j = 0;

	int videoStatus = 0;
	int videoLostFlag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	int week = 0;
	int hour = 0;
	int second = 0;
	int minute = 0;
	int start_hour = 0;
	int end_hour = 0;
	int start_minute = 0;
	int end_minute = 0;
	
	int time_cur = 0;
	int time_set_start = 0;
	int time_set_end = 0;
	
	int old_manual_record_flag[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int old_manual_record_time[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	int valid_record_flag = 0;
	int set_day = 0;
	int recoding_flag = 0;
	int probeIn_recording_flag = 0;
	

	int pause_flag = 0;
	int num = 0;

	int nOnFlag = 0;
	int nOnWeekFlag = 0;
	
	while (g_record_ctrl_run_flag)
	{
		// 暂停录像控制
		if (g_record_ctrl_pause_flag)
		{
			sleep(1);
			continue;
		}
	
		// 获取当前时间
		//week = get_week();
		week = get_week()+1;
		hour = get_hour();
		minute = get_minute();
		second = get_second();
		time_cur = hour * 60 + minute;

		//printf("record_control_fun: %d %d %d %d\n", week, hour, minute, g_video_status[0]);
            //    printf("motion alarm staring \n");
		
		// 视频移动
		for (i=0; i<get_av_channel_num(); i++)
		{
			valid_record_flag = 0;
			
			videoStatus = g_video_status[i] & 0x02;

			//printf("videoStatus = %d\n", videoStatus );
			// 报警发生
			if (videoStatus)
			{
				//printf("starting motion record \n");
				// 判断当天是否启用报警录像
				if (g_record_setup_param.videomotion_record_param[i].day[week].nOnFlag)
				{
					for (j=0; j<MAX_TIME_SEGMENT; j++)
					{
						start_hour = g_record_setup_param.videomotion_record_param[i].day[week].time_segment[j].start_hour;
						start_minute = g_record_setup_param.videomotion_record_param[i].day[week].time_segment[j].start_minute;
						end_hour = g_record_setup_param.videomotion_record_param[i].day[week].time_segment[j].end_hour;
						end_minute = g_record_setup_param.videomotion_record_param[i].day[week].time_segment[j].end_minute;

						time_set_start = start_hour * 60 + start_minute;
						time_set_end = end_hour * 60 + end_minute;

						// 当前时间在报警时间段内
						if ( time_cur >= time_set_start  && time_cur <= time_set_end && (time_set_start != 0 || time_set_end != 0) )
						//if ( time_cur >= time_set_start  && time_cur <= time_set_end && time_set_start <= time_set_end)	// Change the code by lvjh, 2009-05-08
						{
							valid_record_flag = 1;
							break;
						}
					}
				}

				// 判断每天是否启用报警录像
				if (g_record_setup_param.videomotion_record_param[i].day[0].nOnFlag)
				{
					for (j=0; j<MAX_TIME_SEGMENT; j++)
					{
						start_hour = g_record_setup_param.videomotion_record_param[i].day[0].time_segment[j].start_hour;
						start_minute = g_record_setup_param.videomotion_record_param[i].day[0].time_segment[j].start_minute;
						end_hour = g_record_setup_param.videomotion_record_param[i].day[0].time_segment[j].end_hour;
						end_minute = g_record_setup_param.videomotion_record_param[i].day[0].time_segment[j].end_minute;

						time_set_start = start_hour * 60 + start_minute;
						time_set_end = end_hour * 60 + end_minute;
						
						//printf("videomotion: %d %d %d\n", time_set_start, time_set_end, time_cur);

						// 当前时间在报警时间段内
						//if ( time_cur >= time_set_start  && time_cur <= time_set_end && (time_set_start != 0 || time_set_end != 0) )
						if ( time_cur >= time_set_start  && time_cur <= time_set_end && time_set_start <= time_set_end) // Change the code by lvjh, 2009-05-08
						{
							//printf("valid_record_flag = 1\n");
							valid_record_flag = 1;
							break;
						}
					}
				}
			}	

			// 判断是否要报警录像
			if (valid_record_flag)
			{
				//printf("valid_record_flag = %d\n", valid_record_flag);
				// 联锁其它通道录像
				for (j=0; j<get_av_channel_num(); j++)
				{
					// 获取当前联动通道的状态
					if (j = i)
					{
						nOnFlag = 1;
					}
					else
					{
						nOnFlag = (g_record_setup_param.videomotion_record_param[i].day[week].nChnBits >> j) & 0x01;
					}
					nOnWeekFlag = (g_record_setup_param.videomotion_record_param[i].day[0].nChnBits >> j) & 0x01;
				
					// 只要视频不丢失，则就要录像
					if (!videoLostFlag[j])
					{
						// 按位进行联动录像 					
						if (nOnFlag)
						{
							//printf("time: %d %d\n", g_video_motion_record_time[j], g_record_setup_param.videomotion_record_param[i].day[week].nTime*60);

							if (g_video_motion_record_time[j] < g_record_setup_param.videomotion_record_param[i].day[week].nTime*60)
							{
								g_video_motion_record_time[j] = g_record_setup_param.videomotion_record_param[i].day[week].nTime*60;
							}
						}
						else if (nOnWeekFlag)
						{
							if (g_video_motion_record_time[j] < g_record_setup_param.videomotion_record_param[i].day[0].nTime*60)
							{
								g_video_motion_record_time[j] = g_record_setup_param.videomotion_record_param[i].day[0].nTime*60;
							}
						}
					}
					//printf("video_motion: %d %d %d\n", g_video_motion_record_time[j], nOnFlag, nOnWeekFlag);
					// 
					if (g_video_motion_record_time[j] > 0 && (nOnFlag || nOnWeekFlag))
					{
						g_video_motion_record_flag[j] = 1;
						
						//printf("start_record: %d %d RECORD_TYPE_VIDEO_MOVE\n", j, g_video_motion_record_time[j]);
						//printf("RSDK: Stard video motion record(%d), time: %d\n", j, g_video_motion_record_time[j]);
#if 1
						if(recoding_flag == 0){
							//printf("RSDK: Stard video motion record(%d), time: %d\n", j, g_video_motion_record_time[j]);
							start_record(j, RECORD_TYPE_VIDEO_MOVE);
							recoding_flag  = 1;
						}
						//else
							//printf("continue recording , please waitting ...... .....\n");
#endif
						//start_record(j, RECORD_TYPE_VIDEO_MOVE);
					}
				}
			}
		}

		// 如果录像时间到了，则停止录像
		for (j=0; j<get_av_channel_num(); j++)
		{
			if (!videoLostFlag[j] && g_video_motion_record_flag[j]==1)
			{
				g_video_motion_record_time[j] -= (get_cur_tick_count() - last_time);

				if (g_video_motion_record_time[j] <= 0)
				{
					//printf("stop_record: %d RECORD_TYPE_VIDEO_MOVE\n", j);
					printf("RSDK: Stop video motion record(%d), time: %d\n", j, g_video_motion_record_time[j]);
					
					stop_record(j, RECORD_TYPE_VIDEO_MOVE);
					
					g_video_motion_record_flag[j] = 0;
					g_video_motion_record_time[j] = 0;	// Add the code by lvjh, 2009-05-08
					recoding_flag = 0;
				}
			}
		}

		// 探头录像
		for (i=0; i<get_av_channel_num(); i++)
		{
			valid_record_flag = 0;
			
			if (g_detector_status[i])
			{
				// 判断当天是否启用报警录像
				if (g_record_setup_param.detector_record_param[i].day[week].nOnFlag)
				{
					for (j=0; j<MAX_TIME_SEGMENT; j++)
					{
						start_hour = g_record_setup_param.detector_record_param[i].day[week].time_segment[j].start_hour;
						start_minute = g_record_setup_param.detector_record_param[i].day[week].time_segment[j].start_minute;
						end_hour = g_record_setup_param.detector_record_param[i].day[week].time_segment[j].end_hour;
						end_minute = g_record_setup_param.detector_record_param[i].day[week].time_segment[j].end_minute;

						time_set_start = start_hour * 60 + start_minute;
						time_set_end = end_hour * 60 + end_minute;

						// 当前时间在报警时间段内
						if ( time_cur >= time_set_start  && time_cur <= time_set_end && (time_set_start != 0 || time_set_end != 0) )
						//if ( time_cur >= time_set_start  && time_cur <= time_set_end && time_set_start <= time_set_end)	// Change the code by lvjh, 2009-05-08
						{
							valid_record_flag = 1;
							break;
						}
					}
				}
				
				// 判断每天是否启用报警录像
				if (g_record_setup_param.detector_record_param[i].day[0].nOnFlag)
				{
					for (j=0; j<MAX_TIME_SEGMENT; j++)
					{
						start_hour = g_record_setup_param.detector_record_param[i].day[0].time_segment[j].start_hour;
						start_minute = g_record_setup_param.detector_record_param[i].day[0].time_segment[j].start_minute;
						end_hour = g_record_setup_param.detector_record_param[i].day[0].time_segment[j].end_hour;
						end_minute = g_record_setup_param.detector_record_param[i].day[0].time_segment[j].end_minute;

						time_set_start = start_hour * 60 + start_minute;
						time_set_end = end_hour * 60 + end_minute;

						// 当前时间在报警时间段内
						if ( time_cur >= time_set_start  && time_cur <= time_set_end && (time_set_start != 0 || time_set_end != 0) )
						//if ( time_cur >= time_set_start  && time_cur <= time_set_end && time_set_start <= time_set_end)	// Change the code by lvjh, 2009-05-08
						{
							valid_record_flag = 1;
							break;
						}
					}
				}
			}

			// 判断是否要报警录像
			if (valid_record_flag)
			{
				// 联锁其它通道录像
				for (j=0; j<get_av_channel_num(); j++)
				{
					// 获取当前联动通道的状态
					if (j = i)
					{
						nOnFlag = 1;
					}
					else
					{
						nOnFlag = (g_record_setup_param.detector_record_param[i].day[week].nChnBits >> j) & 0x01;
					}
					nOnWeekFlag = (g_record_setup_param.detector_record_param[i].day[0].nChnBits >> j) & 0x01;

					// 只要视频不丢失，则就要录像
					if (!videoLostFlag[j])
					{
						if (nOnFlag)
						{
							if (g_detector_record_time[j] < g_record_setup_param.detector_record_param[i].day[week].nTime*60)
							{
								g_detector_record_time[j] = g_record_setup_param.detector_record_param[i].day[week].nTime*60;
							}
						}
						else if (nOnWeekFlag)
						{
							if (g_detector_record_time[j] < g_record_setup_param.detector_record_param[i].day[0].nTime*60)
							{
								g_detector_record_time[j] = g_record_setup_param.detector_record_param[i].day[0].nTime*60;
							}
						}
					}

					if (g_detector_record_time[j] > 0 && (nOnFlag || nOnWeekFlag))
					{
						g_detector_record_flag[j] = 1;
						
						//printf("start_record: %d %d RECORD_TYPE_ALARM_INPUT\n", j, g_detector_record_time[j]);
						printf("RSDK: Stard probe record(%d), time: %d\n", j, g_detector_record_time[j]);
					
						if(probeIn_recording_flag == 0){
							start_record(j, RECORD_TYPE_ALARM_INPUT);
							probeIn_recording_flag  = 1;
						}

						
						// 探头触发
						g_detector_status[i] = 0;
					}
				}
			}
		}

		// 如果录像时间到了，则停止录像
		for (j=0; j<get_av_channel_num(); j++)
		{
			if (!videoLostFlag[j] &&g_detector_record_flag[j]==1)
			{
				g_detector_record_time[j] -= (get_cur_tick_count() - last_time);

				if (g_detector_record_time[j] <= 0)
				{
					//printf("stop_record: %d RECORD_TYPE_ALARM_INPUT\n", j);
					printf("RSDK: Stard probe record(%d), time: %d\n", j, g_detector_record_time[j]);
					
					stop_record(j, RECORD_TYPE_ALARM_INPUT);
					
					g_detector_record_flag[j] = 0;
					g_detector_record_time[j] = 0;
					probeIn_recording_flag = 0;
				}
			}
		}

		// 定时录像 	
		for (i=0; i<get_av_channel_num(); i++)
		{
			//printf("[JERRY]: time record ...\n");
			
			if (!videoLostFlag[i])
			{
				//printf("g_timer_record_flag: %d\n", g_timer_record_flag[i]);

				// 当前通道没有开启定时录像
				if (g_timer_record_flag[i] == 0)
				{
					//printf("cur_timer_record_flag: %d\n", g_record_setup_param.timer_record_param[i].day[week].nOnFlag);
					if (g_record_setup_param.timer_record_param[i].day[week].nOnFlag)
					{
						for (j=0; j<MAX_TIME_SEGMENT; j++)
						{
							start_hour	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].start_hour;
							start_minute = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].start_minute;
							end_hour	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].end_hour;
							end_minute	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].end_minute;

							time_set_start = start_hour * 60 + start_minute;
							time_set_end  = end_hour * 60 + end_minute;

							// 当前时间在定时录像设定的时间段内
							if ( time_cur>=time_set_start && time_cur<=time_set_end && (time_set_start!=0 || time_set_end!=0) && time_set_start<=time_set_end)
							{
								g_timer_record_flag[i] = 1;
								//printf("time record: %d [%d-%d]!\n", time_cur, time_set_start, time_set_end);
								printf("RSDK: Stard time record(%d), time: %d [%d-%d]\n", i, time_cur, time_set_start, time_set_end);
								
								start_record(i, RECORD_TYPE_TIME);
							}
						}
					}
					//printf("all_timer_record_flag: %d\n", g_record_setup_param.timer_record_param[i].day[0].nOnFlag);
					if (g_record_setup_param.timer_record_param[i].day[0].nOnFlag)
					{
						for (j=0; j<MAX_TIME_SEGMENT; j++)
						{
							start_hour	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].start_hour;
							start_minute = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].start_minute;
							end_hour	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].end_hour;
							end_minute	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].end_minute;

							time_set_start = start_hour * 60 + start_minute;
							time_set_end = end_hour * 60 + end_minute;

							if ( time_cur>=time_set_start && time_cur<=time_set_end && (time_set_start != 0 || time_set_end != 0) && time_set_start<=time_set_end)
							{
								g_timer_record_flag[i] = 1;
								//printf("time record: %d [%d-%d]!\n", time_cur, time_set_start, time_set_end);
								printf("RSDK: Stard time record(%d), time: %d [%d-%d]\n", i, time_cur, time_set_start, time_set_end);
								
								start_record(i, RECORD_TYPE_TIME);
							}
						}
					}
				}
				else
				{
					if (g_timer_record_flag[i])
					{
						if (g_record_setup_param.timer_record_param[i].day[week].nOnFlag==0 && g_record_setup_param.timer_record_param[i].day[0].nOnFlag==0)
						{
							g_timer_record_flag[i] = 0;

							//printf("stop_record: %d RECORD_TYPE_TIME\n", i);
							printf("RSDK: Stop time record(%d)\n", i);

							stop_record(i, RECORD_TYPE_TIME);
						}
						else
						{
							int stop_flag = 1;
							
							if (g_record_setup_param.timer_record_param[i].day[week].nOnFlag)
							{
								for (j=0; j<MAX_TIME_SEGMENT; j++)
								{
									start_hour	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].start_hour;
									start_minute = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].start_minute;
									end_hour	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].end_hour;
									end_minute	 = g_record_setup_param.timer_record_param[i].day[week].time_segment[j].end_minute;

									time_set_start = start_hour * 60 + start_minute;
									time_set_end = end_hour * 60 + end_minute;

									//if (time_cur <= time_set_end)
									// Change the code by lvjh, 2009-04-02
									if (time_cur>=time_set_start && time_cur<=time_set_end)
									{
										//printf("time record: %d [%d-%d]!\n", time_cur, time_set_start, time_set_end);
										stop_flag = 0;
									}
								}
							}
							if (g_record_setup_param.timer_record_param[i].day[0].nOnFlag)
							{
								for (j=0; j<MAX_TIME_SEGMENT; j++)
								{
									start_hour	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].start_hour;
									start_minute = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].start_minute;
									end_hour	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].end_hour;
									end_minute	 = g_record_setup_param.timer_record_param[i].day[0].time_segment[j].end_minute;

									time_set_start = start_hour * 60 + start_minute;
									time_set_end = end_hour * 60 + end_minute;

									//if (time_cur <= time_set_end)
									// Change the code by lvjh, 2009-04-02
									if (time_cur>=time_set_start && time_cur<=time_set_end)
									{
										//printf("time record: %d [%d-%d]!\n", time_cur, time_set_start, time_set_end);
										stop_flag = 0;
									}
								}
							}

							if (stop_flag)
							{
								g_timer_record_flag[i] = 0;

								//printf("stop_record: %d RECORD_TYPE_TIME\n", i);
								printf("RSDK: Stop time record(%d)\n", i);

								stop_record(i, RECORD_TYPE_TIME);
							}
						}
					}
				}
			} 
		}
	
		// 手动录像
		for (i=0; i<get_av_channel_num(); i++)
		{
			if (!videoLostFlag[i])
			{
				/*
				if (g_manual_record_flag[i])
				{
					old_manual_record_flag[i] = 1;
					old_manual_record_time[i] = g_manual_record_time[i] * 60;
					g_manual_record_flag[i] = 0;
					g_manual_record_time[i] = 0;
					printf("start_record: %d %d RECORD_TYPE_NORMAL\n", i, old_manual_record_time[i]);
					ret = start_record(i, RECORD_TYPE_NORMAL);
				}
				else
				{
					if (old_manual_record_time[i] > 1)
					{
						old_manual_record_time[i] -= (get_cur_tick_count() - last_time);
					}
					if (old_manual_record_time[i] == 1)
					{
						old_manual_record_flag[i] = 0;
						old_manual_record_time[i] = 0;
						printf("stop_record: %d, RECORD_TYPE_NORMAL\n", i);
						stop_record(i, RECORD_TYPE_NORMAL);
					}
				}
				*/
				
				if (g_manual_record_flag[i] && old_manual_record_flag[i]==0)
				{
					old_manual_record_flag[i] = 1;
					old_manual_record_time[i] = g_manual_record_time[i] * 60;
					//printf("start manual record: %d sec\n", old_manual_record_time[i]);
					printf("RSDK: Stard manual record(%d), time: %d\n", i, old_manual_record_time[i]);
					ret = start_record(i, RECORD_TYPE_NORMAL);
					manual_record_result(DEV_RECORD_RESULT);
				}
				else
				{
					// 如果手动停止录像且上次手动录像还没结束
					if (g_manual_record_flag[i]==0 && old_manual_record_flag[i]==1)
					{
						old_manual_record_flag[i] = 0;
						old_manual_record_time[i] = 0;
						g_manual_record_flag[i] = 0;
						g_manual_record_time[i] = 0;
						//printf("force to stop manual record\n");
						printf("RSDK: Stop manual record(%d), time: %d\n", i, old_manual_record_time[i]);
						stop_record(i, RECORD_TYPE_NORMAL);
					//	manual_record_result(DEV_RECORD_RESULT);
						
					}
					
					if (old_manual_record_time[i] > 1)
					{
						old_manual_record_time[i] -= (get_cur_tick_count() - last_time);
					}
					if (old_manual_record_time[i] == 1)
					{
						old_manual_record_flag[i] = 0;
						old_manual_record_time[i] = 0;
						g_manual_record_flag[i] = 0;
						g_manual_record_time[i] = 0;
						printf("RSDK: Stop manual record(%d), time: %d\n", i, old_manual_record_time[i]);
						stop_record(i, RECORD_TYPE_NORMAL);
						//manual_record_result(DEV_RECORD_RESULT);
					}
				}
			}
		}
		
		last_time = get_cur_tick_count();
		usleep(100);
	}
	
	pthread_exit(NULL);
	
	return 0;
}

// 启动录像处理线程
int record_control_start()
{
	int i = 0;
	int ret = -1;
	int max_priority;
	pthread_attr_t init_attr;
	struct sched_param init_priority;
	pthread_t thread;
	pthread_t thread_remount;
	

	// 初始化
	for (i=0; i<MAX_AV_CHANNEL; i++)
	{
		g_timer_record_flag[i] = 0;
		g_timer_record_time[i] = 0;

		g_manual_record_flag[i] = 0;
		g_manual_record_time[i] = 0;

		g_video_motion_record_flag[i] = 0;
		g_video_motion_record_time[i] = 0;
		g_video_status[i] = 0;

		g_detector_record_flag[i] = 0;
		g_detector_record_time[i] = 0;
		g_detector_status[i] = 0;
	}
    
	// 初始化录像线程的属性
	max_priority = sched_get_priority_max( SCHED_FIFO );
	pthread_attr_init( &init_attr );
	pthread_attr_setschedpolicy( &init_attr, SCHED_FIFO );
	pthread_attr_getschedparam( &init_attr, &init_priority );
	init_priority.sched_priority = max_priority;
	pthread_attr_setschedparam( &init_attr, &init_priority );
    
	g_record_ctrl_run_flag = 1;
	
	// 创建录像线程
	ret = pthread_create(&thread, NULL, (void *)record_control_fun, NULL);	
	if (ret != 0)
	{
		g_record_ctrl_run_flag = 0;

		pthread_attr_destroy(&init_attr);
		record_control_stop();
		return -1;
	}

	
	
	ret = pthread_create(&thread_remount, NULL, (void *)record_remount_fun, NULL);	
	if (ret != 0)
	{
		return -1;
	}


	pthread_attr_destroy(&init_attr);

	return	0;		
}

// 停止录像处理线程
int record_control_stop()
{
	g_record_ctrl_run_flag = 0;
	
	return 0;
}

// 停止录像处理线程
int record_control_pause()
{
	g_record_ctrl_pause_flag = 1;
	
	return 0;
}

// 停止录像处理线程
int record_control_resume()
{
	g_record_ctrl_pause_flag = 0;
	
	return 0;
}

// 设置手动录像
int set_manual_record_param(int channel, int time, int flag)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}
	if (time <= 0 && flag==1)	// Change the code by lvjh, 2009-04-14
	{
		return -1;	
	}
	if (flag<0 || flag>1)
	{
		return -1;
	}
	
	g_manual_record_flag[channel] = flag;
	g_manual_record_time[channel] = time;
	
	return 0;
}

int set_video_status(int channel, int status)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}
	
	g_video_status[channel] = status;
	
	return 0;	
}

int set_detector_status(int channel, int status)
{
	if (channel<0 || channel>=get_av_channel_num())
	{
		return -1;
	}
	if (status<0 && status>1)
	{
		return -1;
	}	
	
	g_detector_status[channel] = status;

	return 0;	
}


