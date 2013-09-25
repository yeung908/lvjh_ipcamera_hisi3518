#ifndef __FURTHER_GPIO_H_
#define __FURTHER_GPIO_H_

#define DEVICE_NAME		"tds_gpio"

#define IRQ_0             	6
#define IRQ_1             	7

#define PROBE_IN_NUM		1
#define PROBE_OUT_NUM		1
#define MAJOR_NUM 96

#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, int*)
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, int*)
//#define WIREALARM_STATUS_GET_MSG _IOR(MAJOR_NUM, 1, int*)

#if 0
#define 	H_STEPEER_MOTOR_RIGHT 0
#define 	H_STEPEER_MOTOR_LETF  1
#define 	H_STEPEER_MOTOR_STOP  2
#define 	V_STEPEER_MOTOR_UP 		3
#define 	V_STEPEER_MOTOR_DOWN 	4
#define 	V_STEPEER_MOTOR_STOP  5
#endif

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

	GPIO_GET_MOTOR_VERTICAL_UP_STATUS,
	GPIO_GET_MOTOR_VERTICAL_DOWN_STATUS,
	GPIO_GET_MOTOR_HORIZON_RIGHT_STATUS,
	GPIO_GET_MOTOR_HORIZON_LEFT_STATUS,
	
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
	GPIO_SET_IR_CUT_STATUS,

	
	GPIO_GET_MOTOR_LIMIT_STATUS,
	GPIO_GET_MOTOR_LIMIT_VALUE,

	GPIO_GET_WIRELESS_ALARM_STATUS,
	GPIO_SET_ALRAM_GARRISON_STATUS,
	GPIO_SET_ALRAM_OUT,
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

#if 0

typedef struct
{
	int opt;
	
	union
	{
		int probeInStatus;
		int resetStatus;
		int motorVerticalUpStatus;
		int motorVerticalDownStatus;
		int motorHorizonRightStatus;
		int motorHorizonLetfStatus;

		int probeOutStatus;
		int rs485Enalbe;
		int watchDog;
		int indicatorStatus;
		int alarmGarrisonStatus;
		char authCode[10];
		
	}param;
	int D0;
	int D1;
	int D2;
	int D3;
	
	int reserve;
	
}GPIO_CMD;
#endif

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
		int alarmGarrisonStatus;
		int alarmOutStatus;
		char authCode[10];

		int motorLimitStatus;
		int motorLimitValue;
		
	
	}param;
	int D0;
	int D1;
	int D2;
	int D3;
	int reserve;
	
}GPIO_CMD;
typedef struct {
	unsigned int x;
	unsigned int y;
}COORD;
struct stepper{
    STEPEER_CONTROL_OPT CmdID;  //indicate the type of the command
    int Speed;    //if the command is start,reverse,up or down,
	            //user can use this argument to set the speed.
    COORD coord;	            
}; 

#define margin_probe_in	HZ

unsigned int misc_major = 0;

#endif
