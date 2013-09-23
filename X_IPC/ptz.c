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
 
#include    <sys/ipc.h>  
#include    <sys/sem.h>  
#include    <errno.h> 



#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"
#include "session.h"
#include "com.h"
#include "param.h"
#include "miscDrv.h"

#include "ptz.h"
//#include "tv_gpio.h"
//云台电机中间位置坐标X: 542 , Y:146

#if 1
#define BYPASS 33
// 全局变量
int g_rs485_fd = -1;
int g_rs232_fd = -1;
PTZ_INDEX g_ptz_index;
int g_ptz_run_flag = 1;
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
int fd_h = -1;

// 云台解析
static PTZCTRLINFO g_ptz_ctrl_info[4];
static PTZ_AUTO_CTRL g_ptz_auto_ctrl[4];
#endif

int g_handle_up_flags = 0;
int g_handle_down_flags = 0;
int g_handle_right_flags = 0;
int g_handle_left_flags = 0;


int g_handle_flags = 0;

pthread_mutex_t ptc_conf_mutex;


#define ptz_conf_path "/param/ptz.conf"
#define PTZ_MOTOR_STEPEER	 "/dev/tds_gpio"
#define PTZ_RESET_VERTICAL_ANGLE 	594
#define PTZ_RESET_HORIZEN_ANGLE 	167


typedef struct _CURENT_MOTOR_STATE
{
	int angle;                    //以脉冲个数代表坐标水平方向0~256,顺时针+，逆时针-
	int run_flag;
	int direction;             // 水平运动方向:1向右 0向左垂直运动方向: 1向上 0向下
	int limit_stop;           //水平方向坐标范围:0~(64*64-57)    垂直方向坐标范围:0~(16*64-57)
}CURENT_MOTOR_STATE;

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
	ver_run_rotation=3,
	ver_run_goto=4,
	ver_run_init = 5,
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

int speed=0;



CURENT_MOTOR_STATE g_hor_state={0,0,0,64*64};
CURENT_MOTOR_STATE g_ver_state={0,0,0,16*64};
GOTO_OFFSET point_init;           //记录初始位置，以便自检后移动到该位置
/******************************************************/
/*功能:检测限位开关是否闭合
/*input:   LIMIT_VALUE,要检测的开关编号
/*output:0:未闭合1:闭合
/*
/*****************************************************/
int ptz_detection_limit(int num){
	int ret=0;
	ret = miscDrv_Getlimit_value();
	if(ret > 0)
	{
		switch(num){
			case value_up:
					ret = ret & value_up;
				break;
			case value_down:
					ret = ret & value_down;
				break;
			case value_left:
					ret = ret & value_left;
				break;
			case value_right:
					ret = ret & value_right;
				break;
			default:
				break;
		}
		if(ret)
			return 1;
		else
			return 0;
	}
	else{
		return 0;
	}
}
void ptzLeftUpStart()
{
	if(!ptz_detection_limit(value_up)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_UP,speed);
		
	}
	if(!ptz_detection_limit(value_left)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_LETF,speed);
		
	}

}
void ptzLeftDownStart()
{
	if(!ptz_detection_limit(value_down)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_DOWN,speed);
		
	}
	if(!ptz_detection_limit(value_left)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_LETF,speed);
	}
}
void ptzRightUpStart()
{
	if(!ptz_detection_limit(value_up)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_UP,speed);
	}
	if(!ptz_detection_limit(value_right)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_RIGHT,speed);	
	}

}
void ptzRightDownStart()
{
	if(!ptz_detection_limit(value_down)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_DOWN,speed);
	}
	if(!ptz_detection_limit(value_right)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_RIGHT,speed);
	}
}
void ptzVerRotationStart()
{
	miscDrv_DriverStepMotor(V_STEPEER_CONTROL_ROTATION,speed);
}
void ptzRightStart()
{
	if(!ptz_detection_limit(value_right)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_RIGHT,speed);
	}

}

void ptzLeftStart()
{
	if(!ptz_detection_limit(value_left)){
		miscDrv_DriverStepMotor(H_STEPEER_CONTROL_LETF,speed);
	}

}

void ptzUpStart()
{
	if(!ptz_detection_limit(value_up)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_UP,speed);
	}

			
}
void ptzDownStart()
{
	if(!ptz_detection_limit(value_down)){
		miscDrv_DriverStepMotor(V_STEPEER_CONTROL_DOWN,speed);
	}
		
}
void ptzMotorStop()
{
	miscDrv_DriverStepMotor(H_STEPEER_CONTROL_STOP,speed);
	miscDrv_DriverStepMotor(V_STEPEER_CONTROL_STOP,speed);
}
void ptzReset()
{
	//miscDrv_DriverStepMotor(HV_STEPEER_CONTROL_INIT,speed);
}
void ptzRotationStart()
{
	miscDrv_DriverStepMotor(H_STEPEER_CONTROL_ROTATION,speed);
}
void clear_point_list()
{	
	int i,ret;
	FILE *fp;
	pthread_mutex_lock(&ptc_conf_mutex);
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
	pthread_mutex_unlock(&ptc_conf_mutex);
}

int ptzInit()
{
	int ret;
	FILE *fp;
	ret = miscDrv_Open();
	if(ret < 0){
		printf("open stepeer motor error \n");
		return -1;
	}
	pthread_mutex_init(&ptc_conf_mutex, NULL);
	pthread_mutex_lock(&ptc_conf_mutex);
	memset(&ptz_conf,0,sizeof(PTZ_CONF));
	fp=fopen(ptz_conf_path,"rb+");
	if (fp == NULL)
	{	
		pthread_mutex_unlock(&ptc_conf_mutex);
		clear_point_list();
		pthread_mutex_lock(&ptc_conf_mutex);
		fp=fopen(ptz_conf_path,"rb+");
		if(fp == NULL)
		{
			printf("Open file %s fault\n",ptz_conf_path);
			pthread_mutex_unlock(&ptc_conf_mutex);
			return -1;
		}
		
	}
	ret = fread(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	if(ret != sizeof(PTZ_CONF))
	{
		printf("read ptz_conf error\n");
		fclose(fp);	
		pthread_mutex_unlock(&ptc_conf_mutex);
		return -1;
	}

	ret = sscanf(ptz_conf.speed,"speed=%d%[^'\0']",&speed);
	if(ret != 1) 
	{
		printf("speed haven't set yet\n");
		speed = 80;
		fclose(fp);
		pthread_mutex_unlock(&ptc_conf_mutex);
		return ;
	}
	fclose(fp);
	pthread_mutex_unlock(&ptc_conf_mutex);
	return 0;
}


void ptzSetPoint( unsigned char nPoint)
{
	int ret;
	FILE *fp;
	COORD coord;
	nPoint=(nPoint>32)?32:nPoint;
	nPoint=(nPoint<1)?1:nPoint;
	nPoint--;
	ret = miscDrv_DriverStepMotor_goto(HV_STEPEER_CONTROL_GET_COORD,speed,&coord);
	//printf("Set point X:%d\tY:%d\n",coord.x,coord.y);
	if(ret){ 
		return;
	}
	pthread_mutex_lock(&ptc_conf_mutex);
	memset(&(ptz_conf.point[nPoint]),0,24);
	sprintf(ptz_conf.point[nPoint],"point%d=%d,%d",nPoint,coord.x,coord.y);
	
	fp=fopen(ptz_conf_path,"wb+");
	if(fp == NULL){ 
		pthread_mutex_unlock(&ptc_conf_mutex);
		return;
	}
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
	pthread_mutex_unlock(&ptc_conf_mutex);
}

void ptzGotoPoint( unsigned char nPoint)
{
	int ret;
	COORD coord;
	memset(&coord,0,sizeof(COORD));
	
	nPoint=(nPoint>32)?32:nPoint;
	nPoint=(nPoint<1)?1:nPoint;
	nPoint--;
	pthread_mutex_lock(&ptc_conf_mutex);
	ret =sscanf(ptz_conf.point[nPoint],"point%*[^=]=%d,%d%[^'\0']",&coord.x,&coord.y);
	pthread_mutex_unlock(&ptc_conf_mutex);
	if (ret != 2)
	{
		printf("you haven't set %d point yet.\n", nPoint);
		return;
	}
	printf("Goto point X:%d\tY:%d\n",coord.x,coord.y);
	speed = 60;
	ret = miscDrv_DriverStepMotor_goto(HV_STEPEER_CONTROL_GOTO,speed,&coord);
	if(ret) return;
	
}

void ptzClearPoint( unsigned char nPoint)
{
	int ret;
	FILE *fp;
	nPoint = (nPoint>32)?32:nPoint;
	nPoint = (nPoint<1)?1:nPoint;
	nPoint--;

	pthread_mutex_lock(&ptc_conf_mutex);
	memset(ptz_conf.point[nPoint],0,sizeof(ptz_conf.point[nPoint]));
	sprintf(ptz_conf.point[nPoint],"point%d=",nPoint);
	fp=fopen(ptz_conf_path,"wb+");
	if(fp == NULL){ 
		pthread_mutex_unlock(&ptc_conf_mutex);
		return;
	}
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
	pthread_mutex_unlock(&ptc_conf_mutex);
}

void ptzSetSpeed(int speed)
{
	FILE *fp;
	int ret;
	pthread_mutex_lock(&ptc_conf_mutex);
	sprintf(ptz_conf.speed,"speed=%d",speed);
	
	fp=fopen(ptz_conf_path,"wb+");
	if(fp == NULL){ 
		pthread_mutex_unlock(&ptc_conf_mutex);
		return;
	}
	ret = fwrite(&ptz_conf,1,sizeof(PTZ_CONF),fp);
	fclose(fp);
	pthread_mutex_unlock(&ptc_conf_mutex);
}

int ptzAutoCtrlFun()
{
	int i = 0;
	int ret = 0;
	int nTime[4] = {0, 0, 0, 0};
	
	while (g_ptz_run_flag)
	{
		if (g_ptz_pause_flag)
		{
			sleep(1);
			continue;
		}
		ret = miscDrv_select_coord();
		if(ret >= 0){
			//printf("set current coord---\n");
			ptzSetPoint(32);
		}
		
		for (i=0; i<4; i++)
		{
		
			//printf("Time: %d %d\n", i, nTime[i]);
			
			if (g_ptz_auto_ctrl[i].bEnabled==0 
				|| g_ptz_auto_ctrl[i].nAutoTimeLen<1 
				|| g_ptz_auto_flag[i] == 0)
			{
				nTime[i] = 0;
				
				continue;
			}
			
			nTime[i]++;
			
			if (nTime[i] >= g_ptz_auto_ctrl[i].nAutoTimeLen*60)
			{
				nTime[i] = 0;
				PTZ_AUTO_CTRL ptzAutoCtrl;
				char buffer[1024];
				DEV_CMD_RETURN devCmdReturn;
				
				g_cur_channel = i;
				g_cur_ptz_cmd = ROTATION_STOP;
				g_cur_ptz_value = 0;

				
				PTZ_CMD cmd;
				cmd.nCmd = ROTATION_STOP;
				ptzControl_zj(1,cmd);
				
				// 
				devCmdReturn.nCmdID = DEV_PTZ_AUTO_CTRL;
				devCmdReturn.nCmdLen = sizeof(PTZ_AUTO_CTRL);
				devCmdReturn.nResult = 0;
				devCmdReturn.nReserve = 0;
				
				memcpy(&ptzAutoCtrl, &g_ptz_auto_ctrl[i], sizeof(PTZ_AUTO_CTRL));
				ptzAutoCtrl.dwReserved = 0;
				
				memset(buffer, 0, 1024);
				memcpy(buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
				memcpy(buffer+sizeof(DEV_CMD_RETURN), &ptzAutoCtrl, sizeof(PTZ_AUTO_CTRL));
			
				NETSDK_SendAllMsg(&buffer, sizeof(DEV_CMD_RETURN)+sizeof(PTZ_AUTO_CTRL));

				printf("ptzControl: %d %d %d\n", i, g_cur_ptz_cmd, g_cur_ptz_value);

				sem_post(&g_ptz_sem);
			}
		}
	}
	
	return 0;
}

#if 1
#define    command_tmp_ptah     "/mnt/mtd/dvs/command.conf"
#define    IPC_PATH  "/mnt/mtd/dvs/mobile/cgi-bin"
int sem_id;  
void init()  
{  
    key_t key;  
    int ret;  
    unsigned short sem_array[2];  
    union semun  
    {  
        int val;  
        struct semid_ds *buf;  
        unsigned short *array;  
    }arg;  
    key = ftok(IPC_PATH, 's');                                            /*get key*/  
    sem_id = semget(key, 2, IPC_CREAT|0644);/*get semaphore include two sem*/  
      
    /*set sem's vaule*/  
    arg.val = 1; 
    ret = semctl(sem_id, 0, SETVAL, arg);  
    if(ret == -1)  
    {  
        printf("SETVAL failed (%d)\n", errno);  
    }  
    arg.val = 0; 
    ret = semctl(sem_id, 1, SETVAL, arg);  
    if(ret == -1)  
    {  
        printf("SETVAL failed (%d)\n", errno);  
    }
  //  printf("productor init is %d\n", semctl(sem_id, 0, GETVAL));  
  //  printf("consumer init is %d\n\n", semctl(sem_id, 1, GETVAL));  
}  
void del()  
{  
    semctl(sem_id, IPC_RMID, 0);  
}

int ptzCtrlCgiFun()
{
	PTZ_CMD cmd;
	int ret;
	FILE *fp;
	union semun  
	{  
		int val;  
		struct semid_ds *buf;  
		unsigned short *array;	
	}arg;  
#if 0
	ret = ptzStart();
	if(ret)
	{
		printf("PTZ START FAULT!\n");
		return -1;
	}
#endif	
	
	struct sembuf sops[2];	
	/*set operate way for semaphore*/  
	sops[0].sem_num = 0;  
	sops[0].sem_op = 1; 		//POST
	sops[0].sem_flg = SEM_UNDO;  
	sops[1].sem_num = 1;  
	sops[1].sem_op = -1;		 //WAIT  1
	sops[1].sem_flg = SEM_UNDO;  
	
//	printf("consumer init is %d\n\n", semctl(sem_id, 1, GETVAL));  
	
	init();  
	while(1)
	{
	//	printf("-----3----\n");
		if(semctl(sem_id, 1, GETVAL) > 1)
		{
				arg.val = 1; 
				ret = semctl(sem_id, 1, SETVAL, arg);  
				if(ret == -1)  
				{  
					printf("SETVAL failed (%d)\n", errno);	
				}
		}
		semop(sem_id, (struct sembuf*)&sops[1], 1);
		fp = fopen(command_tmp_ptah,"rb+");
		if(fp == NULL)
		{
			continue;
		}
		ret = fread(&cmd,1,sizeof(PTZ_CMD),fp);
		if(ret != sizeof(PTZ_CMD))
		{
			//sem_post(sem_cmd);
			continue;
		}
		//printf("cmd:%d\t nValue:%d\n",cmd.nCmd,cmd.nValue);
		ptzControl(1,cmd);
		memset(&cmd,0,sizeof(PTZ_CMD));
		fclose(fp);
		//semop(sem_id, (struct sembuf*)&sops[0], 1);  
		sleep(1);
	}
	return 0;
}
#endif



int ptzStart()
{
	int i = 0;
	pthread_t ptz_ctrl_threadid,threadID, threadIDcgi;
	int ret;
	ptzInit();

	getPtzAutoCtrlParam(i, &g_ptz_auto_ctrl[i]);

	speed = 80;
	ptzGotoPoint(32);

	ret = pthread_create(&threadID, NULL, (void *)ptzAutoCtrlFun, NULL);
	if (ret)
	{
		g_ptz_run_flag = 0;
		sem_destroy(&g_ptz_sem);
		
		return -1;
	}
	#if 1
	ret = pthread_create(&threadIDcgi, NULL, (void *)ptzCtrlCgiFun, NULL);
	if (ret)
	{
		g_ptz_run_flag = 0;
		sem_destroy(&g_ptz_sem);
		
		return -1;
	}
	#endif
	
	return 0;
}
int ptzControl_zj(int nChannel, PTZ_CMD cmd)
{
	long flip_flag = 0;
	long mirror_flag = 0;
	VIDEO_FLIP_PARAM param_flip;
	VIDEO_MIRROR_PARAM  param_mirror;
	getVideoFlipParam(0,&param_flip);
	getVideoMirrorParam(0,&param_mirror);
	flip_flag = param_flip.nFlip;
	mirror_flag = param_mirror.nMirror;
	//speed = cmd.nValue;
	//if(speed == 0) speed=2;
	speed = 5;
	//printf("PTZcontrol_zj CMD :%d --VALUE:%d\n",cmd.nCmd,cmd.nValue);
	switch(cmd.nCmd)
	{
		case DOWN_START:
			printf("UP\n");
			
			if(!flip_flag){
				ptzUpStart();
			}
			else{
				ptzDownStart();
			}
			break;
		case DOWN_STOP:
			printf("up stop\n");
			ptzMotorStop();
			break;
		case UP_START:
			printf("DOWN\n");
			
			if(!flip_flag){
				ptzDownStart();
			}
			else{
				ptzUpStart();
			}
			break;
		case UP_STOP:
			printf("down stop\n");
			ptzMotorStop();
			break;
		case LEFT_START:
			printf("LEFT\n");
			g_handle_flags = 1;

			#if 1
			if(mirror_flag){
				ptzLeftStart();
			}
			else{
				ptzRightStart();
			}
			#endif
			
			break;
		case LEFT_STOP:
			printf("letf stop\n");
			ptzMotorStop();
			break;
		case RIGHT_START:
			printf("RIGHT\n");

			#if 1
			if(mirror_flag){
				ptzRightStart();
			}
			else{
				ptzLeftStart();
			}
			#endif
			
			g_handle_flags = 1;
			break;
		case RIGHT_STOP:
			//printf("right stop\n");
			ptzMotorStop();
			break;
		case ROTATION_START:
			//printf("entry ROTATION_START\n");
			ptzRotationStart();
			break;
		case ROTATION_STOP:
			//printf("entry ROTATION_STOP\n");
			ptzMotorStop();
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
		case LEFTDOWN_START:
			if((!flip_flag) && (!mirror_flag)){
				ptzRightUpStart();
			}else if((flip_flag) && (!mirror_flag)){
				ptzRightDownStart();
			}else if((!flip_flag) && (mirror_flag)){
				ptzLeftUpStart();
			}else{
				ptzLeftDownStart();
			}
			
			break;
		case LEFTUP_START:
			if((!flip_flag) && (!mirror_flag)){
				ptzRightDownStart();
			}else if((flip_flag) && (!mirror_flag)){
				ptzRightUpStart();
			}else if((!flip_flag) && (mirror_flag)){
				ptzLeftDownStart();
			}else{
				ptzLeftUpStart();
			}
			break;
		case RIGHTDOWN_START:
			
			if((!flip_flag) && (!mirror_flag)){
				ptzLeftUpStart();
			}else if((flip_flag) && (!mirror_flag)){
				ptzLeftDownStart();
			}else if((!flip_flag) && (mirror_flag)){
				ptzRightUpStart();
			}else{
				ptzRightDownStart();
			}
			break;
		case RIGHTUP_START:
			
			if((!flip_flag) && (!mirror_flag)){
				ptzLeftDownStart();
			}else if((flip_flag) && (!mirror_flag)){
				ptzLeftUpStart();
			}else if((!flip_flag) && (mirror_flag)){
				ptzRightDownStart();
			}else{
				ptzRightUpStart();
			}
			break;
		case VERT_ROTATION_START:
			speed = 0;
			ptzVerRotationStart();
			break;
		case VERT_ROTATION_STOP:
			speed = 0;
			ptzMotorStop();
			break;
	
		case LEFTDOWN_STOP:
			ptzMotorStop();
			break;
		case LEFTUP_STOP:
			ptzMotorStop();
			break;
		case RIGHTDOWN_STOP:
			ptzMotorStop();
			break;
		case RIGHTUP_STOP:
			ptzMotorStop();
			break;
		default:
			break;
	}
}

int ptzControl(int nChannel, PTZ_CMD cmd)
{
	
	if (cmd.nCmd == ROTATION_START || cmd.nCmd == ROTATION_STOP)
	{
		PTZ_AUTO_CTRL ptzAutoCtrl;
		char buffer[1024];
		DEV_CMD_RETURN devCmdReturn;
				
		devCmdReturn.nCmdID = DEV_PTZ_AUTO_CTRL;
		devCmdReturn.nCmdLen = sizeof(PTZ_AUTO_CTRL);
		devCmdReturn.nResult = 0;
		devCmdReturn.nReserve = 0;
				
		memcpy(&ptzAutoCtrl, &g_ptz_auto_ctrl[nChannel], sizeof(PTZ_AUTO_CTRL));
		if (cmd.nCmd == ROTATION_START)
		{
			ptzAutoCtrl.dwReserved = 1;
			g_ptz_auto_flag[nChannel] = 1;
		}
		else
		{
			ptzAutoCtrl.dwReserved = 0;
			g_ptz_auto_flag[nChannel] = 0;
		}
				
		memset(buffer, 0, 1024);
		memcpy(buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
		memcpy(buffer+sizeof(DEV_CMD_RETURN), &ptzAutoCtrl, sizeof(PTZ_AUTO_CTRL));
			
		NETSDK_SendAllMsg(&buffer, sizeof(DEV_CMD_RETURN)+sizeof(PTZ_AUTO_CTRL));

		printf("ptzControl: %d %d %d\n", nChannel, g_cur_ptz_cmd, g_cur_ptz_value);
	}
	ptzControl_zj(nChannel,cmd);
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

 int getPtzCmd(char *buffer, int size, int codAddr, char *lpCmd, int dwValue, char *lpOut, int nOutSize)
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

 int FindData(char *pInData, int dwLen, char *pDec, int dwDecLen)
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

 int getPtzComParam(char *buffer, int size, int *nBaudRate, int *nDataBits, int *nStopBits, int *nParity)
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

int getPtzVerify(char *buffer, int size, int *startbit, int *stopbit)
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

