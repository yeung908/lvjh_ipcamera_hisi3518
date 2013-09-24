#include <stdlib.h>
#include <stdio.h>
#include<fcntl.h>
#include <pthread.h>

#include "ircut.h"
#include "miscDrv.h"
#include "vadcDrv.h"
#include "param.h"



// 全局变量
static int g_ircut_run = 0;
static int g_ircut_pause = 0;

int ircutModuleInit()
{
	miscDrv_Load();
	miscDrv_Open();

	return 0;
}

int ircutModuleDeInit()
{
	int ret = -1;

	g_ircut_run = 0;
	g_ircut_pause = 0;

	return 0;
}

#if 1
int ircutFun()
{
	int i = 0;
	int ret = -1;
	int ircutInStatus = 0;
	int ircutInStatusPast = 0;	//--------mody by lv  --old:-1
	int ircutInKeepFlag = 0;
	int sum = 0;				//--------mody by lv  --old:nothing
	tmVadc_Register pregister;
	VIDEO_IN_ATTR vinAttr;


	//printf("ircut fun \n");
	//sleep(15);

	sleep(10);
	while (g_ircut_run)
	{
		ircutInStatus = miscDrv_GetIrCutStatus();
		pregister.address=0x98;
		pregister.lowValue = 0;
		pregister.value = 0;
		pregister.page =0x01;
		ret = vadcDrv_Get_Register(&pregister);
		if(ret == 0)
		{
			//printf("REGISTER	high:0x%02x--low:0x%02x\n",pregister.value,pregister.lowValue);
			ret = miscDrv_GetIrCutValue();
			if(ret)
			{
				if(pregister.lowValue == 0x05)
					vadcDrv_SetColor();
			}
			else
			{
				if(pregister.lowValue == 0x00)
					vadcDrv_SetBack();

				//getVideoInAttrParam(0, &vinAttr);
				//vinAttr.nBrightness = 120;
				vadcDrv_SetBrightness(0, 120);
				//setVideoInAttrParam(0, &vinAttr);
			}
		}
		usleep(200000);

	}

	pthread_exit(NULL);

	return 0;
}

#endif



int ircutStart()
{
	int ret = -1;
	pthread_t threadID;

	ret = miscDrv_Open();
	if (ret < 0)
	{
		printf("miscDrv_Open: Failed!\n");
		return -1;
	}

	g_ircut_run = 1;

	ret = pthread_create(&threadID, NULL, (void *)ircutFun, NULL);
	if (ret < 0)
	{
		printf("ircutStart(): Failed!\n");

		g_ircut_run = 0;
		return -1;
	}

	printf("ircutStart(): Ok!\n");

	return 0;
}

int ircutStop()
{
	g_ircut_run = 0;

	return 0;
}

int ircutrPause()
{
	g_ircut_pause = 1;

	return 0;
}

int ircutResume()
{
	g_ircut_pause = 0;

	return 0;
}

