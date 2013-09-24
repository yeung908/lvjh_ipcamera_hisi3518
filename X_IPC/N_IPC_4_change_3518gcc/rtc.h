#ifndef __RTC_H_
#define __RTC_H_

// 时间参数
typedef struct
{
	unsigned long year;
	unsigned long month;
	unsigned long week;
	unsigned long day;
	unsigned long hour;
	unsigned long minute;
	unsigned long second;

}DATE_PARAM;

int rtcModuleLoad();
int rtcModuleUnload();
int rtcCalibrationStart();
int rtcCalibrationStop();
int rtcCalibrationPause();
int rtcCalibrationResume();
int rtcCalibration(DATE_PARAM param);

#endif
