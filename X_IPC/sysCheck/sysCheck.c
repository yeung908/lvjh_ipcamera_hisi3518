#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "sysCheck.h"

#define SYSTEM_CHECK_FILE		"/param/system.ini"

//标识系统是否正常升级
typedef struct BOOT_FLAG
{
	unsigned long nKernel;				// 内核是否正常: 0x55<正常>  0xAA<升级出错>
	unsigned long nRamDisk;				// RAMDISK是否正常: 0x55<正常>  0xAA<升级出错>
	unsigned long nApp;					// 应用程序是否正常: 0x55<正常>  0xAA<升级出错>
	unsigned long nAppRunFlag;			// 应用程序运行标志: 0x55<正常>  0xAA<升级出错>
	unsigned long nAppFailedCount;		// 应用程序运行失败次数:超过三次恢复正常启动
	unsigned long nAppRunCount;			// 运行次数
	unsigned long nReserve[4];			// 备用
}SYSTEM_STATUS;

// 全局变量
SYSTEM_STATUS g_system_status;

int SYSCHECK_Init()
{
	memset(&g_system_status, 0, sizeof(SYSTEM_STATUS));
	
	g_system_status.nKernel = KERNEL_OK;
	g_system_status.nRamDisk = SYS_OK;
	g_system_status.nApp = APP_OK;
	g_system_status.nAppRunFlag = 0;
	g_system_status.nAppFailedCount = 0;
	
	return 0;
}

int SYSCHECK_ReadFlash()
{
	int nRet = -1;
	FILE *fp = NULL;
	
	fp = fopen(SYSTEM_CHECK_FILE, "r+b");
	if (fp == NULL)
	{
		printf("Can not open file: %s\n", SYSTEM_CHECK_FILE);
		printf("Error: %d %s\n", errno, strerror(errno));
		
		memset(&g_system_status, 0, sizeof(SYSTEM_STATUS));
	
		g_system_status.nKernel = KERNEL_OK;
		g_system_status.nRamDisk = SYS_OK;
		g_system_status.nApp = APP_FAIL;
		g_system_status.nAppRunFlag = 0;
		g_system_status.nAppFailedCount = 0;
		
		fp = fopen(SYSTEM_CHECK_FILE, "w+b");
		if (fp != NULL)
		{
			nRet = fwrite(&g_system_status, 1, sizeof(SYSTEM_STATUS), fp);
			fflush(fp);
			fclose(fp);
		}
		else
		{
			printf("Can not create file: %s\n", SYSTEM_CHECK_FILE);	
			printf("Error: %d %s\n", errno, strerror(errno));
		}
		
		return 0;
	}
	
	nRet = fread(&g_system_status, 1, sizeof(SYSTEM_STATUS), fp);
	if (nRet != sizeof(SYSTEM_STATUS))
	{
		memset(&g_system_status, 0, sizeof(SYSTEM_STATUS));
	
		g_system_status.nKernel = KERNEL_OK;
		g_system_status.nRamDisk = SYS_OK;
		g_system_status.nApp = APP_FAIL;
		g_system_status.nAppRunFlag = 0;
		g_system_status.nAppFailedCount = 0;
		
		nRet = fwrite(&g_system_status, 1, sizeof(SYSTEM_STATUS), fp);
		
		fflush(fp);
		fclose(fp);
		
		return 0;
	}
	
	fflush(fp);
	fclose(fp);
	
	return 0;
}

int SYSCHECK_WriteFlash()
{
	int nRet = -1;
	FILE *fp = NULL;
	
	fp = fopen(SYSTEM_CHECK_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	nRet = fwrite(&g_system_status, 1, sizeof(SYSTEM_STATUS), fp);
	if (nRet != sizeof(SYSTEM_STATUS))
	{
		fclose(fp);
		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);
	
	return 0;
}

int SYSCHECK_GetKernetStatus()
{
	return g_system_status.nKernel;
}

int SYSCHECK_SetKernetStatus(unsigned long nStatus)
{
	g_system_status.nKernel = nStatus;
	
	return 0;
}

int SYSCHECK_GetRamdiskStatus()
{
	return g_system_status.nRamDisk;
}

int SYSCHECK_SetRamdiskStatus(unsigned long nStatus)
{
	g_system_status.nRamDisk = nStatus;
	
	return 0;
}

int SYSCHECK_GetAppStatus()
{
	return g_system_status.nApp;
}

int SYSCHECK_SetAppStatus(unsigned long nStatus)
{
	g_system_status.nApp = nStatus;
	
	return 0;
}

int SYSCHECK_GetAppRunFlag()
{
	return g_system_status.nAppRunFlag;
}

int SYSCHECK_SetAppRunFlag(unsigned long nFlag)
{
	g_system_status.nAppRunFlag = nFlag;
	
	return 0;
}

int SYSCHECK_GetAppFailedCount()
{
	return g_system_status.nAppFailedCount;
}

int SYSCHECK_SetAppFailedCount(unsigned long nCount)
{
	g_system_status.nAppFailedCount = nCount;
	
	return 0;
}

int SYSCHECK_GetAppUpdateMode()
{
	if (g_system_status.nApp == APP_FAIL)
	{
		return UPDATE_MODE;
	}
	if (g_system_status.nAppFailedCount >= 10)
	{
		return UPDATE_MODE;
	}
	
	return WATCH_MODE;		
}

int SYSCHECK_AddAppRunCount()
{
	g_system_status.nAppRunCount++;
	
	printf("nAppRunCount: %d\n", g_system_status.nAppRunCount);
	
	return 0;
}

int SYSCHECK_SetAppRunCount(unsigned long nCount)
{
	//printf("SYSCHECK_SetAppRunCount: %d %d\n", nCount, g_system_status.nAppRunCount);
	
	if (nCount == 0)
	{
		return 0;
	}
	else
	{
		if (g_system_status.nAppRunCount >= nCount)
		{
			g_system_status.nApp = APP_FAIL;
			g_system_status.nAppRunCount = 0;
			SYSCHECK_WriteFlash();
			
			printf("SYSCHECK_SetAppRunCount(%x): %d %d\n", g_system_status.nApp, nCount, g_system_status.nAppRunCount);
			
			return -1;
		}
	}
	
	return 0;
}

int SYSCHECK_ClearAppRunCount()
{	
	g_system_status.nAppRunCount = 0;
	
	return 0;
}