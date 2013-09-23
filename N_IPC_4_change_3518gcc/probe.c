#include <stdlib.h>
#include <stdio.h>
#include<fcntl.h>
#include <pthread.h>

#include "probe.h"
#include "miscDrv.h"
#include "param.h"

#include "ptz.h"


// 鍏ㄥ眬鍙橀噺
static int g_misc_dev_open_flag = 0;
static int g_reset_param_status = 0;
static int g_input_probe_status = 0;
static int g_output_probe_status = 0;
static int g_talk_request_status = 0;
static int g_probe_proc_run = 0;
static int g_probe_proc_reset_run = 0;
static int g_probe_out_run = 0;
static int g_output_probe_flag = 0;


static int g_probe_proc_pause = 0;
static int g_probe_alarm_stop_control = 0;

static int g_output_probe_time[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int g_cur_output_probe_status[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static int g_motion_output_probe_time = 0;

PROBE_OUT_ALARM_PARAM g_alarm_out_param[MAX_PROBE_OUT];
PROBE_IN_ALARM_PARAM g_probe_alarm_param[MAX_PROBE_IN];

static unsigned long cur_year = 0;
static unsigned long cur_month = 0;
static unsigned long cur_day = 0;
static unsigned long cur_week = 0;
static unsigned long cur_hour = 0;
static unsigned long cur_minute = 0;
static unsigned long cur_second = 0;

static int g_flags = 0;


pthread_mutex_t g_input_probe_mutex;
pthread_mutex_t g_output_probe_mutex;

int getResetParamStatus()
{
	return g_reset_param_status;
}

int getProbeInStatus(int nChannel)
{
	int status = 0;

	pthread_mutex_lock(&g_input_probe_mutex);

	status = (g_input_probe_status>>nChannel)&0x01;

	g_input_probe_status = 0;	// Add the code by lvjh, 2009-09-01

	pthread_mutex_unlock(&g_input_probe_mutex);

	return status;
}

int setProbeInStatus(int nChannel)
{
	int status = 0;

	pthread_mutex_lock(&g_input_probe_mutex);

	g_input_probe_status = g_input_probe_status | (0x01 << nChannel);

	pthread_mutex_unlock(&g_input_probe_mutex);

	return 0;
}

int getProbeOutStatus(int nChannel)
{
	int status = 0;

	pthread_mutex_lock(&g_output_probe_mutex);

	status = (g_output_probe_status>>nChannel)&0x01;

	pthread_mutex_unlock(&g_output_probe_mutex);

	return status;
}

int setProbeOutStatus(int nChannel, int time)
{
	int status = 0;

	if (nChannel<0 || nChannel>=16)
	{
		return -1;
	}

	pthread_mutex_lock(&g_output_probe_mutex);
	g_output_probe_status = g_output_probe_status | (0x01 << nChannel);
	g_output_probe_time[nChannel] = time;
	pthread_mutex_unlock(&g_output_probe_mutex);

	//printf("setProbeOutStatus: %d %d\n", g_output_probe_status, g_output_probe_time[nChannel]);

	return 0;
}


int setMotionProbeOutStatus(int nChannel, int time)
{
	int status = 0;

	if (nChannel<0 || nChannel>=16)
	{
		return -1;
	}

	pthread_mutex_lock(&g_output_probe_mutex);
	g_output_probe_flag = 1;
	g_motion_output_probe_time = time;

	pthread_mutex_unlock(&g_output_probe_mutex);

	//printf("setProbeOutStatus: %d %d\n", g_output_probe_status, g_output_probe_time[nChannel]);

	return 0;
}


int manualSetProbeOutStatus(int nChannel, int status)
{
	if (nChannel<0 || nChannel>=16)
	{
		return -1;
	}

	pthread_mutex_lock(&g_output_probe_mutex);

	if (status)
	{
		g_output_probe_status = g_output_probe_status | (0x01 << nChannel);
	}
	else
	{
		g_output_probe_status &= (~(g_output_probe_status | (0x01 << nChannel)));
	}

	miscDrv_SetProbeOut(g_output_probe_status);

	pthread_mutex_unlock(&g_output_probe_mutex);

	return 0;
}

int getTalkRequestStatus()
{
	return g_talk_request_status;
}

int probeModuleInit()
{
	int ret = -1;

	miscDrv_Load();
	ret = miscDrv_Open();
	if (ret < 0)
	{
		return -1;
	}

	g_misc_dev_open_flag = 1;

	pthread_mutex_init(&g_input_probe_mutex, NULL);
	pthread_mutex_init(&g_output_probe_mutex, NULL);

	return 0;
}

int probeModuleDeInit()
{
	int ret = -1;

	g_input_probe_status = 0;
	g_output_probe_status = 0;
	g_probe_proc_run = 0;
	g_probe_proc_pause = 0;

	pthread_mutex_destroy(&g_input_probe_mutex);
	pthread_mutex_destroy(&g_output_probe_mutex);

	return 0;
}

int ProbeOutFun()
{
	int ret = 0;
	int count = 0;
	int resetFlag = 0;
	int counter = 0;

	sleep(10);
	while(g_probe_out_run)
	{
		usleep(1000*100);
		if(g_output_probe_flag == 1)
		{
			//	pthread_mutex_lock(&g_output_probe_mutex);
			printf("motion out alarm %d.....\n", g_motion_output_probe_time);
			miscDrv_SetProbeOut(0);
			sleep(g_motion_output_probe_time);
			miscDrv_SetProbeOut(1);
			//printf("alarm out over\n");
			g_output_probe_flag = 0;
			//	pthread_mutex_unlock(&g_output_probe_mutex);
		}
	}
}



int ProbeProcRestFun()
{
	int ret = 0;
	int count = 0;
	int resetFlag = 0;

	while(g_probe_proc_reset_run)
	{
		usleep(1000*100);
		ret = miscDrv_GetReset();
		if (ret != -1)
		{
			if (ret == 1)
			{
				resetFlag  = 1;
				count++;
			}
			if (ret == 2)
			{
				resetFlag  = 0;

				if (count >= 5)
				{
					printf("Warning: Resume default parameter!!!\n");

					g_reset_param_status = 1;

					initSystemParam(HARDWARE_RESET);
					sleep(1);

					system("rm -rf /param/param.*");

					sleep(1);

					RebootSystem();
				}
				count = 0;
			}
		}
	}
}

int ProbeProcFun()
{
	int ret = -1;
	int alarm_status = -1;
	int i = 0;
	int j = 0;
	int status = 0;
	int time = 0;
	int alarm_flag = 0;
	int flag = 0;
	int cur_time = 0;
	int resetflag = 0;
	int count = 0;
	GPIO_CMD cmd;
	int flags = 1;
	int current_status = 0;
	int initial_status = 0;
	PROBE_IN_ALARM_PARAM param;
	ALARM_STATUS_PARAM alarm_status_param;
	getAlarmStatusParam(&alarm_status_param);
	if(alarm_status_param.nIrAlarmStatus[0] == 1)
		setIrProbeAlarmStartParam();
	else
		setIrProbeAlarmStopParam();


	while (g_probe_proc_run)
	{

		flags = miscDrv_GetProbeIn(&cmd);
		miscDrv_SetIndicator(0);

		g_probe_alarm_stop_control = 0;
		initial_status = 0;

		usleep(1000*100);
		//printf("flags : %d .... getIrProberAlarmParam():%d g_probe_alarm_stop_control %d\n", flags, getIrProberAlarmParam(), g_probe_alarm_stop_control);
		while((flags == 1)||getIrProberAlarmParam())
		{
			usleep(1000*100);
			if(initial_status == 0)
			{
				printf(" set indicator status\n");
				miscDrv_SetProbeOut(0);
				sleep(1);
				miscDrv_SetProbeOut(1);
				miscDrv_SetIndicator(1);//当前处于布防状态
				initial_status = 1;
			}
			if(flags == 1)
			{
				getAlarmStatusParam(&alarm_status_param);
				alarm_status_param.nIrAlarmStatus[0] = 1;
				setAlarmStatusParam(&alarm_status_param);
			}
			if (g_probe_alarm_stop_control)
			{
				printf("WEB control alarm stop\n");
				probeAlarmStart();
				break;
			}
			alarm_status = miscDrv_GetProbeIn(&cmd);//读取报警探头状态
			//printf("alarm_status = %d\n", alarm_status);
			//sleep(1);
			if (g_cur_output_probe_status[i] == 0)
			{
#if 1
				switch(alarm_status)
				{
				case 1:
					flags = 1;//布防
					break;
				case 4:
					miscDrv_SetProbeOut(1);
					miscDrv_SetIndicator(0);
					setIrProbeAlarmStopParam();
					probeNetAlarmStop();
					flags = 0; //撤防
					current_status = 0;
					printf("current_status = %d\n", current_status);
					alarm_status_param.nIrAlarmStatus[0] = 0;
					setAlarmStatusParam(&alarm_status_param);
					//saveParamToFile();
					break;
				case 9://探头7报警
					alarm_flag  = 1;
					break;
				default:
					//printf("Sorry alarm NUmber don't exist\n");
					break;

				}
#endif

			}
			getSystemTimeExt(&cur_year, &cur_month, &cur_day, &cur_week, &cur_hour, &cur_minute, &cur_second);
			cur_time = cur_hour*60 + cur_minute;
			for (i=0; i<MAX_PROBE_OUT; i++)
			{
				flag = 0;

				//	printf("cur_week = %d\n", cur_week);
				//	sleep(3);

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
				if (g_probe_alarm_param[i].day[0].nOnFlag)
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

				if (flag == 0)
				{
					continue;
				}


			}
			time = 0;
			time = flag;
			if((time > 0) && alarm_flag)
			{
				ddnsYiYuanResume(3);
				printf("alarm_flag = %d\n", alarm_flag);
				alarm_flag = 0;
				memset(&param, 0, sizeof(PROBE_IN_ALARM_PARAM));
				getProbeInAlarmParam(0, &param);
				printf("param.bPresetNo[0] = %d\n", param.bPresetNo[0]);
				if(param.bPresetNo[0] > 0)
					ptzGotoPoint(param.bPresetNo[0]);
				param.reserve = 1;
				setProbeInAlarmParam(0, &param);
				probeNetAlarmStart();

			}

		}
		// 鏇存柊鏃堕棿
		if (time > 0)
		{
			g_output_probe_time[i]--;
		}
	}

	sleep(1);


	pthread_exit(NULL);

	return 0;
}


int probeProcStart()
{
	int ret = -1;
	int i = 0;
	pthread_t threadID;
	pthread_t threadID_reset;
	pthread_t threadID_prob_out;



	if (g_misc_dev_open_flag != 1)
	{
		printf("probeProcStart(): Failed!\n");
		return -1;
	}

	for (i=0; i<MAX_PROBE_OUT; i++)
	{
		getProbeOutAlarmParam(i, &g_alarm_out_param[i]);
		getProbeInAlarmParam(i, &g_probe_alarm_param[i]);
	}

	g_probe_proc_run = 1;
	g_probe_proc_reset_run = 1;
	g_probe_out_run = 1;
	miscDrv_SetProbeOut(1);
	setIrProbeAlarmStopParam();

	ret = pthread_create(&threadID, NULL, (void *)ProbeProcFun, NULL);
	if (ret)
	{
		g_probe_proc_run = 0;
		return -1;
	}

#if 1
	ret = pthread_create(&threadID_prob_out, NULL, (void *)ProbeOutFun, NULL);
	if (ret)
	{
		g_probe_proc_run = 0;
		return -1;
	}
#endif


	ret = pthread_create(&threadID_reset, NULL, (void *)ProbeProcRestFun, NULL);
	if (ret)
	{
		g_probe_proc_reset_run = 0;
		return -1;
	}

	printf("probeProcStart(): OK!\n");

	return 0;
}

int probeProcStop()
{
	g_probe_proc_run = 0;

	return 0;
}

int probeProcPause()
{
	g_probe_proc_pause = 1;

	return 0;
}

int probeAlarmStop()
{
	g_probe_alarm_stop_control = 1;
	return 0;
}

int probeAlarmStart()
{
	g_probe_alarm_stop_control = 0;
	return 0;
}

int probeProcResume()
{
	g_probe_proc_pause = 0;

	return 0;
}

int probeProcSetup(int nChannel, PROBE_OUT_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=MAX_PROBE_OUT)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	memcpy(&g_alarm_out_param[nChannel], param, sizeof(PROBE_OUT_ALARM_PARAM));

	return 0;
}


