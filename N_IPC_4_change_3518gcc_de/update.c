#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "crc/crc.h"
#include "mtd/mtd.h"
#include "param.h"
#include "update.h"
#include "sysCheck/sysCheck.h"
#include "session.h"
#include "watchDog.h"

#define KERNEL_FLASH_DEVICE		"/dev/mtd/0"
#define SYSTEM_FLASH_DEVICE		"/dev/mtd/1"
#define APP_FLASH_DEVICE		"/dev/mtd/2"

int g_update_flag = 0;
int g_update_socket = 0;
char g_update_file[32];
int g_update_progress = 0;
struct sockaddr_in g_update_addr;
//int g_update_nflag;

int sendUpdateProgress(int nProgress)
{
	int nRet = -1;
	int nSocket = -1;
	UPDATE_RETURN updateRet;
	MSG_HEAD returnMsg;
	
	nSocket = g_update_socket;

	printf("readUpdateProcess: %d\n", nSocket);

	if (nSocket <= 0)
	{
		return -1;
	}
	
	// 初始化升级返回数据结构
	memset(&updateRet, 0, sizeof(UPDATE_RETURN));
	memset(&returnMsg, 0, sizeof(MSG_HEAD));
	
	memcpy(updateRet.fileName, g_update_file, 32);
	
	returnMsg.nCmd = NETCMD_UPDATE_PROCESS;	 
	returnMsg.nSock = nSocket;
	returnMsg.addr = g_update_addr;
	returnMsg.nflag = g_server_info.nupdate_flag;
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);
	updateRet.nResult = APP_UPDATE_START;
	
	if (g_update_progress != nProgress)
	{
		if(g_server_info.nupdate_flag == UDP_FLAG)
		updateRet.nProgress = nProgress + 1;
		else 
			updateRet.nProgress = nProgress;
		g_update_progress = nProgress;

		NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
				
		printf("Update(%d): %d%%!\n", returnMsg.nSock, updateRet.nProgress);
	}
	
	return 0;
}

int SaveFile(char *fileName, char *data, int size)
{
	int nRet = -1;
	FILE *fp = NULL;
	
	printf("SaveFile: %s %d\n", fileName, size);
	
	if (fileName==NULL || data==NULL || size<=0)
	{
		return -1;
	}
	
	remove(fileName);
	
	fp = fopen(fileName, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the update file(%s)!\n", fileName);
		
		return -1;
	}
	
	nRet = fwrite(data, 1, size, fp);
	if (nRet != size)
	{
		printf("Can not write the update file(%s): %d!\n", fileName, nRet);
		
		fclose(fp);
		return -1;
	}
	
	fflush(fp);
	fclose(fp);

	chmod(fileName, 0777);
	
	return 0;
}

char *FindStr(char *str, char *dst)
{
	int nLen = 0;
	int nSubLen = 0;
	int pos = 0;
	char *strpos = NULL;
	char *temp = NULL;

	nLen = strlen(str);
	nSubLen = strlen(dst);

	if (str==NULL || dst==NULL)
	{
		return NULL;
	}

	temp = (char *)malloc(nLen + 1);
	if (temp == NULL)
	{
		return NULL;
	}

	strcpy(temp, str);
	
	while (pos < nLen - nSubLen)
	{
		char Tmpchar = str[pos + nSubLen];
		temp[pos + nSubLen] = '\0';

		if (0 == strcasecmp(temp+pos, dst))
		{
			strpos = (char *)str + pos;
			break;
		}

		temp[pos + nSubLen] = Tmpchar;
		pos ++;
	}

	free(temp);
	
	return strpos;
}

int SaveWebSystem(int sockFd, int size, char *buffer)
{
	static int nFlag = 0;
	int ret = -1;
	NET_UPDATE_FILE_HEAD fileInfo;
	UPDATE_RETURN updateRet;
	MSG_HEAD returnMsg;
	char fileName[256];
	FILE *fp = NULL;
	int fileLen = 0;
	int count = 0;
	int pos = 0;
	int i = 0;
	int j = 0;
	char *temp = NULL;
	char cmd[256];
	
	if (sockFd<0 || sockFd>65535)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (nFlag == 0)
	{
		system("busybox rm -rf /mnt/mtd/dvs/www/*");
		nFlag = 1;	
	}
	
	memset(&updateRet, 0, sizeof(UPDATE_RETURN));
	memset(&returnMsg, 0, sizeof(MSG_HEAD));
	
	returnMsg.nCmd = NETCMD_UPDATE_PROCESS;	 
	returnMsg.nSock = sockFd;
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);
	
	// 获取升级文件的信息
	memset(&fileInfo, 0, sizeof(NET_UPDATE_FILE_HEAD));
	memcpy(&fileInfo, buffer, sizeof(NET_UPDATE_FILE_HEAD));
	memcpy(updateRet.fileName, fileInfo.strFileName, 32);
	
	// 打开要升级的文件
	memset(fileName, 0, 256);
	temp = FindStr(fileInfo.strFileName, "/");
	if (temp != NULL)
	{
		char dir[128];
		memset(cmd, 0, 256);
		memset(dir, 0, 128);
		memcpy(dir, fileInfo.strFileName, strlen(fileInfo.strFileName)-strlen(temp));
		dir[strlen(fileInfo.strFileName)-strlen(temp)] = '\0';
		sprintf(cmd, "busybox mkdir -p /mnt/mtd/dvs/www/%s", dir);
		system(cmd);
	}
	sprintf(fileName, "/mnt/mtd/dvs/www/%s", fileInfo.strFileName);	
	fp = fopen(fileName, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the update file(%s)!\n", fileName);
		
		return -1;
	}
	else
	{
		printf("Update the file(%s) ... \n", fileInfo.strFileName);
	}
	
	fileLen = fileInfo.nFileLen;
	count = fileInfo.nFileLen/1024;
	while (fileLen > 0)
	{
		if (fileLen >= 1024)
		{
			ret = fwrite((char *)(buffer+sizeof(NET_UPDATE_FILE_HEAD))+pos, 1024, 1, fp);
			fileLen -= 1024;
			pos += 1024;
			i++;
		}
		else
		{
			fwrite((char *)(buffer+sizeof(NET_UPDATE_FILE_HEAD))+pos, fileLen, 1, fp);
			pos += fileLen;
			fileLen = 0;
		}
		//usleep(10);
	
		if (i >= (count/100)*j)
		{
			j++;
			if (j > 99)
			{
				j = 99;
			}
			updateRet.nResult = APP_UPDATE_START;
			updateRet.nProgress = j;
			returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
			
			NETSDK_SendMsg(&returnMsg, (char *)&updateRet);

			//printf("%d %d\n", i, j);
		}
	}
	
	fclose(fp);
	chmod(fileName, 0777);
	
	updateRet.nResult = APP_UPDATE_STOP;
	updateRet.nProgress = 100;
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
		
	NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
	
	// 判断升级是否结束
	if (fileInfo.nReserve == 1)
	{
		nFlag = 0;
		
		return 0;
	}
	
	return 1;
}

int SaveMobileViewSystem(int sockFd, int size, char *buffer)
{
	static int nMobileFlag = 0;
	int ret = -1;
	NET_UPDATE_FILE_HEAD fileInfo;
	UPDATE_RETURN updateRet;
	MSG_HEAD returnMsg;
	char fileName[256];
	FILE *fp = NULL;
	int fileLen = 0;
	int count = 0;
	int pos = 0;
	int i = 0;
	int j = 0;
	char *temp = NULL;
	char cmd[256];
	
	if (sockFd<0 || sockFd>65535)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}
	
	if (nMobileFlag == 0)
	{
		//system("busybox rm -rf /mnt/mtd/dvs/mobile/*");
		nMobileFlag = 1;	
	}
	
	memset(&updateRet, 0, sizeof(UPDATE_RETURN));
	memset(&returnMsg, 0, sizeof(MSG_HEAD));
	
	returnMsg.nCmd = NETCMD_UPDATE_PROCESS;	 
	returnMsg.nSock = sockFd;
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);
	
	// 获取升级文件的信息
	memset(&fileInfo, 0, sizeof(NET_UPDATE_FILE_HEAD));
	memcpy(&fileInfo, buffer, sizeof(NET_UPDATE_FILE_HEAD));
	memcpy(updateRet.fileName, fileInfo.strFileName, 32);
	
	// 打开要升级的文件
	memset(fileName, 0, 256);
	temp = FindStr(fileInfo.strFileName, "/");
	if (temp != NULL)
	{
		char dir[128];
		memset(cmd, 0, 256);
		memset(dir, 0, 128);
		memcpy(dir, fileInfo.strFileName, strlen(fileInfo.strFileName)-strlen(temp));
		dir[strlen(fileInfo.strFileName)-strlen(temp)] = '\0';
		sprintf(cmd, "busybox mkdir -p /mnt/mtd/dvs/mobile/%s", dir);
		system(cmd);
	}
	sprintf(fileName, "/mnt/mtd/dvs/mobile/%s", fileInfo.strFileName);	
	fp = fopen(fileName, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the update file(%s)!\n", fileName);
		
		return -1;
	}
	else
	{
		printf("Update the file(%s) ... \n", fileInfo.strFileName);
	}
	
	fileLen = fileInfo.nFileLen;
	count = fileInfo.nFileLen/1024;
	while (fileLen > 0)
	{
		if (fileLen >= 1024)
		{
			ret = fwrite((char *)(buffer+sizeof(NET_UPDATE_FILE_HEAD))+pos, 1024, 1, fp);
			fileLen -= 1024;
			pos += 1024;
			i++;
		}
		else
		{
			fwrite((char *)(buffer+sizeof(NET_UPDATE_FILE_HEAD))+pos, fileLen, 1, fp);
			pos += fileLen;
			fileLen = 0;
		}
		//usleep(10);
	
		if (i >= (count/100)*j)
		{
			j++;
			if (j > 99)
			{
				j = 99;
			}
			updateRet.nResult = APP_UPDATE_START;
			updateRet.nProgress = j;
			returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
			
			NETSDK_SendMsg(&returnMsg, (char *)&updateRet);

			//printf("%d %d\n", i, j);
		}
	}
	
	fclose(fp);
	chmod(fileName, 0777);
	
	updateRet.nResult = APP_UPDATE_STOP;
	updateRet.nProgress = 100;
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
		
	NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
	
	// 判断升级是否结束
	if (fileInfo.nReserve == 1)
	{
		nMobileFlag = 0;
		
		return 0;
	}
	
	return 1;
}

int updateImageFile(int sockFd, int size, char *buffer, struct sockaddr_in addr, int nflag)
{
	int nRet = -1;
	int nFd = -1;
	pthread_t threadID;
	int nDevOpenFlag = 0;
	unsigned int crc1 = 0;
	unsigned int crc2 = 0;
	UPDATE_RETURN updateRet;
	MSG_HEAD returnMsg;
	NET_UPDATE_FILE_HEAD fileInfo;
	char pUpdateType[128];
	int nUpdateType = 0;
	
	if (sockFd<0 || sockFd>65535) 
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}

	// 初始化升级返回数据结构
	memset(&updateRet, 0, sizeof(UPDATE_RETURN));
	memset(&returnMsg, 0, sizeof(MSG_HEAD));
	
	returnMsg.nCmd = NETCMD_UPDATE_PROCESS;	 
	returnMsg.nSock = sockFd;
	returnMsg.addr = addr;
	returnMsg.nflag = g_server_info.nupdate_flag;
	g_update_addr = addr;
	//g_update_nflag = nflag;
	//printf("g_update_nflag  = %d\n", g_update_nflag);
	
	returnMsg.nBufSize = sizeof(UPDATE_RETURN);
	
	// 获取升级文件的信息
	memset(&fileInfo, 0, sizeof(NET_UPDATE_FILE_HEAD));
	memcpy(&fileInfo, buffer, sizeof(NET_UPDATE_FILE_HEAD));

	printf("Update Start: %s %d\n", fileInfo.strFileName, fileInfo.nFileLen);
	
	memcpy(updateRet.fileName, fileInfo.strFileName, 32);
	memcpy(g_update_file, fileInfo.strFileName, 32);
	memcpy(pUpdateType, fileInfo.strFilePath, 128);
	
	// CRC校验
	crc1 = fileInfo.nFileCRC;
	crc2 = GetDataCRC(buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
	if (crc1 != crc2)
	{
		updateRet.nResult = CRC_ERR;

		returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
		NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
		
		printf("Update Failed(CRC: %x %x)!\n", crc1, crc2);
		
		return -1;
	}
	
	// 判断升级类型,并打开相应的存储设备
	nRet = strcmp(pUpdateType, "KERNEL");
	if (nRet == 0)
	{
		nUpdateType = 1;
	}
	nRet = strcmp(pUpdateType, "SYSTEM");
	if (nRet == 0)
	{
		nUpdateType = 2;
	}
	//add by lvjh 2012-120-08
	#ifdef CCD
	nRet = strcmp(pUpdateType, "APPLICATION_CCD");
	if (nRet == 0)
	{
		nUpdateType = 3;
	}
	#endif
	#ifdef HD_CMOS
	nRet = strcmp(pUpdateType, "APPLICATION");
	if (nRet == 0)
	{
		nUpdateType = 3;
	}
	#endif
	
	nRet = strcmp(pUpdateType, "HWCONFIG");
	if (nRet == 0)
	{
		nUpdateType = 4;
	}
	nRet = strcmp(pUpdateType, "MINISYS");
	if (nRet == 0)
	{
		nUpdateType = 5;
	}
	nRet = strcmp(pUpdateType, "WEB");
	if (nRet == 0)
	{
		nUpdateType = 6;
	}
	// Add the code by lvjh, 2011-03-30
	nRet = strcmp(pUpdateType, "MOBILE");
	if (nRet == 0)
	{
		nUpdateType = 7;
	}
	
	switch (nUpdateType)
	{
	case 1:
		printf("Update Kernel!\n");
		nFd = open(KERNEL_FLASH_DEVICE, O_RDWR);
		if (nFd <= 0)
		{
			nDevOpenFlag = 0;
		}
		else
		{
			close(nFd);
			nDevOpenFlag = 1;
		}
		break;
		
	case 2:
		printf("Update system!\n");
		nFd = open(SYSTEM_FLASH_DEVICE, O_RDWR);
		if (nFd <= 0)
		{
			nDevOpenFlag = 0;
		}
		else
		{
			close(nFd);
			nDevOpenFlag = 1;
		}
		break;
		
	case 3:
		printf("Update Application!\n");
		nFd = open(APP_FLASH_DEVICE, O_RDWR);
		if (nFd <= 0)
		{
			nDevOpenFlag = 0;
		}
		else
		{
			close(nFd);
			nDevOpenFlag = 1;
		}
		break;
		
	case 4:
		printf("Update Hardware Configure!\n");
		nDevOpenFlag = 1;
		break;
		
	case 5:
		printf("Update Mini System!\n");
		nDevOpenFlag = 1;
		break;
		
	case 6:
		printf("Update Web System!\n");
		nDevOpenFlag = 1;
		break;
		
	case 7:
		printf("Update Mobile View System!\n");
		nDevOpenFlag = 1;
		break;
		
	default:
		printf("Update Type Error!\n");
		nDevOpenFlag = 0;
		break;
	}
	if (nDevOpenFlag == 0)
	{
		updateRet.nResult = APP_UPDATE_ERR;
		updateRet.nProgress = 0;
		returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
		NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
		printf("Can not open flash device!\n");
		return -1;
	}
	
	// 发送升级开始信息
	memset(&updateRet, 0, sizeof(UPDATE_RETURN));
	memcpy(updateRet.fileName, g_update_file, 32);
	updateRet.nResult = APP_UPDATE_START;
	updateRet.nProgress = 1;

	returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
	
	NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
	
	printf("Update(%d): 1%%!\n", returnMsg.nSock);
	
	g_update_flag = 1;
	g_update_socket = returnMsg.nSock;
	g_update_progress = 1;
	
	// 设置回调函数
	MTD_SetCallBack(sendUpdateProgress);
	
	// Add the code by lvjh, 2009-09-03
	switch (nUpdateType)
	{
	case 1:
		SYSCHECK_SetKernetStatus(KERNEL_FAIL);
		SYSCHECK_WriteFlash();
		break;
			
	case 2:
		SYSCHECK_SetRamdiskStatus(SYS_FAIL);
		SYSCHECK_WriteFlash();
		break;	
		
	case 3:
		SYSCHECK_SetAppStatus(APP_FAIL);
		SYSCHECK_SetAppFailedCount(0);
		SYSCHECK_ClearAppRunCount();
		SYSCHECK_WriteFlash();
		break;
		
	case 4:
		SYSCHECK_ClearAppRunCount();
		SYSCHECK_WriteFlash();
		break;
		
	case 5:	
		SYSCHECK_ClearAppRunCount();
		SYSCHECK_WriteFlash();
		break;
		
	case 6:	
		SYSCHECK_ClearAppRunCount();
		SYSCHECK_WriteFlash();
		break;
		
	case 7:	
		SYSCHECK_ClearAppRunCount();
		SYSCHECK_WriteFlash();
		break;
	}

	// 
	switch (nUpdateType)
	{
	case 1:
		printf("Update Kernel!\n");
		nRet = MTD_WriteAllRegion(KERNEL_FLASH_DEVICE, buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
		break;
		
	case 2:
		printf("Update system!\n");
		nRet = MTD_WriteAllRegion(SYSTEM_FLASH_DEVICE, buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
		break;
		
	case 3:
		printf("Update Application!\n");
#ifdef MOBILE_VIEW
		videoMobileModulePause();
		videoMobileModuleStop();
		sleep(3);
		system("umount /mnt/mtd/dvs/mobile/tmpfs");
		system("rm -rf /mnt/mtd/dvs/mobile");
#endif
		nRet = MTD_WriteAllRegion(APP_FLASH_DEVICE, buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
		break;
		
	case 4:
		printf("Update Hardware Configure!\n");
		nRet = SaveFile("/param/hw.conf", buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
		break;
		
	case 5:
		printf("Update Mini System!\n");
		nRet = SaveFile("/server/miniserver.out", buffer+sizeof(NET_UPDATE_FILE_HEAD), fileInfo.nFileLen);
		break;
		
	case 6:
		printf("Update Web System!\n");
		nRet = SaveWebSystem(sockFd, size, buffer);
		if (nRet == 1)
		{
			return 0;
		}
		break;
		
	case 7:
		printf("Update Mobile View System!\n");
		nRet = SaveMobileViewSystem(sockFd, size, buffer);
		if (nRet == 1)
		{
			return 0;
		}
		break;
	}
	
	g_update_flag = 0;

	// 修改升级标识
	if (nRet < 0)
	{
		switch (nUpdateType)
		{
		case 1:
			SYSCHECK_SetKernetStatus(KERNEL_FAIL);
			SYSCHECK_WriteFlash();
			break;
			
		case 2:
			SYSCHECK_SetRamdiskStatus(SYS_FAIL);
			SYSCHECK_WriteFlash();
			break;	
		
		case 3:
			SYSCHECK_SetAppStatus(APP_FAIL);
			SYSCHECK_SetAppFailedCount(0);
			SYSCHECK_WriteFlash();
			break;	
		}
	}
	else
	{
		switch (nUpdateType)
		{
		case 1:
			SYSCHECK_SetKernetStatus(KERNEL_OK);
			SYSCHECK_WriteFlash();
			break;
			
		case 2:
			SYSCHECK_SetRamdiskStatus(SYS_OK);
			SYSCHECK_WriteFlash();
			break;	
		
		case 3:
			SYSCHECK_SetAppStatus(APP_OK);
			SYSCHECK_SetAppFailedCount(0);
			SYSCHECK_WriteFlash();
			break;	
		}		
	}
	
		
	if (nRet < 0)
	{
		updateRet.nResult = CRC_ERR;
		updateRet.nProgress = 0;
		memcpy(updateRet.fileName, g_update_file, 32);
		returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
		NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
		
		printf("Update Failed: MTD Error!\n");

#ifdef MOBILE_VIEW		
		videoMobileModuleResume();
#endif		
		
		return -1;
	}
	else
	{
		updateRet.nResult = APP_UPDATE_START;
		updateRet.nProgress = 100;
		memcpy(updateRet.fileName, g_update_file, 32);
		returnMsg.nBufSize = sizeof(UPDATE_RETURN);		
				
		NETSDK_SendMsg(&returnMsg, (char *)&updateRet);
		
		printf("Update(%d): 100%%!\n", returnMsg.nSock);
	}

	#if 1
	sleep(1);		
	watchDogStop(); 
	system("busybox reboot -f");

	#endif
	
	//RebootSystem();
	return 0;
}
