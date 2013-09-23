#ifndef __ALARM_PROC_H_
#define __ALARM_PROC_H_

#include "param.h"

typedef enum
{
	ALARM_TYPE_BASE				= 0x1000,
		
	VIDEO_LOST_ALARM				= ALARM_TYPE_BASE+1,
	VIDEO_MOTION_ALARM				= ALARM_TYPE_BASE+2,
	PROBE_IN_ALARM					= ALARM_TYPE_BASE+3,
	VIDEO_MASK_ALARM				= ALARM_TYPE_BASE+4,

#ifdef JIN_KE_GUANG
	ROBOT_ALARM						= ALARM_TYPE_BASE+5,
#endif
	IRPROBE_IN_ALARM				= ALARM_TYPE_BASE+6,
	
}ALARM_TYPE;

typedef struct
{
	unsigned long nType;
	unsigned short nChannel;
	unsigned short nStatus;
	DATE_PARAM Time;

	unsigned long nReserve;
	
}ALARM_INFO;

typedef struct
{
	unsigned char strDevIP[16];
	
	unsigned long nReserve;
	
}TALK_REQUEST_INFO;

int alarmProcModuleInit();
int alarmProcModuleDeInit();

int alarmProcStart();
int alarmProcStop();
int alarmProcPause();
int alarmProcResume();
int videoLostAlarmSetup(int nChannel, VIDEO_LOST_ALARM_PARAM param);
int videoMotionAlarmSetup(int nChannel, VIDEO_MOTION_ALARM_PARAM param);
int probeAlarmSetup(int nChannel, PROBE_IN_ALARM_PARAM param);

#endif
