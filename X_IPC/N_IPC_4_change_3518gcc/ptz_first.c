#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include<sys/ioctl.h> 


#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"
#include "session.h"
#include "com.h"
#include "param.h"
#include "miscDrv.h"

#include "ptz.h"
//#include "tv_gpio.h"

#if 1
#define BYPASS 33
// 全局变量
int g_rs485_fd = -1;
int g_rs232_fd = -1;
PTZ_INDEX g_ptz_index;
int g_ptz_run_flag = 0;
int g_ptz_pause_flag = 0;
int g_cur_channel = 0;
int g_cur_ptz_cmd = 0;
int g_cur_ptz_value = 0;
int g_ptz_init_flag[4] = {0, 0, 0, 0};
int g_ptz_auto_flag[4] = {0, 0, 0, 0};
int g_ptz_checksum = 0;
PTZCTRLCMD_THROUGH g_ptz_bypass_send;
PTZCTRLCMD g_ptz_bypass_recv;
sem_t g_ptz_sem;

// 云台解析
static PTZCTRLINFO g_ptz_ctrl_info[4];
static PTZ_AUTO_CTRL g_ptz_auto_ctrl[4];
#endif


#define ptz_conf_path "/param/ptz.conf"
#define PTZ_MOTOR_STEPEER	 "/dev/tds_gpio"


#define H_MAJOR_NUM 96
#define V_MAJOR_NUM 97
#define IOCTL_SET_MSG _IOR(H_MAJOR_NUM, 0, int*)
#define IOCTL_GET_MSG_V _IOR(V_MAJOR_NUM, 1, int*)
#define IOCTL_SET_MSG _IOR(V_MAJOR_NUM, 0, int*)
#define IOCTL_GET_MSG _IOR(H_MAJOR_NUM, 1, int*)

int g_cur_hor_angle=0;   //以脉冲个数代表坐标水平方向0~256,顺时针+，逆时针-
int g_cur_ver_angle=0;   //以脉冲个数代表坐标水平方向0~64
    
int g_ver_run_flag=0;
int g_hor_run_flag=0;

int direction_hor = 0;              // 水平运动方向:1向右 0向左
int direction_ver = 0;              //垂直运动方向: 1向上 0向下

int H_hor_limit_stop = 64*64-57;   //水平方向坐标范围:0~(64*64-57)
int H_ver_limit_stop = 16*64-57;    //垂直方向坐标范围:0~(16*64-57)

int speed=0;
int fd_h=-1;
sem_t hor_sem;
sem_t ver_sem;


pthread_mutex_t g_misc_dev_mutex = PTHREAD_MUTEX_INITIALIZER;
typedef enum
{
	hor_run_stopflag  = 0,
	hor_run_rightflag = 1,
	hor_run_leftflag = 2,
	hor_run_rotation = 3,
	hor_run_goto = 4,
	hor_run_init = 5,
}HOR_RUN;
typedef enum
{
	clockwise = 0,
	stop = 3,
	anticlockwise   = 2,
	up = 3,
	down = 4,
}stepmotor;
typedef enum
{
	ver_run_stopflag  =0,
	ver_run_upflag=1,
	ver_run_downflag=2,
	ver_run_goto=3,
	ver_run_init = 4,
}VER_RUN;
typedef struct _PTZ_CONF 
{
	char speed[24];
	char point[33][24];
}PTZ_CONF;

typedef struct _POINT_COORD
{
	int nPoint;
	int coord_x;
	int coord_y;
}POINT_COORD;
PTZ_CONF ptz_conf; 

typedef struct _GOTO_OFFSET
{
	int x_offset;
	int y_offset;
}GOTO_OFFSET;
GOTO_OFFSET goto_offset;
typedef enum 
{
	H_STEPEER_CONTROL_RIGHT,
	H_STEPEER_CONTROL_LETF,
 	H_STEPEER_CONTROL_STOP, 
 	V_STEPEER_CONTROL_UP, 		
 	V_STEPEER_CONTROL_DOWN, 	
	V_STEPEER_CONTROL_STOP,  
}STEPEER_CONTROL_OPT;


typedef struct _STEPPER{
    STEPEER_CONTROL_OPT CmdID;  //indicate the type of the command
 	int Speed;   //user can use this argument to set the speed.
}STEPPER;

GOTO_OFFSET point_init;           //记录初始位置，以便自检后移动到该位置
void ptzRightStart()
{
	g_hor_run_flag = hor_run_rightflag;	
	sem_post(&hor_sem);
}

void ptzLeftStart()
{
	g_hor_run_flag = hor_run_leftflag;
	sem_post(&hor_sem);
}
void ptzHorStop()
{
	g_hor_run_flag = hor_run_stopflag;	
	//sem_post(&hor_sem);
}


void ptzUpStart()
{
	g_ver_run_flag = ver_run_upflag;
	sem_post(&ver_sem);
			
}
void ptzDownStart()
{
	g_ver_run_flag = ver_run_downflag;
	sem_post(&ver_sem);		
}
void ptzVerStop()
{
	g_ver_run_flag = ver_run_stopflag;
	//sem_post(&ver_sem);
}
void ptzReset()
{
	g_ver_run_flag = ver_run_init;
	g_hor_run_flag = hor_run_init;
	sem_post(&ver_sem);
	sem_post(&hor_sem);
}
void ptzRotationStart()
{
	g_hor_run_flag = hor_run_rotation;	
	sem_post(&hor_sem);	
}
void clear_point_list()
{	
	int i,ret;
	FILE *fp;
	memset(&ptz_conf,0,sizeof(PTZ_CONF));
	fp=fopen(ptz_conf_path,"wb+");
	for(i=0;i<32;i++)
	{
		sprintf(ptz_conf.point[i],"point%d=",i);
	}
	sprintf(ptz_conf.point[32],"");
	sprintf(ptz_conf.speed,"speed=%d",63);
	fseek(fp,0L,SEEK_SET);
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
}

int ptzInit()
{

	int ret;
	FILE *fp;
	memset(&ptz_conf,0,sizeof(PTZ_CONF));
	fd_h = open(PTZ_MOTOR_STEPEER,O_RDWR);
	if(fd_h < 0){
		printf("open stepeer motor error \n");
		return -1;
	}
	fp=fopen(ptz_conf_path,"rb+");
	if (fp == NULL)
	{
		clear_point_list();
		fp=fopen(ptz_conf_path,"rb+");
		if(fp == NULL)
		{
			printf("Open file %s fault\n",ptz_conf_path);
			return -1;
		}
		
	}
	ret = fread(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	if(ret != sizeof(PTZ_CONF))
	{
		printf("read ptz_conf error\n");
		fclose(fp);	
		return -1;
	}
	ret = sscanf(ptz_conf.speed,"speed=%d%[^'\0']",&speed);
	if(ret != 1) 
	{
		printf("speed have set yes");
		speed=63;
		return ;
	}
	fclose(fp);
	return 0;
}


void ptzSetPoint( unsigned char nPoint)
{
	int ret;
	nPoint=(nPoint>32)?32:nPoint;
	nPoint=(nPoint<1)?1:nPoint;
	nPoint--;
	sprintf(ptz_conf.point[nPoint],"point%d=%d,%d",nPoint,g_cur_hor_angle,g_cur_ver_angle);
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),ptz_conf_fd);
}

void ptzGotoPoint( unsigned char nPoint)
{
	int ret;
	POINT_COORD point_coord;
	nPoint=(nPoint>32)?32:nPoint;
	nPoint=(nPoint<1)?1:nPoint;
	nPoint--;
	ret =sscanf(ptz_conf.point[nPoint],"point%*[^=]=%d,%d%[^'\0']",&point_coord.coord_x,&point_coord.coord_y);
	if (ret != 2)
	{
		printf("you haven't set this point yet.\n");
		return;
	}
	printf("point %d\t coordx = %d\tcoordy=%d\n",nPoint,point_coord.coord_x,point_coord.coord_y);
	goto_offset.x_offset = point_coord.coord_x-g_cur_hor_angle;
	goto_offset.y_offset = point_coord.coord_y-g_cur_ver_angle;
	printf("goto point offset:x_offset = %d,y_offset = %d\n",goto_offset.x_offset,goto_offset.y_offset);
	g_ver_run_flag = ver_run_goto;
	g_hor_run_flag = hor_run_goto;
	sem_post(&ver_sem);
	sem_post(&hor_sem);
	
}
void ptzClearPoint( unsigned char nPoint)
{
	int ret;
	FILE *fp;
	nPoint=(nPoint>32)?32:nPoint;
	nPoint=(nPoint<1)?1:nPoint;
	nPoint--;
	memset(ptz_conf.point[nPoint],0,sizeof(ptz_conf.point[nPoint]));
	sprintf(ptz_conf.point[nPoint],"point%d=",nPoint);
	fp=fopen(ptz_conf_path,"wb+");
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
}

void ptzSetSpeed(int speed)
{
	FILE *fp;
	int ret;
	sprintf(ptz_conf.speed,"speed=%d",speed);
	fp=fopen(ptz_conf_path,"wb+");
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
}
int miscDrv_GetMotorStatus(int g_misc_fd,int ptz_cmd)
{
	int ret = -1;
	GPIO_CMD cmd;
	if (g_misc_fd > 0)
	{
		cmd.opt = ptz_cmd;
		cmd.param.motorVerticalUpStatus = 0;

		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.motorVerticalUpStatus;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}
/********************限位开关检测线程*************************/
int test_limit_switch()
{
	int fd = -1;
	int ret =-1;
	int opt;
	fd = open(DEVICE_NAME,O_RDONLY);
	while(1)
	{
		sleep(1);
	/**********************垂直方向限位开关**************************/
		opt = GPIO_GET_MOTOR_VERTICAL_UP_STATUS;
		ret = miscDrv_GetMotorStatus(fd,opt);
		if(ret > 0)
		{
			if(g_ver_run_flag == ver_run_init)
			{
				direction_ver = 1-direction_ver;
				H_ver_limit_stop = g_cur_ver_angle;
			}
			else
			{
				g_ver_run_flag = ver_run_stopflag;
				g_cur_ver_angle = H_ver_limit_stop;
			}
		}
		opt = GPIO_GET_MOTOR_VERTICAL_DOWN_STATUS;
		ret = miscDrv_GetMotorStatus(fd,opt);
		if(ret > 0)
		{
			if(g_ver_run_flag == ver_run_init)
			{
				direction_ver = 1-direction_ver;
				point_init.y_offset = -g_cur_ver_angle;
				g_cur_hor_angle = 0;
			}
			else
			{
				g_ver_run_flag = ver_run_stopflag;
				g_cur_ver_angle = 0;
			}

		}
	/**********************水平方向限位开关*******************************/
		opt = GPIO_GET_MOTOR_HORIZON_RIGHT_STATUS;
		ret = miscDrv_GetMotorStatus(fd,opt);
		if(ret > 0)
		{
			if(g_ver_run_flag == hor_run_init)
			{
				direction_hor = 1-direction_hor;
				H_hor_limit_stop = g_cur_hor_angle;
			}
			else
			{
				g_hor_run_flag = hor_run_stopflag;
				g_cur_hor_angle = H_hor_limit_stop;
			}
		}
		opt = GPIO_GET_MOTOR_HORIZON_LEFT_STATUS;
		ret = miscDrv_GetMotorStatus(fd,opt);
		if(ret > 0)
		{
			if(g_ver_run_flag == hor_run_init)
			{
				direction_hor = 1-direction_hor;
				point_init.x_offset = -g_cur_hor_angle;
				g_cur_hor_angle = 0;
			}
			else
			{
				g_hor_run_flag = hor_run_stopflag;
				g_cur_hor_angle = 0;
			}
		}
	}
}
/*******************水平方向电机控制线程***********************/
int hor_motor_ctrl()
{
	char stop_flag = 1;
	STEPPER  s;
	int finish=0;
	memset(&s,0,sizeof(STEPPER));
	printf("*****hor start*******\n");
	while(1)
	{
		usleep(10);
		if(stop_flag == 1)
		{
			printf("current coordinate was x:%d\t y:%d\n",g_cur_hor_angle,g_cur_ver_angle);
			sem_wait(&hor_sem);
			stop_flag = 0;
		}
		switch(g_hor_run_flag)
		{
	    	case hor_run_stopflag:
				stop_flag = 1;
				printf("HOR step motor STOP\n");
				break;
			case hor_run_rightflag:
				if(g_cur_hor_angle == H_hor_limit_stop)
					break;
				printf("HOR step motor RIGHT\n");
				s.CmdID = H_STEPEER_CONTROL_RIGHT;
				s.Speed = speed;
				ioctl(fd_h, IOCTL_SET_MSG, &s);
				g_cur_hor_angle++;
				if(g_cur_hor_angle >= H_hor_limit_stop )
				{
					g_cur_hor_angle = H_hor_limit_stop;
					stop_flag = 1;
					break;
				}
				
				break;
			case hor_run_leftflag:
				if(g_cur_hor_angle == 0)
					break;
				printf("HOR step motor LEFT\n");
				s.CmdID = H_STEPEER_CONTROL_LETF;
				s.Speed = speed;
				ioctl(fd_h, IOCTL_SET_MSG, &s);
				g_cur_hor_angle--;
				if(g_cur_hor_angle <= 0)
				{
					g_cur_hor_angle = 0;
					stop_flag = 1;
					break;
				}
				break;
		/***************水平方向自动巡航*****************/
			case hor_run_rotation:                           
				if(direction_hor)
				{
					printf("HOR step motor RIGHT\n");
					s.CmdID = H_STEPEER_CONTROL_RIGHT;
					s.Speed = speed;
					ioctl(fd_h, IOCTL_SET_MSG, &s);
					g_cur_hor_angle++;
					if(g_cur_hor_angle >= H_hor_limit_stop)
					{
						g_cur_hor_angle = H_hor_limit_stop;
						direction_hor = 0;
					}
				}
				else
				{
					printf("HOR step motor LEFT\n");
					s.CmdID = H_STEPEER_CONTROL_LETF;
					s.Speed = speed;
					ioctl(fd_h, IOCTL_SET_MSG, &s);
					g_cur_hor_angle--;
					if(g_cur_hor_angle <= 0)
					{
						g_cur_hor_angle = 0;
						direction_hor = 1;
					}
				}
				break;
		/********************goto预置点**************************/
			case hor_run_goto:
				while(goto_offset.x_offset != 0)
				{
					if(goto_offset.x_offset > 0)
					{
						printf("HOR step motor RIGHT\n");
						s.CmdID = H_STEPEER_CONTROL_RIGHT;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_hor_angle++;
						goto_offset.x_offset--;
						printf("x_offset was :%d\n",goto_offset.x_offset);
						if(g_cur_hor_angle >= H_hor_limit_stop)
						{
							g_cur_hor_angle = H_hor_limit_stop;
							stop_flag = 1;
							break;
						}
						
					}
					else
					{
						printf("HOR step motor LEFT\n");
						s.CmdID = H_STEPEER_CONTROL_LETF;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_hor_angle--;
						goto_offset.x_offset++;
						printf("x_offset was :%d\n",goto_offset.x_offset);
						if(g_cur_hor_angle <= 0)
						{
							g_cur_hor_angle = 0;
							stop_flag = 1;
							break;
						}
						
					}
				}
				stop_flag=1;
				break;
		/******************开机自检************************/
				case hor_run_init:                           
					if(direction_hor)
					{
						printf("HOR step motor RIGHT\n");
						s.CmdID = H_STEPEER_CONTROL_RIGHT;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_hor_angle++;
					}
					else
					{
						printf("HOR step motor LEFT\n");
						s.CmdID = H_STEPEER_CONTROL_LETF;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_hor_angle--;
						if(g_cur_hor_angle == direction_hor/2)
						{
							stop_flag = 1;
							break;
						}
						
					}
				break;
			default:
				break;
		}
	}
}
/***************垂直方向电机控制线程*******************/
int ver_motor_ctrl()
{
	char stop_flag = 1;
	int direction_ver= 0;
	STEPPER  s;
	memset(&s,0,sizeof(STEPPER));
	printf("****ver start********\n");
	while(1)
	{
		usleep(10);
		if(stop_flag == 1)
		{
			printf("current coordinate was x:%d\t y:%d\n",g_cur_hor_angle,g_cur_ver_angle);
			sem_wait(&ver_sem);
			stop_flag = 0;
		}
		switch(g_ver_run_flag)
		{
			case ver_run_stopflag:
				stop_flag = 1;
				printf("VER step motor STOP\n");
				break;
			case ver_run_upflag:
				
				if(g_cur_ver_angle == H_ver_limit_stop)
					break;
				s.CmdID = V_STEPEER_CONTROL_UP;
				s.Speed = speed;
				printf("VER step motor UP\n");
				ioctl(fd_h, IOCTL_SET_MSG, &s);
				g_cur_ver_angle++;
				if(g_cur_ver_angle >= H_ver_limit_stop)
				{
					g_cur_ver_angle = H_ver_limit_stop;
					stop_flag = 1;
					break;
				}
				break;
			case ver_run_downflag:
				if(g_cur_ver_angle == 0)
					break;
				printf("VER step motor DOWN\n");
				s.CmdID = V_STEPEER_CONTROL_DOWN;
				s.Speed = speed;
				ioctl(fd_h, IOCTL_SET_MSG, &s);
				g_cur_ver_angle--;
				if(g_cur_ver_angle <= 0)
				{
					g_cur_ver_angle = 0;
					stop_flag = 1;
					break;
				}
				
				break;
			case ver_run_goto:
				while(goto_offset.y_offset != 0)
				{
					if(goto_offset.y_offset > 0)
					{
						printf("VER step motor down\n");
						s.CmdID = V_STEPEER_CONTROL_UP;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_ver_angle++;
						goto_offset.y_offset--;
						if(g_cur_ver_angle >= H_ver_limit_stop)
						{
							g_cur_ver_angle = H_ver_limit_stop;
							stop_flag = 1;
							break;
						}
						
					}
					else
					{
						printf("VER step motor UP\n");
						s.CmdID = V_STEPEER_CONTROL_DOWN;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_ver_angle--;
						goto_offset.y_offset++;
						if(g_cur_ver_angle <= 0)
						{
							g_cur_ver_angle = 0;
							stop_flag = 1;
							break;
						}
						
					}
				}
				stop_flag=1;
				break;
			case ver_run_init:
					if(direction_ver)
					{
						printf("HOR step motor UP\n");
						s.CmdID = V_STEPEER_CONTROL_UP;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_ver_angle++;
					}
					else
					{
						printf("HOR step motor DOWN\n");
						s.CmdID = V_STEPEER_CONTROL_DOWN;
						s.Speed = speed;
						ioctl(fd_h, IOCTL_SET_MSG, &s);
						g_cur_ver_angle--;
						if(g_cur_ver_angle == H_ver_limit_stop/2)
						{
							stop_flag = 1;
							break;
						}

					}			
				break;
			default:
				break;
		}
	}
}

int  ptzctrl()
{	
	int ret;
	pthread_t hor_motor_id,ver_motor_id,limit_switch_id;
	sem_init(&hor_sem,0,0);
	sem_init(&ver_sem,0,0);
	ret = pthread_create(&limit_switch_id, NULL, (void *)test_limit_switch, NULL);
	if(ret)
	{
		printf("Limit switch test thread creat fault\n");
		return -1;
	}
	ret = pthread_create(&hor_motor_id, NULL, (void *)hor_motor_ctrl, NULL);
	if(ret)
	{
		printf("Horizontal motor control thread creat fault\n");
		return -1;
	}
	ret = pthread_create(&ver_motor_id, NULL, (void *)ver_motor_ctrl, NULL);
	if(ret)
	{
		printf("Vertical motor control thread creat fault\n");
		return -1;
	}
	sleep(2);
	ptzReset();
	while(1)
	{
		sleep(10);
	}
	return 0;
}
int ptzControl(int nChannel, PTZ_CMD cmd)
{
	pthread_t ptz_ctrl_threadid;
	int ret;
	ptzInit();
	ret = pthread_create(&ptz_ctrl_threadid, NULL, (void *)ptzctrl, NULL);
	if(ret)
	{
		return -1;
	}
	return 0;
}
int ptzStart()
{
	switch(cmd.nCmd)
	{
		case UP_START:
			//printf("UP\n");
			ptzUpStart();
			break;
		case UP_STOP:
			printf("stop\nstop\nstop\nstop\nstop\n");
			ptzVerStop();
			ptzHorStop();
			break;
		case DOWN_START:
			//printf("DOWN\n");
			ptzDownStart();
			break;
		case DOWN_STOP:
			ptzVerStop();
			break;
		case LEFT_START:
			//printf("LEFT\n");
			ptzLeftStart();
			break;
		case LEFT_STOP:
			ptzHorStop();
			break;
		case RIGHT_START:
			printf("RIGHT\n");
			ptzRightStart();
			break;
		case RIGHT_STOP:
			ptzHorStop();
			break;
		case ROTATION_START:
			ptzRotationStart();
			break;
		case ROTATION_STOP:
			ptzHorStop();
			break;
		case SET_POINT:
			ptzSetPoint(cmd.nValue);
			break;
		case GOTO_POINT:
			ptzGotoPoint(cmd.nValue);
			break;
		case CLEAR_POINT:
			ptzClearPoint(cmd.nValue);
			break;
		default:
			break;
	}
}


#if 1

int getPtzRs485Com(int nChannel, COM_PARAM *param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	memcpy(param, &g_ptz_ctrl_info[nChannel].param, sizeof(COM_PARAM));
	
	return 0;
}

int setPtzRs485Com(int nChannel, COM_PARAM param)
{
	int ret = -1;

	
	ret = COM_Setup(g_rs485_fd, param);
	if (ret < 0)
	{
		//printf("COM_Setup() Failed!\n");
	}
	
	memcpy(&g_ptz_ctrl_info[nChannel].param, &param, sizeof(COM_PARAM));
	
	return 0;
}

int getPtzRate(int nChannel, int *param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	*param = g_ptz_ctrl_info[nChannel].rate;
	
	return 0;
}

int getPtzRs232Com(int nChannel, COM_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	return 0;
}

int setPtzRs232Com(int nChannel, COM_PARAM param)
{
	int ret = -1;
	
	ret = COM_Setup(g_rs232_fd, param);
	if (ret < 0)
	{
		//printf("COM_Setup() Failed!\n");
	}
	
	return 0;
}

int ptzControlThrough(int nChannel, int nComType, PTZCTRLCMD_THROUGH cmd)
{
	int i = 0;
	int fd = -1;
	int ret = -1;
	COM_PARAM param;
	
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	
	if (nComType == 0)
	{
		fd = g_rs485_fd;	
	}
	else
	{
		fd = g_rs232_fd;
	}
	
	if (fd < 0)
	{
		return -1;
	}
	
	ret = COM_Setup(fd, cmd.param);
	if (ret < 0)
	{
		return -1;
	}
	
	if (cmd.nCmdLen > MAX_CMD_SIZE)
	{
		return -1;
	}
	
	ret = COM_Send(fd, cmd.cmd, cmd.nCmdLen);
	if (ret < 0)
	{
		return -1;
	}
	
	printf("PTZ BYPASS(%d):", cmd.nCmdLen);
	for (i=0; i<cmd.nCmdLen; i++)
	{
		printf(" %02x", cmd.cmd[i]);
	}
	printf("\n");
	
	memset(&param, 0, sizeof(COM_PARAM));
	if (nComType == 0)
	{
		getPtzRs485Com(nChannel, &param);
	}
	else
	{
		getPtzRs232Com(nChannel, &param);
	}
	
	COM_Setup(fd, cmd.param);
	
	return 0;
}

int ptzControlThroughExt(int nChannel, int nComType, PTZCTRLCMD_THROUGH cmd)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	
	memcpy(&g_ptz_bypass_send, &cmd, sizeof(PTZCTRLCMD_THROUGH));
	g_cur_ptz_cmd = BYPASS;
		
	sem_post(&g_ptz_sem);
	
	return 0;
}

// Add the code by lvjh, 2009-09-16
int setAutoPtz(int nChannel, PTZ_AUTO_CTRL param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	
	memcpy(&g_ptz_auto_ctrl[nChannel], &param, sizeof(PTZ_AUTO_CTRL));
	
	return 0;
}

int getAutoPtz(int nChannel, PTZ_AUTO_CTRL *param)
{
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	memcpy(&param, &g_ptz_auto_ctrl[nChannel], sizeof(PTZ_AUTO_CTRL));
	
	return 0;	
}

#if 1
int setChnPtzExt(int nChannel, char *ptzName, int addr, int rate)
{
	int ret = -1;
	unsigned int n = 0;
	PTZ_PARAM param;
	
	printf("setChnPtzExt(%d %s %d %d)\n", nChannel, ptzName, addr, rate);
	
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	if (ptzName == NULL)
	{
		return -1;
	}
	if (addr<0 || rate<0)
	{
		return -1;
	}
	
	while (n < g_ptz_index.nNum)
	{
		ret = strcmp(g_ptz_index.strName[n], ptzName);
		if (ret == 0)
		{
			//setChnPtz(nChannel, n);
			g_ptz_ctrl_info[nChannel].addr = addr;
			ret = ptzCtrlInit(&g_ptz_ctrl_info[nChannel], ptzName, rate);
			if (!ret)
			{
				param.nAddr = addr;
				param.nRate = rate;
				strcpy(param.strName, ptzName);
				setPtzParam(nChannel, &param);
				g_ptz_init_flag[nChannel] = 1;
			}
			
			printf("ptzCtrlInit(%d)(%d %d %d %d %d)\n", ret, g_ptz_ctrl_info[nChannel].param.nBaudRate, g_ptz_ctrl_info[nChannel].param.nDataBits, g_ptz_ctrl_info[nChannel].param.nParity, g_ptz_ctrl_info[nChannel].param.nStopBits, g_ptz_ctrl_info[nChannel].param.nFlowCtrl);
			
			return ret;
		}
		
		n++;
	}
	
	return -1;
}

int getChnPtzExt(int nChannel, char *ptzName, int *addr, int *rate)
{
	int ret = -1;
	unsigned int n = 0;
	
	if (nChannel<0 || nChannel>=4)
	{
		return -1;
	}
	if (ptzName == NULL)
	{
		return -1;
	}
	
	strcpy(ptzName, g_ptz_ctrl_info[nChannel].name);
	*addr = g_ptz_ctrl_info[nChannel].addr;
	*rate = g_ptz_ctrl_info[nChannel].rate;
	return 0;

}

int getPtzList(PTZ_INDEX *list)
{
	if (list == NULL)
	{
		return -1;
	}
	else
	{
		memcpy(list, &g_ptz_index, sizeof(PTZ_INDEX));
		return 0;
	}

}

int addPtz(char *ptzName, char *fileData, int size)
{
	FILE *fp = NULL;
	char path[128];
	
	if (ptzName==NULL || fileData==NULL || size<=0)
	{
		return -1;
	}
	if (g_ptz_index.nNum > MAX_PTZ_NUM)
	{
		return -1;
	}
	if (strstr(ptzName, ".cod") == NULL)
	{
		return -1;
	}
	
	sprintf(path, "/mnt/mtd/dvs/cod/%s", ptzName);

	fp = fopen(path, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	fwrite(fileData, 1, size, fp);
	
	fclose(fp);
	
	ptzInit();

}

int deletePtz(char *ptzName)
{
	int ret = -1;
	int flag = 0;
	char path[128];
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	
	if (ptzName==NULL)
	{
		return -1;
	}
	if (strstr(ptzName, ".cod") == NULL)
	{
		return -1;
	}

	dir = opendir("/mnt/mtd/dvs/cod");

	if (dir == NULL)
	{
		return -1;
	}
	
	while (entry=readdir(dir))
	{
		ret = strcmp(entry->d_name, ptzName);
		if (ret == 0)
		{
			flag = 1;
			break;
		}
	}
	
	closedir(dir);
	
	if (flag)
	{
		sprintf(path, "/mnt/mtd/dvs/cod/%s", ptzName);
	
		remove(path);
		ptzInit();
		
		return 0;
	}
	else
	{
		return -1;
	}
}
#endif

int ptzCtrlInit(PTZCTRLINFO *info, char *ptzName, int rate)
{
	int ret = -1;
	struct stat st;
	char path[128];
	char *buffer = NULL;
	char ptzCmd[64];
	int buffSize = 0;
	FILE *fp = NULL;
	
	if (info==NULL || ptzName==NULL)
	{
		printf("ptzCtrlInit(): invalid param!\n");
	
		return -1;
	}

	sprintf(path, "/mnt/mtd/dvs/cod/%s", ptzName);

	fp = fopen(path, "r+b");
	if (fp == NULL)
	{
		printf("Can not open the file: %s\n", path);
		return -1;
	}
	
	//ret = fstat(fp, &st);
	ret = stat(path, &st);
	if (ret)
	{
		printf("ptzCtrlInit(): Can not get ptz file attribute!\n");
		fclose(fp);
		return -1;
	}
	if (st.st_size <= 0)
	{
		printf("ptzCtrlInit(): ptz file size too small!\n");
		fclose(fp);
		return -1;
	}
	
	buffSize = st.st_size;
	buffer = (char *)malloc(buffSize);
	if (buffer == NULL)
	{
		printf("ptzCtrlInit(): malloc failed!\n");
		
		fclose(fp);
		
		return -1;
	}
	
	fread(buffer, 1, buffSize, fp);
	fclose(fp);
	
	strcpy(info->name, ptzName);
	info->rate = rate;

	// 获取串口参数
	ret = getPtzComParam(buffer, buffSize, &info->param.nBaudRate, &info->param.nDataBits, &info->param.nStopBits, &info->param.nParity);
	if (ret < 0)
	{
		free(buffer);
		
		return -1;
	}
	
	// Add the code by lvjh, 2010-05-08
	getPtzVerify(buffer, buffSize, &info->sum_start_bit, &info->sum_end_bit);
	
	// 获取PTZ控制命令
	ret = getPtzCmd(buffer, buffSize, info->addr, "Up_Start", rate, ptzCmd, 64); // UP
	if (ret > 0)
	{
		info->ptzCmd[0].nCmdLen = ret;
		memcpy(info->ptzCmd[0].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Up_Stop", rate, ptzCmd, 64);
	
	if (ret > 0)
	{
		info->ptzCmd[1].nCmdLen = ret;
		memcpy(info->ptzCmd[1].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Down_Start", rate, ptzCmd, 64); // DOWN
	if (ret > 0)
	{
		info->ptzCmd[2].nCmdLen = ret;
		memcpy(info->ptzCmd[2].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Down_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[3].nCmdLen = ret;
		memcpy(info->ptzCmd[3].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Left_Start", rate, ptzCmd, 64); // LEFT
	if (ret > 0)
	{
		info->ptzCmd[4].nCmdLen = ret;
		memcpy(info->ptzCmd[4].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Left_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[5].nCmdLen = ret;
		memcpy(info->ptzCmd[5].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Right_Start", rate, ptzCmd, 64); // RIGHT
	if (ret > 0)
	{
		info->ptzCmd[6].nCmdLen = ret;
		memcpy(info->ptzCmd[6].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Right_Stop", rate, ptzCmd, 64);
	
	if (ret > 0)
	{
		info->ptzCmd[7].nCmdLen = ret;
		memcpy(info->ptzCmd[7].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Rotation_Start", rate, ptzCmd, 64); // Rotation
	if (ret > 0)
	{
		info->ptzCmd[8].nCmdLen = ret;
		memcpy(info->ptzCmd[8].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Rotation_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[9].nCmdLen = ret;
		memcpy(info->ptzCmd[9].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "AADD_Start", rate, ptzCmd, 64); // AADD
	if (ret > 0)
	{
		info->ptzCmd[10].nCmdLen = ret;
		memcpy(info->ptzCmd[10].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "AADD_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[11].nCmdLen = ret;
		memcpy(info->ptzCmd[11].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "ADEC_Start", rate, ptzCmd, 64); // ADEC
	if (ret > 0)
	{
		info->ptzCmd[12].nCmdLen = ret;
		memcpy(info->ptzCmd[12].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "ADEC_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[13].nCmdLen = ret;
		memcpy(info->ptzCmd[13].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "FADD_Start", rate, ptzCmd, 64); // FADD
	if (ret > 0)
	{
		info->ptzCmd[14].nCmdLen = ret;
		memcpy(info->ptzCmd[14].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "FADD_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[15].nCmdLen = ret;
		memcpy(info->ptzCmd[15].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "FDEC_Start", rate, ptzCmd, 64); // FDEC
	if (ret > 0)
	{
		info->ptzCmd[16].nCmdLen = ret;
		memcpy(info->ptzCmd[16].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "FDEC_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[17].nCmdLen = ret;
		memcpy(info->ptzCmd[17].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "DADD_Start", rate, ptzCmd, 64); // DADD
	if (ret > 0)
	{
		info->ptzCmd[18].nCmdLen = ret;
		memcpy(info->ptzCmd[18].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "DADD_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[19].nCmdLen = ret;
		memcpy(info->ptzCmd[19].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "DDEC_Start", rate, ptzCmd, 64); // DDEC
	if (ret > 0)
	{
		info->ptzCmd[20].nCmdLen = ret;
		memcpy(info->ptzCmd[20].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "DDEC_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[21].nCmdLen = ret;
		memcpy(info->ptzCmd[21].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Light_Start", rate, ptzCmd, 64); // Light
	if (ret > 0)
	{
		info->ptzCmd[22].nCmdLen = ret;
		memcpy(info->ptzCmd[22].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Light_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[23].nCmdLen = ret;
		memcpy(info->ptzCmd[23].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Rain_Start", rate, ptzCmd, 64); // Rain
	if (ret > 0)
	
	{
		info->ptzCmd[24].nCmdLen = ret;
		memcpy(info->ptzCmd[24].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Rain_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[25].nCmdLen = ret;
		memcpy(info->ptzCmd[25].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Track_Start", rate, ptzCmd, 64); // Track
	if (ret > 0)
	
	{
		info->ptzCmd[26].nCmdLen = ret;
		memcpy(info->ptzCmd[26].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "Track_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[27].nCmdLen = ret;
		memcpy(info->ptzCmd[27].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "IO_Start", rate, ptzCmd, 64); // IO
	if (ret > 0)
	{
		info->ptzCmd[28].nCmdLen = ret;
		memcpy(info->ptzCmd[28].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "IO_Stop", rate, ptzCmd, 64);
	if (ret > 0)
	{
		info->ptzCmd[29].nCmdLen = ret;
		memcpy(info->ptzCmd[29].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "SetPoint", rate, ptzCmd, 64); // SetPoint
	if (ret > 0)
	{
		info->ptzCmd[30].nCmdLen = ret;
		memcpy(info->ptzCmd[30].cmd, ptzCmd, ret);
		info->checksum = g_ptz_checksum;
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "GotoPoint", rate, ptzCmd, 64); // GotoPoint
	if (ret > 0)
	{
		info->ptzCmd[31].nCmdLen = ret;
		memcpy(info->ptzCmd[31].cmd, ptzCmd, ret);
	}
	ret = getPtzCmd(buffer, buffSize, info->addr, "ClearPoint", rate, ptzCmd, 64); // ClearPoint
	if (ret > 0)
	{
		info->ptzCmd[32].nCmdLen = ret;
		memcpy(info->ptzCmd[32].cmd, ptzCmd, ret);
	}
	
	if (buffer)
	{
		free(buffer);
	}
	
	// Add the code by lvjh, 200805-10
	ret = COM_Setup(g_rs485_fd, info->param);
	if (ret < 0)
	{
		printf("COM_Setup(RS485) Failed!\n");
	}	
	
	return 0;
}
static int getPtzCmd(char *buffer, int size, int codAddr, char *lpCmd, int dwValue, char *lpOut, int nOutSize)
{
	char csAddress[16];
	int  data = 0;
	char Address_buf[128];
	char Command_buf[128];
	int  Address_count = 0;
	int nPos = 0;
	char *lpcsAddress = NULL;
	int nAddressPos = 0;
	char *Temp = NULL;
	char *ss   = NULL;
	char *Info = NULL;
	int TempList_count = 0;
	char *TempList[32];
	
	char s[32];
	int  Command_count = 0;
	int  Value = 0;
	int	 nSum = 0;
	char bMod = 0;
	char bDec = 0;
	char bMask = 0xFF;
	char bWait  = 20;
	char bWaitThread = 0;
	int nCommandPos = 0;
	char *lpCommand = NULL;
	int j = 0;
	
	if (buffer==NULL || size<=0 || codAddr<0)
	{
		return -1;
	}
	if (lpCmd==NULL || lpOut==NULL)
	{
		return -1;
	}
	
	sprintf(csAddress, "Address%02d", codAddr);

	//查找地址对应的数字
	nAddressPos = FindData(buffer, size, csAddress, strlen(csAddress));
	if (nAddressPos == 0)
	{
		return 0;
	}
	
	lpcsAddress = buffer + nAddressPos + strlen(csAddress) + 1;
	while (lpcsAddress[0] > 0x20)
	{
		if (lpcsAddress[0] == '[')
			sscanf(lpcsAddress,"[%x]",&data);
		else
			sscanf(lpcsAddress, "%x", &data);
			
		Address_buf[Address_count] = data;
		Address_count ++;
		
		while (1)
		{
			if((lpcsAddress[0]) < 0x20)
				break;
				
			if(lpcsAddress[0] == ',')
			{
				lpcsAddress++;
				break;
			}
			
			lpcsAddress++;
		}
	}
	
	// 查找命令
	nCommandPos = FindData(buffer, size, lpCmd, strlen(lpCmd));
	if (nCommandPos == 0)
	{
		return 0;
	}
	lpCommand = buffer + nCommandPos + strlen(lpCmd) + 1;
	if (lpCommand[0] < 0x20)
	{
		return 0;
	}
	while (lpCommand[0] > 0x20)
	{
		char* Info = lpCommand;
		
		if (Info[0] == '%')
		{
			//取模
			sscanf(lpCommand + 1, "%x", &Value);
			bMod = Value;
		}
		else if(Info[0] == '~')
		{
			//减处理(减校验和)
			sscanf(Info+1, "%x", &Value);
			bDec = Value;
		}
		else if(Info[0] == 'M')
		{
			//Mask处理(与操作)
			sscanf(Info+1, "%x", &Value);
			bMask = Value;
		}
		else if(Info[0] == '#')
		{
			//连续指令间隔时间
			sscanf(Info+1, "%x", &Value);
			bWait = Value;
		}
		else if(Info[0] == '&')
		{
			//连续发码间隔时间
			sscanf(Info+1, "%x", &Value);
			bWaitThread = Value;
		}
		else if(Info[0] == '^')
		{
			//
			for(j = 0; j < Address_count; j++)
			{
				Command_buf[Command_count] = Address_buf[j];
				Command_count++;
			}
		}
		//Up_Start=	%100,FF,^,00,08,H(0-3F),V(0-3F),+5
		else if(Info[0] == 'H' || Info[0] == 'V')
		{
			int nMin = 0;
			int nMax = 0;
			
			sscanf(Info+1, "(%x-%x)", &nMin,&nMax);
			
			if (dwValue > nMax)
				dwValue = nMax;
				
			if (dwValue < nMin)
				dwValue = nMin;
				
			Command_buf[Command_count++] = (char)dwValue;
		}
		else if(Info[0] == '!')
		{
			//云台预置点
			sscanf(Info+1, "%x", &Value);
			
			Command_buf[Command_count] = (char)dwValue + Value;
			
			Command_count++;
		}
		else if(Info[0] == '+')
		{
			g_ptz_checksum = Info[0];	// Add the code by lvjh, 2010-02-02
			
			//求校验和
			int TempList_count = 1;
			Temp = Info + 1;
			ss = strtok(Temp, ".");
			TempList[0] = ss;
			
			while((ss = strtok(NULL, ".") ) )
			{
				TempList[TempList_count] = ss;
				TempList_count ++;
			}
			
			//-----------------
			if (TempList_count == 1)
			{
				sscanf(TempList[0], "%x", &Value);
				nSum = 0;
				
				for (j=0; j<Value; j++)
				{
					if (Command_count - j - 1<0)
						return 0;
						
					nSum += Command_buf[Command_count - j - 1];
				}
				
				if (bMod != 0)
					Command_buf[Command_count] = nSum % bMod;
				else
					Command_buf[Command_count] = nSum;
					
				if (bMask != 0xFF)
					Command_buf[Command_count] &= bMask;
					
				if (bDec != 0)
					Command_buf[Command_count] = bDec - Command_buf[Command_count];
					
				Command_count++;
			}
			else
			{
				nSum = 0;
				
				for (j=0; j<TempList_count; j++)
				{
					Temp = TempList[j];
					
					if (Temp[strlen(Temp) - 1] == '+')
					{
						memcpy(s, Temp, strlen(Temp) - 1);
						sscanf(s, "%x", &Value);
						nSum += Value;
					}
					else
					{
						sscanf(TempList[j], "%x", &Value);
						
						if (Command_count - Value < 0)
							return 0;
							
						nSum += Command_buf[Command_count - Value];
					}
				}
				
				if (bMod!=0)
					Command_buf[Command_count] = nSum % bMod;
				else
					Command_buf[Command_count] = nSum;
					
				if (bMask!=0xFF)
					Command_buf[Command_count] &= bMask;
					
				if (bDec!=0)
					Command_buf[Command_count] = bDec - Command_buf[Command_count];
					
				Command_count++;
			}
		}
		else if(Info[0] == '-')
		{
			g_ptz_checksum = Info[0];	// Add the code by lvjh, 2010-02-02
			
			//求校验和(取反)
			Temp = Info + 1;
			ss = strtok(Temp, ",");
			TempList[0] = ss;
			TempList_count = 1;
			
			while ((ss = strtok(NULL, ",")))
			{
				TempList[TempList_count] = ss;
				TempList_count ++;
			}
			
			if (TempList_count == 1)
			{
				sscanf(TempList[0],"%x",&Value);
				nSum = 0;
				
				for (j=0; j<Value; j++)
				{
					if (Command_count - j - 1<0)
					{
						return 0;
					}
					
					nSum += Command_buf[Command_count - j - 1];
				}
				
				if (bMod != 0)
					Command_buf[Command_count] = ~(nSum%bMod);
				else
					Command_buf[Command_count] = ~(nSum);
					
				if (bMask != 0xFF)
					Command_buf[Command_count] &= bMask;
					
				if (bDec != 0)
					Command_buf[Command_count] = bDec - Command_buf[Command_count];
					
				Command_count++;
			}
			else
			{
				nSum = 0;
				
				for(j=0; j<TempList_count; j++)
				{
					sscanf(TempList[j], "%x", &Value);
					
					if(Command_count - Value < 0)
					{
						return 0;
					}
					
					nSum += Command_buf[Command_count - Value];
				}
				
				if (bMod != 0)
					Command_buf[Command_count] = ~(nSum%bMod);
				else
					Command_buf[Command_count] = ~(nSum);
					
				if (bMask != 0xFF)
					Command_buf[Command_count] &= bMask;
					
				if (bDec != 0)
					Command_buf[Command_count] = bDec - Command_buf[Command_count];
					
				Command_count++;
			}
		}
		else if(Info[0] == '=')
		{
			g_ptz_checksum = Info[0];	// Add the code by lvjh, 2010-02-02
			
			//求异或校验值
			int xorIndex = 0;
			int bXor;
			
			Temp = Info + 1;
			sscanf(Temp, "%x", &Value);
			
			if (Command_count - Value < 0)
			{
				return 0;
			}
			
			xorIndex = Command_count - Value;
			bXor = Command_buf[xorIndex];
			
			for (j=1; j<Value; j++)
			{
				xorIndex ++;
				
				if (xorIndex >= Command_count)
				{
					return 0;
				}
				
				bXor ^= Command_buf[xorIndex];
			}
			
			Command_buf[Command_count] = bXor;
			Command_count++;
		}
		else if( strlen(Info) >=3 && Info[0] =='[' && Info[strlen(Info) - 1]==']')
		{
			memcpy(s, Info +1, strlen(Info) - 2);
			
			if (strstr(s, "STX") !=0)
			{
				Value = 0x02;
			}
			else if(strstr(s, "ETX") !=0)
			{
				Value = 0x03;
			}
			else
			{
				char temp = (char)Value;
				sscanf(s,"%c",&temp);
				Value = temp;
			}
			
			Command_buf[Command_count] = Value;
			Command_count++;
		}
		else
		{		
			//直接转换成16
			sscanf(Info, "%x", &Value);
			Command_buf[Command_count] = Value;
			Command_count++;
		}
		
		if (Command_count > 126)
			break;
			
		while (1)
		{
			if ((lpCommand[0]) < 0x20)
				break;
				
			if (lpCommand[0] == ',')
			{
				lpCommand++;
				break;
			}
			
			lpCommand++;
		}
	}
	
	if (lpOut && nOutSize > Command_count)
	{
		memcpy(lpOut, Command_buf, Command_count);
		
		return Command_count;
	}
	
	return 0;
}

static int FindData(char *pInData, int dwLen, char *pDec, int dwDecLen)
{
	int i = 0;
	int nsame = 0;
	
	for (i=0; i<dwLen-dwDecLen; i++)
	{
		if (pInData[i] == pDec[0])
		{
			char *pver = (char *)pInData + i;
			
			while (nsame < dwDecLen)
			{
				nsame ++;
				
				if (pInData[i + nsame] != pDec[nsame])
					break;
			}
			
			if (nsame != dwDecLen)
			{
				nsame = 0;
				i ++;
			}
			else
			{
				return i;
			}
		}
	}
	
	return 0;
}

static int getPtzComParam(char *buffer, int size, int *nBaudRate, int *nDataBits, int *nStopBits, int *nParity)
{
	int offset = 0;
	int cur_offset = 0;	
	int cur_size = 0;
	char *temp = NULL;
	char *data = NULL;	
	
	*nBaudRate = 0;
	*nDataBits = 0;
	*nStopBits = 0;
	*nParity = 0;
	
	if (buffer==NULL || nBaudRate==NULL || nDataBits==NULL || nStopBits==NULL || nParity==NULL || size<=0)
	{	
		return -1;
	}
	
	temp = buffer;
	offset = FindData(temp, size, "[Comm]", 6);
	
	if (offset == 0)
	{
		return -1;
	}
	
	cur_offset = offset + 6;
	cur_size = size - cur_offset;
	offset = FindData(temp+cur_offset, cur_size, "StopBit", 7);
	
	if (offset)
	{
		data = temp + cur_offset + offset + 1 + 7;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*nStopBits = 0;
		}
		else
		{
			*nStopBits = atoi(data);
		}
	}
	
	offset = FindData(temp+cur_offset, cur_size, "DataBit", 7);
	
	if (offset)
	{
		data = temp + cur_offset + offset + 1 + 7;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*nDataBits = 0;
		}
		else
		{
			*nDataBits = atoi(data);
		}
	}
	
	offset = FindData(temp+cur_offset, cur_size, "Parity", 6);
	
	if (offset)
	{
		data = temp + cur_offset + offset + 1 + 6;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*nParity = 0;
		}
		else
		{
			*nParity = atoi(data);
		}
	}
	
	offset = FindData(temp+cur_offset, cur_size, "Baud", 4);
	if (offset)
	{
		data =  temp + cur_offset + offset + 1 + 4;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*nBaudRate = 0;
		}
		else
		{
			*nBaudRate = atoi(data);
		}
	}
	
	return 0;
}

static int getPtzVerify(char *buffer, int size, int *startbit, int *stopbit)
{
	int offset = 0;
	int cur_offset = 0;	
	int cur_size = 0;
	char *temp = NULL;
	char *data = NULL;	
	
	*startbit = 0;
	*stopbit = 0;
	
	if (buffer==NULL || startbit==NULL || stopbit==NULL || size<=0)
	{	
		return -1;
	}
	
	temp = buffer;
	offset = FindData(temp, size, "[Verify]", 6);
	
	if (offset == 0)
	{
		return -1;
	}
	
	cur_offset = offset + 6;
	cur_size = size - cur_offset;
	offset = FindData(temp+cur_offset, cur_size, "Start_Bit", 9);
	
	if (offset)
	{
		data = temp + cur_offset + offset + 1 + 9;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*startbit = 0;
		}
		else
		{
			*startbit = atoi(data);
		}
	}
	
	offset = FindData(temp+cur_offset, cur_size, "End_Bit", 7);
	
	if (offset)
	{
		data = temp + cur_offset + offset + 1 + 7;
		
		if (strncmp(data, "NULL", 4) == 0)
		{
			*stopbit = 0;
		}
		else
		{
			*stopbit = atoi(data);
		}
	}
	
	return 0;
}

#endif



