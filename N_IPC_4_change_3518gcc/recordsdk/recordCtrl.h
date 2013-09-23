#ifndef __RECORD_CTRL_H_
#define __RECORD_CTRL_H_

#include "recordStruct.h"

int set_videomotion_record_param(int channel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param);
int get_videomotion_record_param(int channel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param);
int set_timer_record_param(int channel, TIMER_RECORD_CHANNEL_PARAM *param);
int get_timer_record_param(int channel, TIMER_RECORD_CHANNEL_PARAM *param);
int set_detector_record_param(int channel, DETECTOR_RECORD_CHANNEL_PARAM *param);
int get_detector_record_param(int channel, DETECTOR_RECORD_CHANNEL_PARAM *param);
int set_record_param(int channel, RECORD_CHANNEL_PARAM *param);
int get_record_param(int channel, RECORD_CHANNEL_PARAM *param);
int set_all_record_param(RECORD_SETUP *param);
int get_all_record_param(RECORD_SETUP *param);
int record_control_start();
int record_control_stop();
int record_control_pause();
int record_control_resume();
int set_manual_record_param(int channel, int time, int flag);
int set_video_status(int channel, int status);
int set_detector_status(int channel, int status);


#endif

