#ifndef __MISC_DRV_H_
#define __MISC_DRV_H_

#define PROBE_IN_NUM		2
#define PROBE_OUT_NUM		1

#define DEVICE_NAME		"tds_gpio"  //mody by lv was ---start old:noghing 



#if 0
typedef enum 
{

	GPIO_GET_PROBE_IN_STATUS,
	GPIO_GET_RESET_STATUS,
	GPIO_GET_TALK_REQUEST_STATUS,
	
	GPIO_SET_PROBE_OUT_STATUS,
	GPIO_SET_RS485_ENABLE,
	GPIO_SET_WATCH_DOG,
	GPIO_SET_INDICATOR,
	GPIO_AUTH,
	GPIO_INIT,
	GPIO_GET_IR_CUT_STATUS,
	GPIO_SET_IR_CUT_STATUS,

	
	GPIO_GET_MOTOR_VERTICAL_UP_STATUS,
	GPIO_GET_MOTOR_VERTICAL_DOWN_STATUS,
	GPIO_GET_MOTOR_HORIZON_RIGHT_STATUS,
	GPIO_GET_MOTOR_HORIZON_LEFT_STATUS,
	//GPIO_GET_ALRAM_GARRISON_STATUS,

	GPIO_GET_WIRELESS_ALARM_STATUS,
	GPIO_SET_ALRAM_GARRISON_STATUS,
	GPIO_SET_ALRAM_OUT,
}GPIO_OPT;
#endif
typedef enum 
{

	GPIO_GET_PROBE_IN_STATUS,
	GPIO_GET_RESET_STATUS,
	GPIO_GET_TALK_REQUEST_STATUS,
	
	GPIO_SET_PROBE_OUT_STATUS,
	GPIO_SET_RS485_ENABLE,
	GPIO_SET_WATCH_DOG,
	GPIO_SET_INDICATOR,
	GPIO_AUTH,
	GPIO_INIT,
	GPIO_GET_IR_CUT_STATUS,
	GPIO_GET_IR_CUT_VALUE,
	GPIO_SET_IR_CUT_STATUS,

	GPIO_GET_MOTOR_LIMIT_STATUS,
	GPIO_GET_MOTOR_LIMIT_VALUE,

	GPIO_GET_WIRELESS_ALARM_STATUS,
	GPIO_SET_ALRAM_GARRISON_STATUS,
	GPIO_SET_ALRAM_OUT,
	//mody by lv end old:nothing
}GPIO_OPT;

typedef enum{
	limit_no=0,
	limit_right=1,
	limit_left=2,
	limit_up=3,
	limit_down=4
}LIMIT_STATUS;
typedef enum{
	value_no=0x0,
	value_right = 0x01,
	value_left = 0x02,
	value_up = 0x04,
	value_down = 0x08
}LIMIT_VALUE;

typedef struct
{
	int opt;
	
	union
	{
	
		int probeInStatus;
		int talkRequestStatus;
		int resetStatus;
		int probeOutStatus;
		int rs485Enalbe;
		int watchDog;
		int indicatorStatus;
		int ircutInStatus;
		int ircutOutStatus;
		//mody by lv was ---start old:noghing old get:char authCode[10];
		int alarmGarrisonStatus;
		int alarmOutStatus;
		char authCode[10];

		int motorLimitStatus;
		int motorLimitValue;
		//mody by lv was ---end old:noghing 
	}param;
	//mody by lv was ---start old:noghing 
	int D0;
	int D1;
	int D2;
	int D3;
	//mody by lv was ---end old:noghing 
	int reserve;	
}GPIO_CMD;
//mody by lv was ---start old:noghing 
typedef enum 
{
	H_STEPEER_CONTROL_RIGHT,
	H_STEPEER_CONTROL_LETF,
 	H_STEPEER_CONTROL_STOP, 
 	H_STEPEER_CONTROL_ROTATION,
 	V_STEPEER_CONTROL_UP, 		
 	V_STEPEER_CONTROL_DOWN, 	
	V_STEPEER_CONTROL_STOP,  
	V_STEPEER_CONTROL_ROTATION,
	HV_STEPEER_CONTROL_GET_COORD,
	HV_STEPEER_CONTROL_GOTO,
	//HV_STEPEER_CONTROL_INIT,
}STEPEER_CONTROL_OPT;

typedef struct {
	unsigned int x;
	unsigned int y;
}COORD;
typedef struct stepper{
    STEPEER_CONTROL_OPT CmdID;  //indicate the type of the command
    int Speed;    //if the command is start,reverse,up or down,
	            //user can use this argument to set the speed.
    COORD coord;	            
}STEPPER; 
//mody by lv was ---end old:noghing 


int miscDrv_Load();
int miscDrv_Unload();
int miscDrv_Open();
int miscDrv_Close();
int miscDrv_GetProbeIn(GPIO_CMD *cmd);
int miscDrv_GetTalkRequest();
int miscDrv_SetProbeOut(int status);
int miscDrv_GetReset();
int miscDrv_SetRs485(int status);
int miscDrv_SetWatchDog();
int miscDrv_SetIndicator(int status);
int miscDrv_Authen();
int miscDrv_GetIrCutStatus();
int miscDrv_SetIrCutOutStatus(int status);

#endif
