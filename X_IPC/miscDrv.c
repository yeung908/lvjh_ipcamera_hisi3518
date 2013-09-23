#include <stdlib.h>
#include <stdio.h>
#include<fcntl.h>
#include <pthread.h>
#include <linux/ioctl.h>                //mody by lv old:nothing

#include "miscDrv.h"
#define MAJOR_NUM 96
#define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, int*) //mody by lv old:nothing
#define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, int*) //mody by lv old:nothing

pthread_mutex_t g_misc_dev_mutex;

int g_misc_fd = -1;

int miscDrv_Load()
{
	int ret = -1;

	ret = pthread_mutex_init(&g_misc_dev_mutex, NULL);
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int miscDrv_Unload()
{
	return 0;
}

int miscDrv_Open()
{
	if (g_misc_fd > 0)
	{
		return 0;
	}

	g_misc_fd = open("/dev/tds_gpio", O_RDWR);
	if (g_misc_fd < 0)
	{
		printf("Can not open /dev/tds_gpio!!!\n");
		return -1;
	}
	else
	{
		printf("Open /dev/tds_gpio OK!!!\n");
	}
	return 0;
}

int miscDrv_Close()
{
	if (g_misc_fd > 0)
	{
		close(g_misc_fd);
	}
	
	g_misc_fd = -1;
	
	return 0;	
}

#if 0 //mody by lv was old
int miscDrv_GetProbeIn()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_PROBE_IN_STATUS;
		cmd.param.probeInStatus = 0;
	
		pthread_mutex_lock(&g_misc_dev_mutex);
	
		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));
		//printf("read g_misc_fd return  = %d\n", ret);
		

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.probeInStatus;
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
#endif

#if 1    //mody by lv was new
int miscDrv_GetProbeIn(GPIO_CMD *cmd)
{
	int ret = -1;
	GPIO_CMD cmd_driver;
	
	if (g_misc_fd > 0)
	{
		cmd_driver.opt = GPIO_GET_WIRELESS_ALARM_STATUS;
		cmd_driver.D0 = 0;
		cmd_driver.D1 = 0;
		cmd_driver.D2 = 0;
		cmd_driver.D3 = 0;
	
		pthread_mutex_lock(&g_misc_dev_mutex);
		ret = read(g_misc_fd, &cmd_driver, sizeof(GPIO_CMD));
		pthread_mutex_unlock(&g_misc_dev_mutex);
		if (!ret)
		{
			cmd->D0 = cmd_driver.D0;
			cmd->D1 = cmd_driver.D1;
			cmd->D2 = cmd_driver.D2;
			cmd->D3 = cmd_driver.D3;
			//printf("cmd_driver.D0 = %d cmd_driver.D1 = %d cmd_driver.D2 = %d cmd_driver.D3 = %d\n", cmd_driver.D0, cmd_driver.D1, cmd_driver.D2, cmd_driver.D3);
			ret = 8*cmd_driver.D3 +4*cmd_driver.D2+2*cmd_driver.D1+1*cmd_driver.D0;
			return ret;
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
#endif


int miscDrv_GetTalkRequest()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_TALK_REQUEST_STATUS;
		cmd.param.talkRequestStatus = 0;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.probeInStatus;
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


#if 0 //mody by lv was old
int miscDrv_SetProbeOut(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_PROBE_OUT_STATUS;
		cmd.param.probeOutStatus = status;

		//printf("miscDrv_SetProbeOut: %x\n", status);		

		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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
#endif

#if 1  //mody by lv was new
int miscDrv_SetProbeOut(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_ALRAM_OUT;
		cmd.param.alarmOutStatus = status;

		//printf("miscDrv_SetProbeOut: %x\n", status);		

		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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
#endif


int miscDrv_SetWatchDog(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_WATCH_DOG;
		cmd.param.watchDog = status;

		//printf("miscDrv_watchDog: %x\n", status);		

		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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

int miscDrv_GetReset()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_RESET_STATUS;
		cmd.param.resetStatus = 0;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.resetStatus;
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

int miscDrv_SetRs485(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_RS485_ENABLE;
		cmd.param.rs485Enalbe = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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

int miscDrv_SetIndicator(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_INDICATOR;
		cmd.param.indicatorStatus = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		//printf("miscDrv_SetIndicator: %d\n", ret);

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//printf("miscDrv_SetIndicator(%d): Failed!\n", status);

		return -1;
	}
}

int miscDrv_Authen()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_AUTH;
		//cmd.param.authCode = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_lock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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


int miscDrv_GetIrCutStatus()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_IR_CUT_STATUS;
		cmd.param.ircutInStatus = 0;
	
		pthread_mutex_lock(&g_misc_dev_mutex);
	
		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.ircutInStatus;
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

int miscDrv_SetIrCutOutStatus(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_IR_CUT_STATUS;
		cmd.param.ircutOutStatus = status;

		printf("miscDrv_IrCutOut: %x\n", status);		

		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
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


//mody by lv was ---start old:noghing 
int  miscDrv_GetWirenessAlarmStatus()
{
	int ret = -1;
	GPIO_CMD cmd;
	int WirenessAlarmStatus[4] = {0};
	unsigned int WirenessAlarmStatusCommand = 0;

	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_WIRELESS_ALARM_STATUS;
		cmd.D0 = 0;
		cmd.D1 = 0;
		cmd.D2 = 0;
		cmd.D3 = 0;
		pthread_mutex_lock(&g_misc_dev_mutex);
	
		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			WirenessAlarmStatus[0] = cmd.D0;
			WirenessAlarmStatus[1] = cmd.D1;
			WirenessAlarmStatus[2] = cmd.D2;
			WirenessAlarmStatus[3] = cmd.D3;
			WirenessAlarmStatusCommand = WirenessAlarmStatus[0]*8 + WirenessAlarmStatus[1]*4 + WirenessAlarmStatus[2]*2 + WirenessAlarmStatus[3]*1;

			//printf("D0 = %d: D1 = %d: D2 = %d: D3 = %d\n", WirenessAlarmStatus[0], WirenessAlarmStatus[1], WirenessAlarmStatus[2], WirenessAlarmStatus[3]);
			//if(WirenessAlarmStatusCommand)
			return 1;
		}
		else
		{
			return  -1;
		}
	}
	else
	{
		return -1;
	}
}


/*******无线报警学习设置 2 秒为学习过程，10高电平为擦除学习记录*********************/
int miscDrv_Set_Garrison_study(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_ALRAM_GARRISON_STATUS;
		cmd.param.alarmGarrisonStatus = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		//printf("miscDrv_SetIndicator: %d\n", ret);

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//printf("miscDrv_SetIndicator(%d): Failed!\n", status);

		return -1;
	}
}


#if 0
int miscDrv_Set_alarm_output(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_ALRAM_OUT;
		cmd.param.alarmOutStatus = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		//printf("miscDrv_SetIndicator: %d\n", ret);

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//printf("miscDrv_SetIndicator(%d): Failed!\n", status);

		return -1;
	}
}
#endif

int miscDrv_Set_alarm_output(int status)
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_SET_ALRAM_OUT;
		cmd.param.alarmOutStatus = status;
		
		pthread_mutex_lock(&g_misc_dev_mutex);

		ret = write(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		//printf("miscDrv_SetIndicator: %d\n", ret);

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//printf("miscDrv_SetIndicator(%d): Failed!\n", status);

		return -1;
	}
}
int miscDrv_DriverStepMotor(STEPEER_CONTROL_OPT num,int speed)
{
	STEPPER  s;
	memset(&s,0,sizeof(STEPPER));
	s.CmdID = num;
	//printf("--ZJ-DEBUH--SPEED:%d\n",speed);
	if(speed<0||speed>100) speed = 80;
	s.Speed = 7-(speed/20);
	pthread_mutex_lock(&g_misc_dev_mutex);
	ioctl(g_misc_fd, IOCTL_SET_MSG, &s);
	pthread_mutex_unlock(&g_misc_dev_mutex);
	return 0;
}
int miscDrv_DriverStepMotor_goto(STEPEER_CONTROL_OPT num,int speed,COORD *coord){
	STEPPER  s;
	int ret;
	memset(&s,0,sizeof(STEPPER));
	s.CmdID = num;
	s.Speed = speed;
	//printf("--ZJ-DEBUH--SPEED:%d\n",speed);
	
	if(speed<0||speed>100) speed = 80;
	s.Speed = 7-(speed/20);
	memcpy(&(s.coord),coord,sizeof(COORD));
	pthread_mutex_lock(&g_misc_dev_mutex);
	ret = ioctl(g_misc_fd, IOCTL_SET_MSG, &s);
	pthread_mutex_unlock(&g_misc_dev_mutex);
	if(ret)	return -1;
	else 		memcpy(coord,&s.coord,sizeof(COORD));
	return 0;
}

int miscDrv_Getlimit_value()
{
	int ret;
	GPIO_CMD cmd;
	cmd.opt = GPIO_GET_MOTOR_LIMIT_VALUE;
	cmd.param.motorLimitValue= 0;
	
	pthread_mutex_lock(&g_misc_dev_mutex);
	ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));
	pthread_mutex_unlock(&g_misc_dev_mutex);
	if(!ret){
		return cmd.param.motorLimitValue;
	}else{
		return -1;
	}
}

int miscDrv_GetIrCutValue()
{
	int ret = -1;
	GPIO_CMD cmd;
	
	if (g_misc_fd > 0)
	{
		cmd.opt = GPIO_GET_IR_CUT_VALUE;
		cmd.param.ircutInStatus = 0;
	
		pthread_mutex_lock(&g_misc_dev_mutex);
	
		ret = read(g_misc_fd, &cmd, sizeof(GPIO_CMD));

		pthread_mutex_unlock(&g_misc_dev_mutex);

		if (!ret)
		{
			return cmd.param.ircutInStatus;
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


int miscDrv_select_coord()
{
	int ret = -1;
	fd_set fset;

	if (g_misc_fd > 0)
	{
		FD_ZERO(&fset);
		FD_SET(g_misc_fd, &fset);


		ret = select(g_misc_fd+1,&fset,NULL,NULL,NULL);
		if(ret == 0){
			return 0;
		}else if(ret > 0){
			return 1;
		}else{
			return -1;
		}
	}
	else
	{
		return -1;
	}
}


