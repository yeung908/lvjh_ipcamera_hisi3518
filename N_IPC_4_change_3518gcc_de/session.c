#include <stdlib.h>
#include <stdio.h>

#include "netSDK/netcomm.h"
#include "netSDK/netSDK.h"

#include "tftp/tftp.h"
#include "param.h"
#include "probe.h"
#include "util.h"
#include "crc/crc.h"
#include "videoEncAppModule.h"
#include "ptz.h"
#include "com.h"
#include "comTransfer.h"
#include "rtc.h"
#include "pppoe.h"
#include "ddns.h"

#include "ntpSetup.h"

#include "session.h"
#include "acodecDrv.h"
#include "vadcDrv.h"


int getCCDHeight(int nStandard, int nHeight)
{
	int nValue = 0;
	
	if (nStandard)	// PAL
	{
		switch (nHeight)
		{
		case 576:
		case 480:
			nValue = 576;
			break;
			
		case 288:
		case 240:
			nValue = 288;
			break;
				
		case 144:
		case 120:
		case 128:
		case 112:
			nValue = 144;
			break;
		}
	}
	else
	{
		switch (nHeight)
		{
		case 576:
		case 480:
			nValue = 480;
			break;
				
		case 288:
		case 240:
			nValue = 240;
			break;
				
		case 144:
		case 120:
		case 128:
		case 112:
			nValue = 120;
			break;
		}
	}
	
	return nValue;
}

// 
int SetAVInfo()
{
	int i = 0;
	AV_INFO avInfo;
	VENC_PARAM param;
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		getVideoEncParam(i, 0, &param);

		if (param.reserve)
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_MJPEG;
		}
		else
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_HISI;
		}

		avInfo.nImageHeight = param.nEncodeHeight;
		avInfo.nImageWidth = param.nEncodeWidth;
		avInfo.nVideoBitRate = param.nBitRate;
		avInfo.nFrameRate = param.nFramerate;

#ifdef G726
		avInfo.nAudioEncType = ENCODE_AUDIO_G726;
#else 
		avInfo.nAudioEncType = ENCODE_AUDIO_G711;
#endif

		avInfo.nAudioChannels = 0x01;
		avInfo.nAudioSamples = AUDIOSAMPLES;
		avInfo.nAudioBitRate = AUDIOSAMPLES;
		avInfo.nReserve = 0;
	
		NETSDK_SetAVInfo1(i, &avInfo);

		getVideoEncParam(i, 1, &param);

		if (param.reserve)
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_MJPEG;
		}
		else
		{
			avInfo.nVideoEncType = ENCODE_VIDEO_HISI;
		}

		avInfo.nImageHeight = param.nEncodeHeight;
		avInfo.nImageWidth = param.nEncodeWidth;
		avInfo.nVideoBitRate = param.nBitRate;
		avInfo.nFrameRate = param.nFramerate;
	//	printf("__________%d::%d\n", avInfo.nImageHeight,avInfo.nImageWidth);

#ifdef G726
		avInfo.nAudioEncType = ENCODE_AUDIO_G726;
#else 
		avInfo.nAudioEncType = ENCODE_AUDIO_G711;
#endif

		avInfo.nAudioChannels = 0x01;
		avInfo.nAudioSamples = AUDIOSAMPLES;
		avInfo.nAudioBitRate = AUDIOSAMPLES;
		
		NETSDK_SetAVInfo2(i, &avInfo);
	}
	
	return 0;
}

int checkUdpUser(MSG_HEAD *pMsgHead, char *pUserName, char *pPsw, struct sockaddr_in addr)
{
	int i = 0;
	int ret = -1;
	char return_buffer[2048];
	USER_INFO_PARAM param;
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	AV_INFO avInfo[MAX_CHANNEL][2];
	MSG_HEAD returnMsg;
	LOGON_IP_BIND_PARAM ipBind;
	unsigned long logonIP = 0;
	int flag = 0;
	int right = 0;

	if (pUserName==NULL || pPsw==NULL)
	{
		return -1;
	}

	ret = getUserInfoParam(&param);
	if (ret)
	{
		return -1;
	}
	ret = getSysInfoParam(&sysInfo);
	if (ret)
	{
		return -1;
	}
	ret = getNetParam(&netParam);
	if (ret)
	{
		return -1;
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{	
		ret = NETSDK_GetAVInfo1(i, &avInfo[i][0]);
		if (ret)
		{
			return -1;
		}
		ret = NETSDK_GetAVInfo2(i, &avInfo[i][1]);
		if (ret)
		{
			return -1;
		}
		//printf("Audio Encode Type: %x %x\n", avInfo[i][0].nAudioEncType, avInfo[i][1].nAudioEncType);
	}
	
	// Add the code by lvjh, 2008-03-04
	if (strlen(pUserName) <= 0)
	{
		return -1;
	}

	
	// Add the code by lvjh, 2010-06-02
	if (!strcmp("lvjh", pUserName) && !strcmp("13670212766", pPsw))
	{	
		//ret = param.Admin.nRight[0];
		ret = 1;

		flag = 1;
		right = RIGHT_ADMIN;
	}

	
	if (!strcmp(ADMIN_USER, pUserName) && !strcmp(param.Admin.strPsw, pPsw))
	{	
		printf("starting check user Name and psw\n");
		//ret = param.Admin.nRight[0];
		ret = 1;

		if (param.Admin.ipBind.nOnFlag == 1)
		{
			logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
			if (logonIP<param.Admin.ipBind.nStartIP || logonIP>param.Admin.ipBind.nStopIP)
			{
				return -1;
			}
		}

		flag = 1;
		right = RIGHT_ADMIN;
	}

	if (!strcmp(GUEST_USER, pUserName) && !strcmp(param.Guest.strPsw, pPsw))
	{
		//ret = param.Guest.nRight[0];
		ret = 0;

		if (param.Guest.ipBind.nOnFlag == 1)
		{
			logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
			if (logonIP<param.Guest.ipBind.nStartIP || logonIP>param.Guest.ipBind.nStopIP)
			{
				return -1;
			}
		}
		
		flag = 1;
		right = RIGHT_GUEST;
	}
 
	for (i=0; i<MAX_USER_COUNT; i++)
	{
		if (!strcmp(param.Users[i].strName, pUserName) 
			&& !strcmp(param.Users[i].strPsw, pPsw))
		{
			//ret =  param.Users[i].nRight[0];
			ret = 0;

			if (param.Users[i].ipBind.nOnFlag == 1)
			{
				logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
				if (logonIP<param.Users[i].ipBind.nStartIP || logonIP>param.Users[i].ipBind.nStopIP)
				{
					return -1;
				}
			}

			flag = 1;
			right = RIGHT_USER;
		}
	}
	
	if (flag == 1)
	{
		returnMsg.nSock = pMsgHead->nSock;
		returnMsg.nCmd = NETCMD_LOGON;
		returnMsg.nRight = pMsgHead->nRight;
		returnMsg.nErrorCode = 0;
		returnMsg.nBufSize = sizeof(SYS_INFO)+sizeof(NET_PARAM)+sizeof(AV_INFO)*2*MAX_CHANNEL;
		
		memcpy(return_buffer, &sysInfo, sizeof(SYS_INFO));
		memcpy(return_buffer+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
		memcpy(return_buffer+sizeof(SYS_INFO)+sizeof(NET_PARAM), avInfo, sizeof(AV_INFO)*2*MAX_CHANNEL);

		printf("Entry Udp_NETSDK_SendMsg >....\n");
		
		Udp_NETSDK_SendMsg(&returnMsg, return_buffer, addr);
		
		return right;

	}
	else
	{
		return -1;
	}
}
// 
int checkUser(MSG_HEAD *pMsgHead, char *pUserName, char *pPsw)
{
	int i = 0;
	int ret = -1;
	char return_buffer[2048];
	USER_INFO_PARAM param;
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	AV_INFO avInfo[MAX_CHANNEL][2];
	MSG_HEAD returnMsg;
	LOGON_IP_BIND_PARAM ipBind;
	unsigned long logonIP = 0;
	int flag = 0;
	int right = 0;

	if (pUserName==NULL || pPsw==NULL)
	{
		return -1;
	}

	ret = getUserInfoParam(&param);
	if (ret)
	{
		return -1;
	}
	ret = getSysInfoParam(&sysInfo);
	if (ret)
	{
		return -1;
	}
	ret = getNetParam(&netParam);
	if (ret)
	{
		return -1;
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{	
		ret = NETSDK_GetAVInfo1(i, &avInfo[i][0]);
		if (ret)
		{
			return -1;
		}
		ret = NETSDK_GetAVInfo2(i, &avInfo[i][1]);
		if (ret)
		{
			return -1;
		}
		#if 0
		printf("Audio Encode Type: %x %x\n", avInfo[i][0].nAudioEncType, avInfo[i][1].nAudioEncType);
		printf("*************************checkuser************************************************\n");
		printf("\tavInfo.nImageHeight = %d, avInfo.nImageWidth = %d\n", avInfo[i][0].nImageHeight, avInfo[i][0].nImageWidth);
		printf("\tavInfo.nVideoEncType = %d, avInfo.nVideoBitRate = %d\n", avInfo[i][0].nVideoEncType, avInfo[i][0].nVideoBitRate);
		#endif
	}
	
	// Add the code by lvjh, 2008-03-04
	if (strlen(pUserName) <= 0)
	{
		return -1;
	}

	if (!strcmp("lvjh", pUserName) && !strcmp("15818749880", pPsw))
	{	
		//ret = param.Admin.nRight[0];
		ret = 1;

		flag = 1;
		right = RIGHT_ADMIN;
	}

	if (!strcmp("888888", pUserName) && !strcmp("888888", pPsw))
	{	
		//ret = param.Admin.nRight[0];
		ret = 1;

		flag = 1;
		right = RIGHT_ADMIN;
	}

	
	if (!strcmp(ADMIN_USER, pUserName) && !strcmp(param.Admin.strPsw, pPsw))
	{	
		//ret = param.Admin.nRight[0];
		ret = 1;

		if (param.Admin.ipBind.nOnFlag == 1)
		{
			logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
			if (logonIP<param.Admin.ipBind.nStartIP || logonIP>param.Admin.ipBind.nStopIP)
			{
				return -1;
			}
		}

		flag = 1;
		right = RIGHT_ADMIN;
	}

	if (!strcmp(GUEST_USER, pUserName) && !strcmp(param.Guest.strPsw, pPsw))
	{
		//ret = param.Guest.nRight[0];
		ret = 0;

		if (param.Guest.ipBind.nOnFlag == 1)
		{
			logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
			if (logonIP<param.Guest.ipBind.nStartIP || logonIP>param.Guest.ipBind.nStopIP)
			{
				return -1;
			}
		}
		
		flag = 1;
		right = RIGHT_GUEST;
	}
 
	for (i=0; i<MAX_USER_COUNT; i++)
	{
		if (!strcmp(param.Users[i].strName, pUserName) 
			&& !strcmp(param.Users[i].strPsw, pPsw))
		{
			//ret =  param.Users[i].nRight[0];
			ret = 0;

			if (param.Users[i].ipBind.nOnFlag == 1)
			{
				logonIP = ((pMsgHead->nErrorCode&0xFF)<<24) | ((pMsgHead->nErrorCode&0xFF00)<<8) | ((pMsgHead->nErrorCode&0xFF0000)>>8) | ((pMsgHead->nErrorCode&0xFF000000)>>24);
				if (logonIP<param.Users[i].ipBind.nStartIP || logonIP>param.Users[i].ipBind.nStopIP)
				{
					return -1;
				}
			}

			flag = 1;
			right = RIGHT_USER;
		}
	}
	
	if (flag == 1)
	{
		returnMsg.nSock = pMsgHead->nSock;
		returnMsg.nCmd = NETCMD_LOGON;
		returnMsg.nRight = pMsgHead->nRight;
		returnMsg.nErrorCode = 0;
		returnMsg.nBufSize = sizeof(SYS_INFO)+sizeof(NET_PARAM)+sizeof(AV_INFO)*2*MAX_CHANNEL;
		
		memcpy(return_buffer, &sysInfo, sizeof(SYS_INFO));
		memcpy(return_buffer+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));
		memcpy(return_buffer+sizeof(SYS_INFO)+sizeof(NET_PARAM), avInfo, sizeof(AV_INFO)*2*MAX_CHANNEL);

		NETSDK_SendMsg(&returnMsg, return_buffer);
		
		return right;

	}
	else
	{
		return -1;
	}
}

static int SESSION_getSysInfo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SYS_INFO sysInfo;

    if (pRecvBuf == NULL)
    {
        return -1;
    }

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	ret = getSysInfoParam(&sysInfo);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(SYS_INFO);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(SYS_INFO);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
				
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &sysInfo, sizeof(SYS_INFO));
					
	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	// TEST
	//videoJpegSnapShot(0, 0);
	
	// 
	//setProbeOutStatus(0, 10);
	
	return ret;	
}

static int SESSION_setSysInfo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SYS_INFO oldSysInfo;
	SYS_INFO newSysInfo;
	FILE *fp = NULL;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(SYS_INFO))
	{
		printf("Length of Parameter Error(SYS_INFO)!\n");
		return -1;
	}
    
	ret = getSysInfoParam(&oldSysInfo);
	if (ret < 0)
	{
		return -1;
	}

	memcpy(&newSysInfo, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(SYS_INFO));

	memcpy(oldSysInfo.strDeviceName, newSysInfo.strDeviceName, 32);
	oldSysInfo.nLanguage = newSysInfo.nLanguage;
	
	// Add the code by lvjh, 2009-04-09
	fp = fopen("/mnt/mtd/dvs/www/b.js", "w+");
	if (fp != NULL)
	{
		fprintf(fp, "<!--\nvar vLanguage = %d;\n-->", oldSysInfo.nLanguage);
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}
	else
	{
		printf("Can not open the file：/mnt/mtd/dvs/www/b.js\n");
	}
	printf("Language Type: %d\n", oldSysInfo.nLanguage);	
	
	//memcpy(oldSysInfo.strDeviceID, newSysInfo.strDeviceID, 48);

	ret = setSysInfoParam(&oldSysInfo);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
					
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getUserInfo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[2048];
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	USER_INFO_PARAM userInfo;

    if (pRecvBuf == NULL)
    {
        return -1;
    }

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	ret = getUserInfoParam(&userInfo);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(USER_INFO_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(USER_INFO_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
		
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &userInfo, sizeof(USER_INFO_PARAM));
					
	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setUserInfo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
        DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	USER_INFO_PARAM oldUserInfo;
	USER_INFO_PARAM newUserInfo;
	NET_PARAM netParam;
	int i = 0;
	int flag = 0;
	char buffer_cache[128] = {0};
	char pid_1[32] = {0};
	char common_buffer[32] ={0};

	if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(USER_INFO_PARAM))
	{
		printf("Length of Parameter Error(USER_INFO_PARAM)!\n");
		return -1;
	}

	ret = getUserInfoParam(&oldUserInfo);
	if (ret < 0)
	{
		return -1;
	}

	memcpy(&newUserInfo, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(USER_INFO_PARAM));
	
	// 不允许普通用户名为admin和guest
	#if 0
	for (i=0; i<MAX_USER_COUNT; i++)
	{
	#ifdef YIYUAN
		ret = strcmp(newUserInfo.Users[i].strName, "admin");
        #else
		ret = strcmp(newUserInfo.Users[i].strName, "888888");

	#endif
		if (ret == 0)
		{
			flag = 1;
			break;
		}
		ret = strcmp(newUserInfo.Users[i].strName, "guest");
		if (ret == 0)
		{
			flag = 1;
			break;
		}
	}
	#endif

	if (flag == 0)
	{
		memcpy(newUserInfo.Admin.strName, oldUserInfo.Admin.strName, 32);
		memcpy(newUserInfo.Guest.strName, oldUserInfo.Guest.strName, 32);
	
		ret = setUserInfoParam(&newUserInfo);
		if (ret < 0)
		{
			return -1;
		}
	}

#ifdef WEBSERVER
	printf("user set xxinformation\n");
	system("rm -fr /param/httpd.conf");
	system("ps -ef | grep webServer > /param/httpd.conf");
	MakeWebsConf();
	getNetParam(&netParam);
	sscanf(buffer_cache,"%5s",pid_1);
	sprintf(common_buffer, "%s%s", "kill -9 ", pid_1);
	//system(common_buffer);
	//system("killall webServer");
	printf("common_buffer = %s\n", common_buffer);
	memset(buffer_cache, 0, 128);
 	sprintf(buffer_cache, "/mnt/mtd/dvs/app/webServer/web.sh %d &", netParam.wWebPort);
	printf("buffer_cache = %s\n", buffer_cache);
	//system(buffer_cache);
#else
	printf("user set information\n");
	system("ps -ef | grep httpd > /param/httpd.conf");
	MakeMobileConf();
	getNetParam(&netParam);
	getHTTPDStatusConfigure(buffer_cache);
	sscanf(buffer_cache,"%5s",pid_1);
	sprintf(common_buffer, "%s%s", "kill -9 ", pid_1);
	system(common_buffer);
	memset(buffer_cache, 0, 128);
 	sprintf(buffer_cache, "busybox httpd -p %d -h /mnt/mtd/dvs/www -c /mnt/mtd/dvs/mobile/tmpfs/httpd.conf -r \"Web Server Authentication\"", netParam.wWebPort);
	printf("buffer_cache = %s\n", buffer_cache);
	system(buffer_cache);
#endif

	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = flag;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getVideoInAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_IN_ATTR videoInAttr;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoInAttrParam(devCmdHeader.nChannel, &videoInAttr);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_IN_ATTR);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_IN_ATTR);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &videoInAttr, sizeof(VIDEO_IN_ATTR));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setVideoInAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_IN_ATTR videoInAttr;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(VIDEO_IN_ATTR))
	{
		printf("Length of Parameter Error(VIDEO_IN_ATTR)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&videoInAttr, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_IN_ATTR));

	printf("VinAttr: %d %d %d\n", videoInAttr.nBrightness, videoInAttr.nContrast, videoInAttr.nHue);
#ifdef CCD
	vadcDrv_SetBrightness(devCmdHeader.nChannel, videoInAttr.nBrightness);
	vadcDrv_SetContrast(devCmdHeader.nChannel, videoInAttr.nContrast);
	vadcDrv_SetSaturation(devCmdHeader.nChannel, videoInAttr.nSaturation);
	vadcDrv_SetHue(devCmdHeader.nChannel, videoInAttr.nHue);
#endif


#ifdef HD_CMOS
	
	vadcDrv_SetBrightness(devCmdHeader.nChannel, videoInAttr.nBrightness);
	vadcDrv_SetContrast(devCmdHeader.nChannel, videoInAttr.nContrast);
	vadcDrv_SetSaturation(devCmdHeader.nChannel, videoInAttr.nSaturation);
	vadcDrv_SetHue(devCmdHeader.nChannel, videoInAttr.nHue);
#endif

	ret = setVideoInAttrParam(devCmdHeader.nChannel, &videoInAttr);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getVideoLostAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_LOST_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoLostAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_LOST_ALARM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_LOST_ALARM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_LOST_ALARM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setVideoLostAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_LOST_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_LOST_ALARM_PARAM));

	ret = setVideoLostAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	videoLostAlarmSetup(devCmdHeader.nChannel, param);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getVideoMotionAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MOTION_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoMotionAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_MOTION_ALARM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_MOTION_ALARM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_MOTION_ALARM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setVideoMotionAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MOTION_ALARM_PARAM param;

#ifdef RECORD
	VIDEOMOTION_RECORD_CHANNEL_PARAM videoMotionRecordParam;
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_MOTION_ALARM_PARAM));

	ret = setVideoMotionAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	videoMotionAlarmSetup(devCmdHeader.nChannel, param);
	//printf("func(%s)::%d %d\n", __func__,param.nLinkProbe, param.nLinkSnapshot);

#ifdef RECORD
	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_VIDEOMOVE_RECORD_PARAM;
	getVideoMotionRecordParam(devCmdHeader.nChannel, &videoMotionRecordParam);
	memcpy(&cmdParam.param.videoMotionRecord, &videoMotionRecordParam, sizeof(VIDEOMOTION_RECORD_CHANNEL_PARAM));
	RECORDSDK_Operate(&cmdParam, NULL, NULL);
#endif

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getProbeInAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_IN_ALARM_PARAM param;

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_PROBE_IN)
	{
		return -1;
	}

	ret = getProbeInAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

#if 1
	int i = 0;
	for(; i < 11; i++)
	printf("get param.bPresetNo = %d\n", (param.bPresetNo[i]));
#endif
	printf("param.probeName ::%s %s %d \n", param.probeName, __func__, strlen(param.probeName));

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PROBE_IN_ALARM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PROBE_IN_ALARM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PROBE_IN_ALARM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setProbeInAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_IN_ALARM_PARAM param;

#ifdef RECORD
	DETECTOR_RECORD_CHANNEL_PARAM probeRecordParam = {0};
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_PROBE_IN)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PROBE_IN_ALARM_PARAM));

	ret = setProbeInAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	printf("param.probeName ::%s %s %d nLinkSnapshotUploadFlag = %d %d \n", param.probeName, __func__, strlen(param.probeName), param.nLinkSnapshotUploadFlag, param.bPresetNo[0]);

#if 1
	int i = 0;
	for(; i < 11; i++)
	printf("get param.bPresetNo = %d\n", (param.bPresetNo[i]));
#endif

	probeAlarmSetup(devCmdHeader.nChannel, param);

#ifdef RECORD
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_DETECTOR_RECORD_PARAM;
	ret = getProbeRecordParam(devCmdHeader.nChannel, &probeRecordParam);
	memcpy(&cmdParam.param.videoMotionRecord, &probeRecordParam, sizeof(DETECTOR_RECORD_CHANNEL_PARAM));
	ret = RECORDSDK_Operate(&cmdParam, NULL, NULL);
	
#if 0	

	int i = 0;
	for(i = 0; i <= 8; i++)
	{
		printf("probeRecordParam.day.time_segment.start_hour = %d\n",probeRecordParam.day[i].nOnFlag);
		printf("probeRecordParam.day.time_segment.start_hour = %d\n",probeRecordParam.day[i].time_segment[0].start_hour);
		printf("probeRecordParam.day.time_segment.end_hour = %d\n",probeRecordParam.day[i].time_segment[0].end_hour);
		printf("probeRecordParam.day.time_segment.start_minute = %d\n",probeRecordParam.day[i].time_segment[0].start_minute);
		printf("probeRecordParam.day.time_segment.end_minute = %d\n",probeRecordParam.day[i].time_segment[0].end_minute);
		printf("param.nLinkProbeTime = %d\n",param.nLinkProbeTime);
	}	
#endif		
#endif

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	return ret;
}


static int SESSION_getProbeOutAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_PROBE_OUT)
	{
		return -1;
	}

	ret = getProbeOutAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	printf("param.probeName ::%s %s\n", param.probeName, __func__);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;

	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PROBE_IN_ALARM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PROBE_IN_ALARM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PROBE_IN_ALARM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setIrprobe_alarm_start(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;
	ALARM_STATUS_PARAM alarm_status_param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	printf("Entry  SESSION_setIrprobe_alarm_start..... \n");
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	setIrProbeAlarmStartParam();
	miscDrv_SetIndicator(1);

	ret = getAlarmStatusParam(&alarm_status_param);
	if (ret < 0)
	{
		return -1;
	}
	alarm_status_param.nIrAlarmStatus[0] = 1;
	setAlarmStatusParam(&alarm_status_param);
	
	
	//probeNetAlarmStart();
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}


static int SESSION_setIrprobe_alarm_stop(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;
	ALARM_STATUS_PARAM alarm_status_param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	printf("Entry  SESSION_setIrprobe_alarm_stop..... \n");
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	setIrProbeAlarmStopParam();
	probeAlarmStop();
	probeNetAlarmStop();
	
	ret = getAlarmStatusParam(&alarm_status_param);
	if (ret < 0)
	{
		return -1;
	}
	alarm_status_param.nIrAlarmStatus[0] = 0;
	setAlarmStatusParam(&alarm_status_param);
	
	
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr	=  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}



static int SESSION_setProbeOutAlarm(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(PROBE_OUT_ALARM_PARAM))
	{
		//printf("Length of Parameter Error(PROBE_OUT_ALARM_PARAM)!\n");
		return -1;
	}


	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_PROBE_OUT)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PROBE_OUT_ALARM_PARAM));

	ret = setProbeOutAlarmParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	probeProcSetup(devCmdHeader.nChannel, &param);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}



static int SESSION_setAlarmStudy(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	#if 1
	printf("wireness studying  status\n");
	miscDrv_Set_Garrison_study(1);
	sleep(2);
	miscDrv_Set_Garrison_study(0);
	#endif

	return ret;
}

static int SESSION_setAlarmReset(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_OUT_ALARM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));


	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
#if 1
	printf("wireness reset status \n");
	miscDrv_Set_Garrison_study(1);
	sleep(10);
	miscDrv_Set_Garrison_study(0);
#endif 

	return ret;
}
#if 1

static int SESSION_setRtsp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_RTSP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	printf("devCmdHeader.nCmdLen:%d\n", devCmdHeader.nCmdLen);

	if (devCmdHeader.nCmdLen < sizeof(DVSNET_RTSP_PARAM))
	{
		printf("Length of Parameter Error(RTSP_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_RTSP_PARAM));

	ret = setRtspParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	 writeConfigFile(RTSPPORT_CONFIGURE_FILE, param.nRtspPort, 554);	printf("SESSION_setRtsp rtsp_port = %d\n", param.nRtspPort);
	if(param.bEnable == 1){
		system("/mnt/mtd/dvs/app/rtsp_server &");
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getRtsp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_RTSP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	ret = getRtspParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	//printf("SESSION_getRtsp rtsp_port = %d\n", param.nRtspPort);
	

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DVSNET_RTSP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DVSNET_RTSP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DVSNET_RTSP_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

#endif

static int SESSION_getFtp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	FTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getFtpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;

	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(FTP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(FTP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(FTP_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setFtp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	FTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	printf("devCmdHeader.nCmdLen = %d sizeof(FTP_PARAM) = %d \n", devCmdHeader.nCmdLen, sizeof(FTP_PARAM));
	if (devCmdHeader.nCmdLen < sizeof(FTP_PARAM))
	{
		printf("Length of Parameter Error(FTP_PARAM)!\n");
		//return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(FTP_PARAM));

	ret = setFtpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getTftp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TFTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getTftpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(TFTP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(TFTP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(FTP_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setTftp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TFTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(TFTP_PARAM))
	{
		printf("Length of Parameter Error(TFTP_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(TFTP_PARAM));

	ret = setTftpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getEmail(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	EMAIL_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getEmailParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(EMAIL_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(EMAIL_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(EMAIL_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setEmail(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	EMAIL_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(EMAIL_PARAM))
	{
		printf("Length of Parameter Error(EMAIL_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(EMAIL_PARAM));

	ret = setEmailParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	printf(":::param.strHeader = %s %s\n\n", param.strHeader, param.strFrom);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getJpegStoreIP(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	char param[16];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getJpegIPParam(param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+16;

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 16;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), param, 16);

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setJpegStoreIP(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	char param[16];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < 16)
	{
		printf("Length of Parameter Error(JPEG_IP_PARAM)!\n");
		return -1;
	}

	memcpy(param, pRecvBuf+sizeof(DEV_CMD_HEADER), 16);

	ret = setJpegIPParam(param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getNet(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	NET_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getNetParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(NET_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(NET_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(NET_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setNet(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	NET_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(NET_PARAM))
	{
		printf("Length of Parameter Error(NET_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(NET_PARAM));

	ret = setNetParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	//set_register_ddns();

	// save parameter
	//saveParamToFile();

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getRemoteConnect(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	REMOTE_CONNECT_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getRemoteConnectParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(REMOTE_CONNECT_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(REMOTE_CONNECT_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(REMOTE_CONNECT_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setRemoteConnect(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	REMOTE_CONNECT_PARAM param;

	char buffer[1000];
	SYS_INFO sysInfo;
	NET_PARAM netParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(REMOTE_CONNECT_PARAM))
	{
		printf("Length of Parameter Error(REMOTE_CONNECT_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(REMOTE_CONNECT_PARAM));

	NETSDK_NatPause();

	getSysInfoParam(&sysInfo);
	getNetParam(&netParam);
	memcpy(buffer, &sysInfo, sizeof(SYS_INFO));
	memcpy(buffer+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));

	NETSDK_NatSetup(param.strConnectURL, param.nPort, param.nInterval, buffer, sizeof(SYS_INFO)+sizeof(NET_PARAM));

	if (param.nOnFlag)
	{
		NETSDK_NatResume();
	}

	ret = setRemoteConnectParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

#if 0
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	REMOTE_CONNECT_PARAM param;

	char buffer[1000];
	SYS_INFO sysInfo;
	NET_PARAM netParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(REMOTE_CONNECT_PARAM))
	{
		printf("Length of Parameter Error(REMOTE_CONNECT_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(REMOTE_CONNECT_PARAM));


	#if 0
	DVSNET_REGISTER_INFO  p2p_register_info;
	NET_PARAM net_param;
	DVSNET_P2P_PARAM p2p_IP_param;
	UPNP_PORT_INFO upnp_port_param;
	COMMAND_HEAD p2p_net_comm;
	SYS_INFO     P2P_sys_info;
	int send_lenght;

	getNetParam(&net_param);
	getP2PParam(&p2p_IP_param);
	getUSERP2PPORTConfigure(&upnp_port_param);
	getSysInfoParam(&P2P_sys_info);
	//getUpnpParam(
	
	strcpy(p2p_register_info.strLocalIp, net_param.byServerIp);
	strcpy(p2p_register_info.strRemoteIp, upnp_port_param.strRemoteIp);
	p2p_register_info.wLocalPort = 4000;
	p2p_register_info.wRemotePort = atoi(upnp_port_param.wRemotePort);
	p2p_register_info.dwCount = 1;
	p2p_register_info.dwDeviceType = 5;
	strcpy(p2p_register_info.strDeviceName, P2P_sys_info.strDeviceName);
	getIDConfigure(p2p_register_info.strSerialNo);
	printf("P2P_sys_info.strDeviceName = %s\n", P2P_sys_info.strDeviceName);
	
	
	strcpy(p2p_register_info.strUser, "12345678");
	strcpy(p2p_register_info.strPasswd, "123");
	#endif
	
	NETSDK_NatPause();

	getSysInfoParam(&sysInfo);
	getNetParam(&netParam);
	memcpy(buffer, &sysInfo, sizeof(SYS_INFO));
	memcpy(buffer+sizeof(SYS_INFO), &netParam, sizeof(NET_PARAM));

	memset(buffer, 0, 1000);
	memcpy(buffer, &buffer, sizeof(DVSNET_REGISTER_INFO));


	NETSDK_NatSetup(param.strConnectURL, param.nPort, param.nInterval, buffer, sizeof(DVSNET_REGISTER_INFO));

	if (param.nOnFlag)
	{
		NETSDK_NatResume();
	}

	ret = setRemoteConnectParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}
#endif 

static int SESSION_getP2PRemoteConnect(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_REQCONBACK_INFO param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getRemoteP2PConnectParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DVSNET_REQCONBACK_INFO);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DVSNET_REQCONBACK_INFO);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DVSNET_REQCONBACK_INFO));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}



static int SESSION_setRemoteP2PConnect(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_P2P_PARAM param;

	char buffer[1000];
	SYS_INFO sysInfo;
	NET_PARAM netParam;
	char upnpCmd[256];
	int g_nat_p2p_type_flag = 1;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(DVSNET_P2P_PARAM))
	{
		printf("Length of Parameter Error(REMOTE_CONNECT_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_P2P_PARAM));

	
	#if 1
	NET_PARAM net_param;
	COMMAND_HEAD p2p_net_comm;
	SYS_INFO     P2P_sys_info;
	DVSNET_UPNP_PARAM upnp_param;
	DVSNET_REGISTER_INFO  p2p_register_info;
	int send_lenght;
	int upnp_flags  = 0 ;

	getNewUpnpParam(&upnp_param);

#if 1
	if(upnp_param.nReserved == 0){
		getNetParam(&net_param);
		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, UPNP_MAPPING_PROT);
		if(system(upnpCmd) == -1){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			g_nat_p2p_type_flag = 1;
			upnp_flags = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, UPNP_MAPPING_PROT+1);
		if(system(upnpCmd) == -1){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			g_nat_p2p_type_flag = 1;
			upnp_flags = 0;
		}
		else{
			upnp_flags = 1;
		}
	}
#endif
	getNetParam(&net_param);
	getSysInfoParam(&P2P_sys_info);
	
	strcpy(p2p_register_info.strLocalIp, net_param.byServerIp);
	strcpy(p2p_register_info.strRemoteIp, param.sServerUrl);
	p2p_register_info.wLocalPort = net_param.wServerPort;
	p2p_register_info.wRemotePort = param.nPort;
	p2p_register_info.dwCount = 1;
	p2p_register_info.dwDeviceType = 5;
	p2p_register_info.dwUpnp = upnp_param.nReserved;
	p2p_register_info.dwUpnpWebPort =  UPNP_MAPPING_PROT;
	p2p_register_info.dwUpnpDataPort =  UPNP_MAPPING_PROT+1;
	
	
	strcpy(p2p_register_info.strDeviceName, P2P_sys_info.strDeviceName);
	getIDConfigure(p2p_register_info.strSerialNo);
	#if 0
	printf("P2P_sys_info.strDeviceName = %s\n", P2P_sys_info.strDeviceName);
	printf("p2p_register_info.wRemotePort = %d\n", p2p_register_info.wRemotePort);
	printf("p2p_register_info.strRemoteIp = %s\n", p2p_register_info.strRemoteIp);
	#endif
	strcpy(p2p_register_info.strUser, param.strUser);
	strcpy(p2p_register_info.strPasswd, param.strPasswd);
	#endif
	NETSDK_NatPause();
	memset(buffer, 0, 1000);
	memcpy(buffer, &p2p_register_info, sizeof(DVSNET_REGISTER_INFO));
	
	if(g_nat_p2p_type_flag == 0)
		NETSDK_NatSetup_P2P(param.sServerUrl, param.nPort, param.nInterval, buffer, sizeof(DVSNET_REGISTER_INFO));
	else
		NETSDK_NatSetup_udp_P2P(param.sServerUrl, param.nPort, param.nInterval, buffer, sizeof(DVSNET_REGISTER_INFO));

	if (param.bEnabled)
	{
		NETSDK_NatResume();
	}

	ret = setP2PParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_setP2P_ENDCONBACK(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	P2P_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(P2P_PARAM))
	{
		printf("Length of Parameter Error(P2P_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(P2P_PARAM));

	ret = setP2PBACKParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}


static int SESSION_getPPPOE(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PPPOE_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getPPPOEParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PPPOE_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PPPOE_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PPPOE_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setPPPOE(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PPPOE_PARAM param;
	NET_PARAM netParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(PPPOE_PARAM))
	{
		printf("Length of Parameter Error(PPPOE_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PPPOE_PARAM));

	/*
	getNetParam(&netParam);

	PPPOE_Open();
	PPPOE_Setup(param.userName, param.userPsw, netParam.byDnsAddr, netParam.byDnsAddr);
	if (param.nOnFlag)
	{
		PPPOE_Start();
	}
	else
	{
		PPPOE_Stop();
	}
	*/
	
	ret = setPPPOEParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}


static int SESSION_getYIYUAN_DDNS(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	YIYUAN_ALARM_DDNS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getYiyuanAlarmDDNSParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	printf("get yiuan ddsn param %d %d %d %s\n", param.nOnFlag, param.nPort, param.nTime, param.sIpAddr);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(YIYUAN_ALARM_DDNS_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(YIYUAN_ALARM_DDNS_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(YIYUAN_ALARM_DDNS_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}


static int SESSION_setYIYUAN_DDNS(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	YIYUAN_ALARM_DDNS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(YIYUAN_ALARM_DDNS_PARAM))
	{
		printf("Length of Parameter Error(YIYUAN_ALARM_DDNS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(YIYUAN_ALARM_DDNS_PARAM));

	ret = setYiyuanAlarmDDNSParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	printf("yiuan ddsn param %d %d %d %s\n", param.nOnFlag, param.nPort, param.nTime, param.sIpAddr);
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}


static int SESSION_getTimeReboot(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TIME_REBOOT_PARAM param;
		
	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getTimeRebootParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	//printf("get time reboot::%d \n", param.bEnable);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(TIME_REBOOT_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(TIME_REBOOT_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(TIME_REBOOT_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}


static int SESSION_setTimeReboot(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TIME_REBOOT_PARAM param;

	char buffer[1000] = {0};

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(TIME_REBOOT_PARAM))
	{
		printf("Length of Parameter Error(TIME_REBOOT_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(TIME_REBOOT_PARAM));
	ret = setTimeRebootParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	set_time_flag();
	//printf("set time reboot::%d %d\n", param.bEnable, param.nRebootInterval);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	return ret;
}


static int SESSION_getAlarmStatus(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	ALARM_STATUS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getAlarmStatusParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	printf("alarm status %d %d %d\n", param.nAlarmInStatus, param.nIrAlarmStatus[0], param.nMotionStatus);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(ALARM_STATUS_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(ALARM_STATUS_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(ALARM_STATUS_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setAlarmStatus(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	ALARM_STATUS_PARAM param;

	char buffer[1000] = {0};

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(ALARM_STATUS_PARAM))
	{
		printf("Length of Parameter Error(ALARM_STATUS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(ALARM_STATUS_PARAM));
	//printf("alarm status %d %d %d\n", param.nAlarmInStatus, param.nIrAlarmStatus[0], param.nMotionStatus);
		
	ret = setAlarmStatusParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	if(param.nAlarmInStatus&&param.nIrAlarmStatus[0]){
		setIrProbeAlarmStartParam();
		miscDrv_SetIndicator(1);
		probeNetAlarmStart();
	}
	else
	{
		setIrProbeAlarmStopParam();
		probeAlarmStop();
		probeNetAlarmStop();
	}
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

#if 0
	devCmdHeader.nCmdID = SEND_ALARMSTATUS_PARAM;
	devCmdHeader.nChannel = 0;
	devCmdHeader.nCmdLen = sizeof(ALARM_STATUS_PARAM);

	memcpy(buffer, &devCmdHeader, sizeof(DEV_CMD_HEADER));
	memcpy(buffer+sizeof(ALARM_STATUS_PARAM), &param, sizeof(DEV_CMD_HEADER));
	ret = NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(ALARM_STATUS_PARAM));
#endif

//	printf("########NETSDK_SendAllMsg %x\n\n", buffer);
	
	return ret;
}


static int SESSION_setAllIrAlarmStatus(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int i = 0;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	ALARM_STATUS_PARAM param;

	char buffer[1000] = {0};

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(ALARM_STATUS_PARAM))
	{
		printf("Length of Parameter Error(ALARM_STATUS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(ALARM_STATUS_PARAM));
	printf("alarm status %d %d %d \n", param.nAlarmInStatus, param.nIrAlarmStatus[0], param.nMotionStatus);
	if(param.nAlarmInStatus == 1){
		for(i = 0; i < 12; i++){
			param.nIrAlarmStatus[i] = 1;
		}
	}
	else if(param.nAlarmInStatus == 0){
		for(i = 0; i < 12; i++){
			param.nIrAlarmStatus[i] = 0;
		}
	}
	ret = setAlarmStatusParam(&param);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr	=  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	devCmdHeader.nCmdID = SEND_ALARMSTATUS_PARAM;
	devCmdHeader.nChannel = 0;
	devCmdHeader.nCmdLen = sizeof(ALARM_STATUS_PARAM);

	memcpy(buffer, &devCmdHeader, sizeof(DEV_CMD_HEADER));
	memcpy(buffer+sizeof(ALARM_STATUS_PARAM), &param, sizeof(DEV_CMD_HEADER));
	ret = NETSDK_SendAllMsg(buffer, sizeof(DEV_CMD_HEADER)+sizeof(ALARM_STATUS_PARAM));
	
	return ret;
}



static int SESSION_getDDNS(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DDNS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getDDNSParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DDNS_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DDNS_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DDNS_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}


static int SESSION_setDDNS(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DDNS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(DDNS_PARAM))
	{
		printf("Length of Parameter Error(DDNS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DDNS_PARAM));
	printf("param = %d\n", param.authType);
	ret = setDDNSParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	set_register_ddns();

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

int SESSION_getUpnp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_UPNP_PARAM param;
	UPNP_WIFI_IP_PORT_INFO ip_wifi_upnp_param;
	char upnpCmd[256];


	NET_PARAM net_param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	get_ip_wifi_configure(&ip_wifi_upnp_param);
	param.bEnabled = ip_wifi_upnp_param.bEnabled;
	param.upnpEthNo = ip_wifi_upnp_param.upnpEthNo;
	param.upnpWebPort = ip_wifi_upnp_param.nWebPort;
	param.upnpDataPort = ip_wifi_upnp_param.nDataPort;
	param.upnpRtspPort = ip_wifi_upnp_param.nRtspPort;
	param.upnpWebStatus = ip_wifi_upnp_param.upnpWebStatus;
	param.upnpDataStatus = ip_wifi_upnp_param.upnpDataStatus;
	param.upnpRtspStatus = ip_wifi_upnp_param.upnpRtspStatus;
	param.nReserved = ip_wifi_upnp_param.nReserved;

#if 0
	ret = getNewUpnpParam(&param);
	if (ret < 0)
	{
		return -1;
	}
#endif


	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DVSNET_UPNP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DVSNET_UPNP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	printf("get upnp parameter:: param.upnpDataPort = %d param.upnpWebPort = %d param.upnpDataStatus = %d, param.upnpWebStatus = %d , param.upnpRtspStatus= %d\n\n", param.upnpDataPort, param.upnpWebPort,param.upnpDataStatus ,param.upnpWebStatus,  param.upnpRtspStatus);
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DVSNET_UPNP_PARAM));
	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	return ret;
}

int SESSION_setUpnp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int nwifi_status_flag = 0;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_UPNP_PARAM param;
	DVSNET_UPNP_PARAM upnp_param;
	DVSNET_RTSP_PARAM rtsp_param;
	WIFI_PARAM wifiParam;
	
	//UPNP_PORTMAPPING_INFO upnp_portmapping_param;
	UPNP_PORT_INFO upnp_port_param;
	char upnpCmd[256];
	char return_buffer[1024];
	NET_PARAM net_param;
	

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(DVSNET_UPNP_PARAM))
	{
		printf("Length of Parameter Error(DDNS_PARAM)!%d, %d\n", devCmdHeader.nCmdLen, sizeof(DVSNET_UPNP_PARAM));
	//	return ;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_UPNP_PARAM));

	getWifiParam(&wifiParam);
	getRtspParam(&rtsp_param);
	ret = getNewUpnpParam(&upnp_param);
	if (ret < 0)
	{
		return -1;
	}
	upnp_param.upnpWebStatus = 1;
	upnp_param.upnpDataStatus = 1;
	upnp_param.upnpWebPort= param.upnpWebPort;
	upnp_param.upnpDataPort = param.upnpDataPort;
	upnp_param.bEnabled = param.bEnabled;
	nwifi_status_flag =	get_wifi_status();
	
#if 1
	if(wifiParam.nOnFlag&&(nwifi_status_flag == 1))
	{
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
		if(system(upnpCmd) !=  0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpWebStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpWebStatus = 1;
		}
		upnp_param.bEnabled = 1;
		setNewUpnpParam(&upnp_param);
		usleep(1000*10);
	
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpDataStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpDataStatus = 1;
		}
			upnp_param.bEnabled = 1;
		setNewUpnpParam(&upnp_param);
			usleep(1000*10);
	
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", wifiParam.byServerIp, rtsp_param.nRtspPort , upnp_param.upnpRtspPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpRtspStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpRtspStatus = 1;
		}
			upnp_param.bEnabled = 1;
		setNewUpnpParam(&upnp_param);
			usleep(1000*10);

		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wWebPort, upnp_param.upnpWebPort);
		if(system(upnpCmd) !=  0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpWebStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpWebStatus = 1;
		}
		usleep(1000*10);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, net_param.wServerPort, upnp_param.upnpDataPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpDataStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpDataStatus = 1;
		}
		usleep(1000*10);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", wifiParam.byServerIp, rtsp_param.nRtspPort, upnp_param.upnpRtspPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.nReserved = 0;
			upnp_param.upnpRtspStatus = 0;
		}
		else{
			upnp_param.nReserved = 1;
			upnp_param.upnpRtspStatus = 1;
		}
	}
	else
	{
		getNetParam(&net_param);
		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wWebPort, param.upnpWebPort);
		if(system(upnpCmd) !=  0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpWebStatus = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wServerPort, param.upnpDataPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpDataStatus = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d UDP", net_param.byServerIp, net_param.wServerPort, rtsp_param.nRtspPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpDataStatus = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wWebPort, param.upnpWebPort);
		if(system(upnpCmd) !=  0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpWebStatus = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, param.upnpDataPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpDataStatus = 0;
		}
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, net_param.wServerPort, rtsp_param.nRtspPort);
		if(system(upnpCmd) != 0 ){
			printf("upnpc portmapping error = %s\n", upnpCmd);
			upnp_param.upnpDataStatus = 0;
		}
		//upnp_web_resume();
	}

ret = setNewUpnpParam(&upnp_param);
if (ret < 0)
{
	return -1;
}
#endif
	
	printf("upnp_param.bEnabled = %d; param.upnpWebStatus = %d, param.upnpDataStatus= %d\n\n", upnp_param.bEnabled ,param.upnpDataStatus , param.upnpWebStatus);
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

int SESSION_getP2P(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_P2P_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getP2PParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DVSNET_P2P_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DVSNET_P2P_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DVSNET_P2P_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

int SESSION_setP2P(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_P2P_PARAM param;
	NET_PARAM net_param;
	DVSNET_REGISTER_INFO p2p_param;
	UPNP_PORT_INFO upnp_port_param;
	UPNP_PORTMAPPING_INFO upnp_portmapping_param;
	char upnpCmd[256];
	char buffer[1000];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(DVSNET_P2P_PARAM))
	{
		printf("Length of Parameter Error(DDNS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_P2P_PARAM));
	
	if(param.bEnabled){
		NETSDK_NatPause();
		getNetParam(&net_param);
		
		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, 2400, 10000);
		printf("upnpCmd = %s\n", upnpCmd);
		system(upnpCmd);
		if(system(upnpCmd) == -1){
			upnp_portmapping_param.nWebPortMappingStatus = 0;
			printf("upnpc portmapping error = %s\n", upnpCmd);
		}

		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, 554, 10004);
		printf("upnpCmd = %s\n", upnpCmd);
		if(system(upnpCmd) == -1){
			upnp_portmapping_param.nWebPortMappingStatus = 0;
			printf("upnpc portmapping error = %s\n", upnpCmd);
		}
		
		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, 3000, 10001);
		printf("upnpCmd = %s\n", upnpCmd);
		if(system(upnpCmd) == -1){
			upnp_portmapping_param.nWebPortMappingStatus = 0;
			printf("upnpc portmapping error = %s\n", upnpCmd);
		}

		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, 3001, 10002);
		printf("upnpCmd = %s\n", upnpCmd);
		if(system(upnpCmd) == -1){
			upnp_portmapping_param.nWebPortMappingStatus = 0;
			printf("upnpc portmapping error = %s\n", upnpCmd);
		}

		memset(upnpCmd, 0, 256);
		sprintf(upnpCmd, "upnpc -a %s %d %d TCP", net_param.byServerIp, 80, 10003);
		printf("upnpCmd = %s\n", upnpCmd);
		if(system(upnpCmd) == -1){
			upnp_portmapping_param.nWebPortMappingStatus = 0;
			printf("upnpc portmapping error = %s\n", upnpCmd);
		}
		getUSERP2PPORTConfigure(&upnp_port_param);
		
		strcpy(upnp_portmapping_param.strRemoteIp, upnp_port_param.strRemoteIp);
		upnp_portmapping_param.wRemoteDataPort = 3000;
		upnp_portmapping_param.wRemotePamerSetPort = 3001;
		upnp_portmapping_param.wRemoteWebPort = 2400;
		upnp_portmapping_param.wRemoteMediaPort = 554;
		upnp_portmapping_param.nOnFlag = 1;
		upnp_portmapping_param.nWebPortMappingStatus = 1;
		
		#if 0
		//register p2p server 
		strcpy(p2p_param.strLocalIp, net_param.byServerIp);
		strcpy(p2p_param.wLocalPort, net_param.wServerPort);

		strcpy(p2p_param.strRemoteIp , param.sServerUrl);
		p2p_param.wRemotePort = param.nPort;
		strcpy(p2p_param.strUser, param.strUser);
		strcpy(p2p_param.strPasswd,param.strPasswd);

		p2p_param.dwUpnp = 1;
		p2p_param.dwUpnpWebPort = upnp_port_param.wLocalPort;
		printf("starting in p2p register .....\n");
		memcpy(buffer, &p2p_param, sizeof(DVSNET_REGISTER_INFO));
		NETSDK_NatSetup(param.sServerUrl, param.nPort, param.nInterval, buffer, sizeof(DVSNET_REGISTER_INFO));
		
		NETSDK_NatResume();
		#endif
	}
			
	ret = setP2PParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}


int SESSION_DVSNET_REGISTER(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_REGISTER_INFO param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(DVSNET_REGISTER_INFO))
	{
		printf("Length of Parameter Error(DDNS_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_REGISTER_INFO));

	ret = setP2PRegisterParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getWifi(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	WIFI_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	
	ret = getWifiParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	//param.Reserve = 1;param
	
	printf("param.Reserve  = %d %s\n\n", param.Reserve, param.byServerIp);
	

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(WIFI_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(WIFI_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(WIFI_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setWifi(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	WIFI_PARAM param;
	NET_PARAM net_param;
	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));


	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(WIFI_PARAM));
	getNetParam(&net_param);
	param.Reserve = 1;
	if(param.pReserve[0]){
		strcpy(param.byServerIp, net_param.byServerIp);
		strcpy(param.byDnsAddr, net_param.byDnsAddr);
		strcpy(param.byGateway, net_param.byGateway);
		strcpy(param.byServerMask, net_param.byServerMask);
				
	}
	
	
	ret = setWifiParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}



static int SESSION_scanWifi(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[sizeof(WIFI_DEVICE_ALL)*NET_MAX_AP_NUMBER];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	WIFI_PARAM param;
	WIFI_DEVICE_ALL wifi_buffer;
	

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	system("/mnt/mtd/dvs/wlan/iwscan > /param/wifi.conf");
	if(Wifi_Param_Struct(&wifi_buffer)== NULL)
	{
		perror("get wifi param error\n");
		return -1;
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(WIFI_DEVICE_ALL);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(WIFI_DEVICE_ALL);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &wifi_buffer, sizeof(WIFI_DEVICE_ALL));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	return ret;
}

static int SESSION_getPTZ(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;

	}

	ret = getPtzParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PTZ_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PTZ_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PTZ_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setPTZ(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_PARAM param;
	COM_PARAM com;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(PTZ_PARAM))
	{
		printf("Length of Parameter Error(PTZ_PARAM)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZ_PARAM));

	ret = setChnPtzExt(devCmdHeader.nChannel, param.strName, param.nAddr, param.nRate);
	if (ret < 0)
	{
		return -1;
	}

	ret = setPtzParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	// 
	ret = getPtzRs485Com(devCmdHeader.nChannel, &com);
	if (ret == 0)
	{
		// Add the code by lvjh, 2009-02-120
		com.nReserve = 0;
		setRs485Param(devCmdHeader.nChannel, &com);
		/*
		printf("RS485 Parameter:\n");
		printf("nBaudRate: %d\n", com.nBaudRate);
		printf("nDataBits: %d\n", com.nDataBits);
		printf("nParity: %d\n", com.nParity);
		printf("nStopBits: %d\n", com.nStopBits);
		printf("nFlowCtrl: %d\n", com.nFlowCtrl);
		printf("nReserve: %d\n", com.nReserve);
		*/
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);


	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getRs485COM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	COM_PARAM param;

	//printf("SESSION_getRs485COM ...\n");

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getRs485Param(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-02-12
	param.nReserve = 0;
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(COM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(COM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(COM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setRs485COM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	COM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(COM_PARAM))
	{
		printf("Length of Parameter Error(COM_PARAM)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(COM_PARAM));

	ret = setPtzRs485Com(devCmdHeader.nChannel, param);
	if (ret < 0)
	{
		return -1;
	}

	// Add the code by lvjh, 2009-02-12
	param.nReserve = 1;
	
	ret = setRs485Param(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getRs232COM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	COM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getRs232Param(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(COM_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(COM_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(COM_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setRs232COM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	COM_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(COM_PARAM))
	{
		printf("Length of Parameter Error(COM_PARAM)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(COM_PARAM));

	ret = setPtzRs232Com(devCmdHeader.nChannel, param);
	if (ret < 0)
	{
		return -1;
	}

	ret = setRs232Param(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_rs485Through(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZCTRLCMD_THROUGH param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(COM_PARAM))
	{
		printf("Length of Parameter Error(COM_PARAM)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZCTRLCMD_THROUGH));

	//ret = ptzControlThrough(devCmdHeader.nChannel, 0, param);
	ret = ptzControlThroughExt(devCmdHeader.nChannel, 0, param);	// Add the code by lvjh, 2011-01-11
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_rs232Through(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZCTRLCMD_THROUGH param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < sizeof(COM_PARAM))
	{
		printf("Length of Parameter Error(COM_PARAM)!\n");
		return -1;
	}

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZCTRLCMD_THROUGH));

	//ret = ptzControlThrough(devCmdHeader.nChannel, 1, param);
	ret = ptzControlThroughExt(devCmdHeader.nChannel, 1, param);	// Add the code by lvjh, 2011-01-11
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getRemoteTalkIP(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	char param[16];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getRemoteTalkIPParam(param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+16;

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 16;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), param, 16);

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setRemoteTalkIP(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	char param[16];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nCmdLen < 16)
	{
		printf("Length of Parameter Error(REMOTETALK_IP_PARAM)!\n");
		return -1;
	}

	memcpy(param, pRecvBuf+sizeof(DEV_CMD_HEADER), 16);

	ret = setRemoteTalkIPParam(param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getSysTime(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DATE_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getSystemTimeExt(&param.year, &param.month, &param.day, &param.week, &param.hour, &param.minute, &param.second);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DATE_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DATE_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DATE_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setSysTime(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;

	DEV_CMD_RETURN devCmdReturn;
	DATE_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nCmdLen < sizeof(DATE_PARAM))
	{
		//printf("Length of Parameter Error(DATE_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DATE_PARAM));

	ret = rtcCalibration(param);
	if (ret < 0)
	{
		//return -1;
	}
	ret = setSystemTime(param.year, param.month, param.day, param.hour, param.minute, param.second);
	if (ret < 0)
	{
		return -1;
	}
    
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	printf("SESSION_setSysTime: %d\n", ret);
	return ret;
}

static int SESSION_getNtp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	NTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getNtpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(NTP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(NTP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(NTP_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setNtp(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;

	DEV_CMD_RETURN devCmdReturn;
	NTP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nCmdLen < sizeof(NTP_PARAM))
	{
		printf("Length of Parameter Error(DATE_PARAM)!\n");
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(NTP_PARAM));

	printf("SESSION_setNtp(%d)...\n", param.nOnFlag);

	ret = ntp_setup(param);
	if (ret < 0)
	{
		//return -1;
	}

	ret = setNtpParam(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getPTZList(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[4096];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_INDEX param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	ret = getPtzList(&param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PTZ_INDEX);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PTZ_INDEX);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PTZ_INDEX));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_AddPTZ(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_DATA param;
	char *ptzData = NULL;		// Add the code by lvjh, 2009-06-02

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	if (devCmdHeader.nCmdLen < sizeof(PTZ_DATA))
	{
		printf("Length of Parameter Error(PTZ_DATA)!\n");
		return -1;
	}
	
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZ_DATA));
	ptzData = pRecvBuf+sizeof(DEV_CMD_HEADER)+sizeof(PTZ_DATA);		// Add the code by lvjh, 2009-06-02

	//ret = addPtz(param.strName, param.strData, param.nSize);
	ret = addPtz(param.strName, ptzData, param.nSize);		// Add the code by lvjh, 2009-06-02
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_DeletePTZ(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	char param[128];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	if (devCmdHeader.nCmdLen > 128)
	{
		printf("Length of Parameter Error(PTZ_DATA)!\n");
		return -1;
	}
	
	memcpy(param, pRecvBuf+sizeof(DEV_CMD_HEADER), devCmdHeader.nCmdLen);

	ret = deletePtz(param);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = ret;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_CtrlPTZ(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_CMD param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZ_CMD));


	
	ret = ptzControl(devCmdHeader.nChannel, param);
	if (ret < 0)
	{
		return -1;
	}
	//printf("######%d %d\n\n", param.nCmd, param.nValue);
	

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_ManualSnapshop(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	//printf("SESSION_ManualSnapshop: %d\n", pMsgHead->nSock);

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	//g_jpeg_manual_socket[devCmdHeader.nChannel] = pMsgHead->nSock;
	SNAPSHOT_Manual(pMsgHead->nSock);
	videoJpegSnapShot(devCmdHeader.nChannel, SPECIAL_IP_MANUAL_UPLOAD);
	manual_record_result(DEV_SNAPSHOT_RESULT);
				

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	return ret;
}

static int SESSION_getProbeStatus(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_STATUS param;
	unsigned long tmp;
	unsigned long probeIn;
	unsigned long probeOut;

    if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	for (i=0; i<MAX_PROBE_IN; i++)
	{
		tmp = getProbeInStatus(i);
		probeIn += tmp;
	}
	for (i=0; i<MAX_PROBE_OUT; i++)
	{
		tmp = getProbeOutStatus(i);
		probeOut += tmp;
	}

	param.status = probeIn<<16 + probeOut;

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PROBE_STATUS);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PROBE_STATUS);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PROBE_STATUS));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setProbeStatus(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PROBE_STATUS param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PROBE_STATUS));

	for (i=0; i<MAX_PROBE_IN; i++)
	{
		manualSetProbeOutStatus(i, (param.status>>i)&0x01);
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_SaveSysParam(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;

    if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	saveParamToFile();

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	//printf("Parameter Save!\n");

	return ret;
}

static int SESSION_ResumeDefaultParam(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	setParamStatusToFile(SOFTWARE_RESET);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	devCmdReturn.nCmdID = DEV_NOTIFY_INFO;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendAllMsg((char *)&devCmdReturn, sizeof(DEV_CMD_RETURN));

	RebootSystem();

	return ret;
}


static int SESSION_RebootDev(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
    DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

    memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
    
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	//
	devCmdReturn.nCmdID = DEV_NOTIFY_INFO;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendAllMsg((char *)&devCmdReturn, sizeof(DEV_CMD_RETURN));

	RebootSystem();

	return ret;
}

static int SESSION_getVideoInStandard(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_STANDARD_PARAM videoStdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoInStandardParam(devCmdHeader.nChannel, &videoStdParam);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_STANDARD_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_STANDARD_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &videoStdParam, sizeof(VIDEO_STANDARD_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setVideoInStandard(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_STANDARD_PARAM oldVideoStd;
	VIDEO_STANDARD_PARAM newVideoStd;
	int standard = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

#ifdef CCD
	memcpy(&newVideoStd, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_STANDARD_PARAM));

	getVideoInStandardParam(devCmdHeader.nChannel, &oldVideoStd);
	standard = newVideoStd.nStandard;

	printf("Video Standard: %d(old: %d)\n", newVideoStd.nStandard, oldVideoStd.nStandard);

	if (oldVideoStd.nStandard != newVideoStd.nStandard)
	{
		int i = 0;
		VENC_PARAM param;
		
		for (i=0; i<MAX_CHANNEL_ENC_NUM; i++)
		{
			memset(&param, 0, sizeof(VENC_PARAM));
			getVideoEncParam(devCmdHeader.nChannel, i, &param);
			
			switch (param.nEncodeHeight)
			{
			case 576:
			case 480:
				if (standard)
				{
					param.nEncodeHeight = 480;
				}
				else
				{
					param.nEncodeHeight = 576;
				}
				break;
	
			case 288:
			case 240:
				if (standard)
				{
					param.nEncodeHeight = 240;
				}
				else
				{
					param.nEncodeHeight = 288;
				}
				break;
	
			case 144:
			case 120:
			case 112:
			case 128:
				if (standard)
				{
					param.nEncodeHeight = 128;
				}
				else
				{
					param.nEncodeHeight = 144;
				}
				break;
	
			default:
				if (standard)
				{
					param.nEncodeHeight = 240;
				}
				else
				{
					param.nEncodeHeight = 288;
				}
				break;
			}
			setVideoEncParam(devCmdHeader.nChannel, i, &param);
		}
		
		vadcDrv_SetStandard(devCmdHeader.nChannel, newVideoStd.nStandard);
		ret = setVideoInStandardParam(devCmdHeader.nChannel, &newVideoStd);
		
		memset(&param, 0, sizeof(VENC_PARAM));
		getVideoEncParam(devCmdHeader.nChannel, 0, &param);
		setVideoEnc(devCmdHeader.nChannel, 0, param);
	}
#else

#endif

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;		
}

static int SESSION_getVideoEnc(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VENC_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoEncParam(devCmdHeader.nChannel, devCmdHeader.nReserve, &param);
	if (ret < 0)
	{
		return -1;
	}

#if 0	
	param.nEncodeWidth  = 640;
	param.nEncodeHeight = 480;
#endif

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VENC_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VENC_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VENC_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

int setVideoEnc(int nChannel, int nStreamType, VENC_PARAM param)
{
	int nRet = 0;
	VENC_PARAM primaryParam;
	VENC_PARAM secondaryParam;
	VIDEO_ENC_PARAM vencParam;
	int standard = 0;
	VIDEO_STANDARD_PARAM videoStd;
	
	int i = 0;
	OSD_PARAM osd;
	LOGO_PARAM logo;
	MASK_PARAM mask;
	VIDEO_MOTION_PARAM motion;
	char logoData[MAX_LOGO_DATA];

#ifdef RECORD
	int nPauseFlag = 0;
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	if (nChannel<0 || nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>1)
	{
		return -1;
	}	
	if (param.nEncodeMode<0 || param.nEncodeMode>1)
	{
		return -1;
	}	
	if (param.nKeyInterval<=0 || param.nKeyInterval>1000)
	{
		return -1;
	}
	if (param.nFramerate<=0 || param.nFramerate>30)
	{
		return -1;
	}
	if (param.reserve == 1)	// MJPEG
	{
		if (param.nBitRate<1024000 || param.nBitRate>20480000)
		{
			return -1;
		}
	}
	else
	{
		if (param.nBitRate<16384 || param.nBitRate>20480000)
		{
			return -1;
		}
	}
	
	memset(&primaryParam, 0, sizeof(VENC_PARAM));
	memset(&secondaryParam, 0, sizeof(VENC_PARAM));
	
	if (nStreamType == 0)
	{
		memcpy(&primaryParam, &param, sizeof(VENC_PARAM));
		getVideoEncParam(nChannel, 1, &secondaryParam);
	}
	else
	{
		getVideoEncParam(nChannel, 0, &primaryParam);
		memcpy(&secondaryParam, &param, sizeof(VENC_PARAM));	
	}

#ifdef RECORD
	// record SDK
	cmdParam.nChannel = nChannel;
	cmdParam.nOpt = RSDKCMD_PAUSE_RECORD;
	RECORDSDK_Operate(&cmdParam, NULL, NULL);
	nPauseFlag = 1;
#endif

	videoEncModulePause(nChannel);
	
	usleep(100);
	sleep(1);
	
	videoEncStop(nChannel, 0);
	videoEncClose(nChannel, 0);
	
	usleep(100);
	
	// 主码流
#ifdef CCD
	getVideoInStandardParam(nChannel, &videoStd);	// Change the code by lvjh, 2009-05-27
	standard = videoStd.nStandard & 0x0F;
	videoInSetup(nChannel, 0x00000000|standard);
	
	if (standard)
	{
		if (primaryParam.nFramerate > 30)
		{
			primaryParam.nFramerate = 30;
		}
		if (primaryParam.nFramerate < 1)
		{
			primaryParam.nFramerate = 1;
		}
	}
	else
	{			
		if (primaryParam.nFramerate > 25)
		{
			primaryParam.nFramerate = 25;
		}
		if (primaryParam.nFramerate < 1)
		{
			primaryParam.nFramerate = 1;
		}
	}
	switch (primaryParam.nEncodeHeight)
	{
	case 576:
	case 480:
		if (standard)
		{
			primaryParam.nEncodeWidth = 720;
			primaryParam.nEncodeHeight = 480;
		}
		else
		{
			primaryParam.nEncodeWidth = 720;
			primaryParam.nEncodeHeight = 576;
		}
		break;

	case 288:
	case 240:
		if (standard)
		{
			primaryParam.nEncodeHeight = 240;
		}
		else
		{
			primaryParam.nEncodeHeight = 288;
		}
		break;
		
	case 144:
	case 120:
	case 128:
	case 112:
		if (standard)
		{
			primaryParam.nEncodeWidth = 176;
			primaryParam.nEncodeHeight = 128;
		}
		else
		{
			primaryParam.nEncodeWidth = 176;
			primaryParam.nEncodeHeight = 144;
		}
		break;		

	default:
		if (standard)
		{
			primaryParam.nEncodeWidth = 352;
			primaryParam.nEncodeHeight = 240;
		}
		else
		{
			primaryParam.nEncodeWidth = 352;
			primaryParam.nEncodeHeight = 288;
		}
		break;
	}
#endif

#ifdef HD_CMOS
	printf("primaryParam.nEncodeHeight = %d\n", primaryParam.nEncodeHeight);
	switch (primaryParam.nEncodeHeight)
	{
	case 1200:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, UXVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x06000000);
			
			primaryParam.nEncodeWidth = 1600;
			primaryParam.nEncodeHeight = 1200;
		}
		break;

	case 720:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, XXVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x07000000);
			
			primaryParam.nEncodeWidth = 1280;
			primaryParam.nEncodeHeight = 720;
		}
		break;

		case 576:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, XXVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x08000000);
			
			primaryParam.nEncodeWidth = 720;
			primaryParam.nEncodeHeight = 576;
		}
			break;

		case 480:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, XXVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x09000000);
			
			primaryParam.nEncodeWidth = 640;
			primaryParam.nEncodeHeight = 480;
		}
		break;
		
		case 240:
		{
			printf("session 320X240\n");
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, QVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x05000000);
			
			primaryParam.nEncodeWidth = 320;
			primaryParam.nEncodeHeight = 240;
		}
		break;

	default:
		{
			VIDEO_FLIP_PARAM flipParam;
			VIDEO_MIRROR_PARAM mirrorParam;
			VIDEO_HZ_PARAM hzParam;
			VIDEO_IN_ATTR vinAttr;
			
			vadcDrv_SetImageFormat(nChannel, UXVGA);
			getVideoFlipParam(nChannel, &flipParam);
			vadcDrv_SetImageFlip(nChannel, flipParam.nFlip);
			getVideoMirrorParam(nChannel, &mirrorParam);
			vadcDrv_SetImageMirror(nChannel, mirrorParam.nMirror);
			getVideoHzParam(nChannel, &hzParam);
			vadcDrv_SetImageHz(nChannel, hzParam.nHz);
			
			// Add the code by lvjh, 2009-05-27
			getVideoInAttrParam(nChannel, &vinAttr);
			vadcDrv_SetBrightness(nChannel, vinAttr.nBrightness);
			vadcDrv_SetHue(nChannel, vinAttr.nHue);
			vadcDrv_SetContrast(nChannel, vinAttr.nContrast);
			vadcDrv_SetSaturation(nChannel, vinAttr.nSaturation);
			
			videoInSetup(nChannel, 0x06000000);
			
			primaryParam.nEncodeWidth = 1600;
			primaryParam.nEncodeHeight = 1200;
		}
		break;
	}
	if (primaryParam.nFramerate > 30)
	{
		primaryParam.nFramerate = 30;
	}
	if (primaryParam.nFramerate < 1)
	{
		primaryParam.nFramerate = 1;
	}
#endif
	
	usleep(100);

	//
	videoEncOpen(nChannel, 0);

	//
	memset(&vencParam, 0, sizeof(VIDEO_ENC_PARAM));
	memcpy(&vencParam.param.encParam, &primaryParam, sizeof(VENC_PARAM));

	nRet = videoEncSetup(nChannel, 0, venc_setup_enc_param, &vencParam);
	//setVideoFormat(i, 0, primaryParam.nEncodeWidth, primaryParam.nEncodeHeight);
	setVideoFormat(i, 0, primaryParam.nEncodeWidth, primaryParam.nEncodeHeight, primaryParam.reserve);
	
	// parameter
	nRet = setVideoEncParam(nChannel, 0, &primaryParam);
	if (nRet)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-03-26
	SetAVInfo();
		
#ifdef CCD
	if (standard)
	{
		if (secondaryParam.nFramerate > 30)
		{
			secondaryParam.nFramerate = 30;
		}
		if (secondaryParam.nFramerate < 1)
		{
			secondaryParam.nFramerate = 1;
		}
	}
	else
	{			
		if (secondaryParam.nFramerate > 25)
		{
			secondaryParam.nFramerate = 25;
		}
		if (secondaryParam.nFramerate < 1)
		{
			secondaryParam.nFramerate = 1;
		}
	}
	switch (secondaryParam.nEncodeHeight)
	{
	case 576:
	case 480:
		if (standard)
		{
			secondaryParam.nEncodeWidth = 720;
			secondaryParam.nEncodeHeight = 480;
		}
		else
		{
			secondaryParam.nEncodeWidth = 720;
			secondaryParam.nEncodeHeight = 576;
		}
		break;

	case 288:
	case 240:
		if (standard)
		{
			secondaryParam.nEncodeHeight = 240;
		}
		else
		{
			secondaryParam.nEncodeHeight = 288;
		}
		break;
		
	case 144:
	case 120:
	case 128:
	case 112:
		if (standard)
		{
			secondaryParam.nEncodeWidth = 176;
			secondaryParam.nEncodeHeight = 128;
		}
		else
		{
			secondaryParam.nEncodeWidth = 176;
			secondaryParam.nEncodeHeight = 144;
		}
		break;		

	default:
		if (standard)
		{
			secondaryParam.nEncodeWidth = 352;
			secondaryParam.nEncodeHeight = 240;
		}
		else
		{
			secondaryParam.nEncodeWidth = 352;
			secondaryParam.nEncodeHeight = 288;
		}
		break;
	}
#endif

#ifdef HD_CMOS

	if (primaryParam.nEncodeHeight == 1200)
	{
		switch (secondaryParam.nEncodeHeight)
		{
		case 592:
		case 600:
		case 608:
			{
				secondaryParam.nEncodeWidth = 800;
				secondaryParam.nEncodeHeight = 608;
			}
			break;
	
		case 300:
		case 304:
		case 288:
			{
				secondaryParam.nEncodeWidth = 400;
				secondaryParam.nEncodeHeight = 304;
			}
			break;
			
		default:
			{
				secondaryParam.nEncodeWidth = 800;
				secondaryParam.nEncodeHeight = 608;
			}
			break;
		}
	}
	if (primaryParam.nEncodeHeight == 720)
	{
		switch (secondaryParam.nEncodeHeight)
		{
		case 352:
			{
				secondaryParam.nEncodeWidth = 640;
				secondaryParam.nEncodeHeight = 352;
			#if YIYUAN_VIDEO
				secondaryParam.nEncodeWidth = 704;
				secondaryParam.nEncodeHeight = 576;
			#endif
			}
			break;
	
		case 176:
			{
				secondaryParam.nEncodeWidth = 320;
				secondaryParam.nEncodeHeight = 176;
				
			#if YIYUAN_VIDEO
					secondaryParam.nEncodeWidth = 352;
					secondaryParam.nEncodeHeight = 288;
			#endif
			}
			break;		
		
		default:
			{
				secondaryParam.nEncodeWidth = 640;
				secondaryParam.nEncodeHeight = 352;
				
#if YIYUAN_VIDEO
				secondaryParam.nEncodeWidth = 704;
				secondaryParam.nEncodeHeight = 576;
#endif
			}
			break;
		}
	}

	if (secondaryParam.nFramerate > 30)
	{
		secondaryParam.nFramerate = 30;
	}
	if (secondaryParam.nFramerate < 1)
	{
		secondaryParam.nFramerate = 1;
	}
#endif	
	memset(&vencParam, 0, sizeof(VIDEO_ENC_PARAM));
	memcpy(&vencParam.param.encParam, &secondaryParam, sizeof(VENC_PARAM));
	
	nRet = videoEncSetup(nChannel, 1, venc_setup_enc_param, &vencParam);
	//setVideoFormat(i, 1, secondaryParam.nEncodeWidth, secondaryParam.nEncodeHeight);
	setVideoFormat(i, 1, secondaryParam.nEncodeWidth, secondaryParam.nEncodeHeight, secondaryParam.reserve);
	
	// parameter
	nRet = setVideoEncParam(nChannel, 1, &secondaryParam);
	if (nRet)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-03-26
	SetAVInfo();

	videoEncStart(nChannel, 0);
	
	usleep(1000);

	videoEncModuleResume(nChannel);
		
	// OSD\LOGO\MASK
	//OSD
	#if 0
	getOsdParam(nChannel, &osd);
	memset(&vencParam, 0, sizeof(VIDEO_ENC_PARAM));	
	// TITLE
	vencParam.param.encOSD.nChannel = nChannel;
	vencParam.param.encOSD.nIndex = 2;
	vencParam.param.encOSD.nShow = osd.TitleOSD[0].bShow;	
	vencParam.param.encOSD.data.nxPos = primaryParam.nEncodeWidth-16-strlen(osd.TitleOSD[0].sTitle)*8;
	vencParam.param.encOSD.data.nyPos = primaryParam.nEncodeHeight-32;
	vencParam.param.encOSD.data.color = 0xFFFFFF;
	//set_title_osd_color(0, 0xFFFFFF);
	memcpy(vencParam.param.encOSD.data.data, osd.TitleOSD[0].sTitle, 16);
	vencParam.param.encOSD.data.data[15] = '\0';
	nRet = videoEncSetup(nChannel, 0, venc_setup_osd, &vencParam);
		
	memset(&vencParam, 0, sizeof(VIDEO_ENC_PARAM));	
	// TITLE
	vencParam.param.encOSD.nChannel = nChannel;
	vencParam.param.encOSD.nIndex = 2;
	vencParam.param.encOSD.nShow = osd.TitleOSD[0].bShow;	
	vencParam.param.encOSD.data.nxPos = secondaryParam.nEncodeWidth-16-strlen(osd.TitleOSD[0].sTitle)*8;
	vencParam.param.encOSD.data.nyPos = secondaryParam.nEncodeHeight-32;
	vencParam.param.encOSD.data.color = 0xFFFFFF;
	//set_title_osd_color(0, 0xFFFFFF);
	memcpy(vencParam.param.encOSD.data.data, osd.TitleOSD[0].sTitle, 16);
	vencParam.param.encOSD.data.data[15] = '\0';
	nRet = videoEncSetup(nChannel, 1, venc_setup_osd, &vencParam);
	#endif
	

	// LOGO
	getLogoParam(nChannel, &logo, logoData);
	memset(&vencParam, 0, sizeof(VIDEO_ENC_PARAM));	
	vencParam.param.encLogo.nChannel = nChannel;
	vencParam.param.encLogo.nIndex = 0;
	vencParam.param.encLogo.nShow = logo.bShow;
	vencParam.param.encLogo.data.nxPos = (logo.x/16)*16;
	vencParam.param.encLogo.data.nyPos = (logo.y/16)*16;
	vencParam.param.encLogo.data.nWidth = logo.nWidth;
	vencParam.param.encLogo.data.nHeight = logo.nHeight;
	if (logo.nDataLen<MAX_LOGO_DATA  && logo.nDataLen>0)
	{
		vencParam.param.encLogo.data.data = logoData;
	}
	else
	{
		vencParam.param.encLogo.data.data = NULL;
	}
	nRet = videoEncSetup(nChannel, 0, venc_setup_logo, &vencParam);

	// MASK
	getMaskParam(nChannel, &mask);

	nRet = videoInSetMask(nChannel, mask.VideoMask[0]);

	// MOTION
	getVideoMotionParam(nChannel, &motion);
	memcpy(&vencParam.param.mdParam, &motion, sizeof(VIDEO_MOTION_PARAM));

	videoEncSetup(nChannel, 0, venc_setup_md_area, &vencParam);
	vencParam.param.nMD = 1;
	videoEncSetup(nChannel, 0, venc_setup_md, &vencParam);		

#ifdef RECORD
	// record SDK
	if (nPauseFlag)
	{
		// Add the code by lvjh, 2009-01-03
		int nSize = 0;
		RECORD_CHANNEL_PARAM recordChannelParam;
			
		memset(&recordChannelParam, 0, sizeof(RECORD_CHANNEL_PARAM));
			
		cmdParam.nChannel = nChannel;
		cmdParam.nOpt = RSDKCMD_GET_RECORD_PARAM;
		RECORDSDK_Operate(&cmdParam, &recordChannelParam, &nSize);
			
		recordChannelParam.avFormat.nImageHeight = param.nEncodeHeight;
		recordChannelParam.avFormat.nImageWidth = param.nEncodeWidth;
		setRecord(nChannel, recordChannelParam);
			
		cmdParam.nChannel = nChannel;
		cmdParam.nOpt = RSDKCMD_RESUME_RECORD;
		RECORDSDK_Operate(&cmdParam, NULL, NULL);
			
		nPauseFlag = 0;
	}
#endif
	
	return 0;
}


static int SESSION_setVideoEnc(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VENC_PARAM oldParam;
	VENC_PARAM param;
	int standard = 0;
	VIDEO_STANDARD_PARAM oldVideoStd;
	VIDEO_ENC_PARAM vencParam;

#ifdef RECORD
	int nPauseFlag = 0;
	RECORDSDK_CMD_PARAM cmdParam;
#endif

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	printf("[DEBUG]: setVideoEnc(%d %d) ...\n", devCmdHeader.nChannel, devCmdHeader.nReserve);
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		//return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VENC_PARAM));
	
	// Add the code by lvjh, 2009-04-22
	ret = setVideoEnc(devCmdHeader.nChannel, devCmdHeader.nReserve, param);
	printf("[DEBUG]: setVideoEnc(%d %d): %d\n", devCmdHeader.nChannel, devCmdHeader.nReserve, ret);
	
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);	
	
	return ret;
}

static int SESSION_getVideoWM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	return -1;
}

static int SESSION_setVideoWM(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	return 0;
}

static int SESSION_getVideoOSD(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	OSD_PARAM param;

	if (pRecvBuf == NULL)
	{

		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getOsdParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(OSD_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(OSD_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(OSD_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setVideoOSD(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	OSD_PARAM param;
	VIDEO_ENC_PARAM encParam;
	VENC_PARAM vencparam;
	char return_buffer[1024];

	char buffer[64];

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(OSD_PARAM));

//	printf("param = %d %s\n", param.TitleOSD[0].bShow, param.TitleOSD[0].sTitle);
//	printf("param = %d %s\n", param.TitleOSD[1].bShow,  param.TitleOSD[1].sTitle);
	
	// OSD
	//videoOsdTimePause(devCmdHeader.nChannel);

	// DATE
	encParam.param.encOSD.nChannel = devCmdHeader.nChannel;
	encParam.param.encOSD.nIndex = 0;
	/*
	if (param.TimeOSD.nFormat >= 1)
	{
		encParam.param.encOSD.nShow = 1;
	}
	else
	{
		encParam.param.encOSD.nShow = 0;
	}
	*/
	//encParam.param.encOSD.data.nxPos = (param.TimeOSD.x/16)*16;
	//encParam.param.encOSD.data.nyPos = (param.TimeOSD.y/16)*16;
	//memset(encParam.param.encOSD.data.data, 0, 32);
	//vencParam.param.encOSD.data.nColor = osd.TimeOSD.nColor;
	//videoEncSetup(devCmdHeader.nChannel, 0, venc_delete_osd, &encParam);	// Add the code by lvjh, 2008-02-24
	//set_time_osd_color(param.TimeOSD.nColor);
	
	// Delete the code by lvjh, 2010-02-22
	//set_time_osd_color(0xFFFFFF);
	//videoTimeOSD(devCmdHeader.nChannel, param.TimeOSD.bShow);
	
	//ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_osd, &encParam);

	//videoOsdTimeResume(devCmdHeader.nChannel);

	// BITRATES
	//encParam.param.encOSD.nChannel = devCmdHeader.nChannel;
	//encParam.param.encOSD.nIndex = 1;
	//encParam.param.encOSD.nShow = param.BitsOSD.bShow;
	//encParam.param.encOSD.data.nxPos = param.BitsOSD.x;
	//encParam.param.encOSD.data.nyPos = param.BitsOSD.y;
	//videoEncSetup(devCmdHeader.nChannel, 0, venc_delete_osd, &encParam);	// Add the code by lvjh, 2008-02-24
	
	// Delete the code by lvjh, 2010-02-22
	//set_bits_osd_color(param.BitsOSD.nColor);
	//set_bits_osd_color(0xFFFFFF);
	//videoBitRateOSD(devCmdHeader.nChannel, param.BitsOSD.bShow);
	
	//ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_osd, &encParam);

	// Add the code by lvjh, 2010-02-22
	printf("param.BitsOSD.x param.BitsOSD.y = %d %d\n", param.BitsOSD.x, param.BitsOSD.y);
	if((abs((param.TimeOSD.y - param.BitsOSD.y)) <= 19)||(abs((param.TimeOSD.y - param.TitleOSD[0].y)) <= 19 )||(abs((param.BitsOSD.y - param.TitleOSD[0].y)) <= 19))
	{
		printf("osd set error ,return success!\n");
		returnMsg.nSock = pMsgHead->nSock;
		returnMsg.addr	=  pMsgHead->addr;
		returnMsg.nflag = pMsgHead->nflag;
		returnMsg.nCmd = NETCMD_USER_CMD;
		returnMsg.nRight = pMsgHead->nRight;
		returnMsg.nErrorCode = 0;
		returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
		
		devCmdReturn.nCmdID = devCmdHeader.nCmdID;
		devCmdReturn.nCmdLen = 0;
		devCmdReturn.nResult = 0;
		devCmdReturn.nReserve = 0;
		
		ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
		
		// Add the code by lvjh, 2009-09-17
		devCmdReturn.nCmdID = DEV_TITLE_CHANGE;
		devCmdReturn.nCmdLen = sizeof(TITLE_OSD);
		devCmdReturn.nResult = 0;
		devCmdReturn.nReserve = 0;
			
		memset(return_buffer, 0, 1024);
		memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
		memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param.TitleOSD[0], sizeof(TITLE_OSD));
				
		NETSDK_SendAllMsg(&return_buffer, sizeof(DEV_CMD_RETURN)+sizeof(TITLE_OSD));
		return ret;
	}
	else {	
		
		videoTimeOSDExt(devCmdHeader.nChannel, param);

		// TITLE
		for (i=0; i<2; i++)
		{
			//printf("osd = %d param.TitleOSD[i].x = %d param.TitleOSD[i].y = %d\n", i, param.TitleOSD[i].x, param.TitleOSD[i].y);
			memset(buffer, 0, 64);
			encParam.param.encOSD.nChannel = i;
			encParam.param.encOSD.nShow = param.TimeOSD.bShow;
			encParam.param.encOSD.nIndex = 0;
			encParam.param.encOSD.data.nxPos = param.TimeOSD.x;
			encParam.param.encOSD.data.nyPos = param.TimeOSD.y;
			encParam.param.encOSD.data.color = param.TimeOSD.nColor;
			getTimeString(buffer, param.TimeOSD.nFormat);
			memcpy(encParam.param.encOSD.data.data, buffer, 32);
			videoEncSetup(i, 0, venc_setup_osd, &encParam);

			getVideoEncParam(i, 0, &vencparam);
			encParam.param.encOSD.nChannel = devCmdHeader.nChannel;
			encParam.param.encOSD.nIndex = 2+i;
			encParam.param.encOSD.nShow = param.TitleOSD[i].bShow;
			encParam.param.encOSD.data.nxPos = param.TitleOSD[i].x;
			encParam.param.encOSD.data.nyPos = param.TitleOSD[i].y;
			encParam.param.encOSD.data.color = param.TitleOSD[i].nColor;
			memcpy(encParam.param.encOSD.data.data, param.TitleOSD[i].sTitle, strlen(param.TitleOSD[i].sTitle));
			
			encParam.param.encOSD.data.data[strlen(param.TitleOSD[i].sTitle)] = '\0';
			ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_osd, &encParam);	

			// Add the code by lvjh, 2008-02-24	
			getVideoEncParam(i, 1, &vencparam);
			encParam.param.encOSD.nChannel = devCmdHeader.nChannel;
			encParam.param.encOSD.nIndex = 2+i;
			encParam.param.encOSD.nShow = param.TitleOSD[i].bShow;
			encParam.param.encOSD.data.nxPos = param.TitleOSD[i].x;
			encParam.param.encOSD.data.nyPos = param.TitleOSD[i].y;
			encParam.param.encOSD.data.color = param.TitleOSD[i].nColor;

			//set_title_osd_color(i, 0xFFFFFF);
			memcpy(encParam.param.encOSD.data.data, param.TitleOSD[i].sTitle, strlen(param.TitleOSD[i].sTitle));
			encParam.param.encOSD.data.data[strlen(param.TitleOSD[i].sTitle)] = '\0';
			
		//	memcpy(encParam.param.encOSD.data.data, "abcdefghigklmnopqrstuvwxyz12345", 31);
		//	encParam.param.encOSD.data.data[31] = '\0';
			
			ret = videoEncSetup(devCmdHeader.nChannel, 1, venc_setup_osd, &encParam);

			printf("setVideoOSD: %d %d %d %d %d %s\n", encParam.param.encOSD.nChannel, encParam.param.encOSD.nIndex, encParam.param.encOSD.nShow, encParam.param.encOSD.data.nxPos, encParam.param.encOSD.data.nyPos, encParam.param.encOSD.data.data);
		}

		//
		ret = setOsdParam(devCmdHeader.nChannel, &param);
		if (ret)
		{
			return -1;
		}
		
		returnMsg.nSock = pMsgHead->nSock;
		returnMsg.addr	=  pMsgHead->addr;
		returnMsg.nflag = pMsgHead->nflag;
		returnMsg.nCmd = NETCMD_USER_CMD;
		returnMsg.nRight = pMsgHead->nRight;
		returnMsg.nErrorCode = 0;
		returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
		
		devCmdReturn.nCmdID = devCmdHeader.nCmdID;
		devCmdReturn.nCmdLen = 0;
		devCmdReturn.nResult = 0;
		devCmdReturn.nReserve = 0;
		
		ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
		
		// Add the code by lvjh, 2009-09-17
		devCmdReturn.nCmdID = DEV_TITLE_CHANGE;
		devCmdReturn.nCmdLen = sizeof(TITLE_OSD);
		devCmdReturn.nResult = 0;
		devCmdReturn.nReserve = 0;
			
		memset(return_buffer, 0, 1024);
		memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
		memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param.TitleOSD[0], sizeof(TITLE_OSD));
				
		NETSDK_SendAllMsg(&return_buffer, sizeof(DEV_CMD_RETURN)+sizeof(TITLE_OSD));
		return ret;
	}

	//d

	
}

static int SESSION_getVideoLogo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	LOGO_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	ret = getLogoParam(devCmdHeader.nChannel, &param, NULL);
	if (ret < 0)
	{
		return -1;
	}
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(LOGO_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(LOGO_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(LOGO_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
		
	return ret;
}

static int SESSION_setVideoLogo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int flag = 0;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	LOGO_PARAM param;
	VIDEO_ENC_PARAM encParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(LOGO_PARAM));

	// LOGO
	encParam.param.encLogo.nChannel = devCmdHeader.nChannel;
	encParam.param.encLogo.nIndex = 0;
	encParam.param.encLogo.nShow = param.bShow;
	encParam.param.encLogo.data.nxPos = (param.x/16)*16;
	encParam.param.encLogo.data.nyPos = (param.y/16)*16;
	encParam.param.encLogo.data.nWidth = param.nWidth;
	encParam.param.encLogo.data.nHeight = param.nHeight;
	if (param.nDataLen < MAX_LOGO_DATA && param.nDataLen>0)
	{
		encParam.param.encLogo.data.data = pRecvBuf+sizeof(DEV_CMD_HEADER)+sizeof(LOGO_PARAM);
	}
	else
	{
		encParam.param.encLogo.data.data = NULL;
		//return -1;
	}

	printf("setLogo: %d  %d %d %d %d %d\n", param.bShow, param.x, param.y, param.nWidth, param.nHeight, param.nDataLen);
	//videoEncSetup(devCmdHeader.nChannel, 0, venc_delete_logo, &encParam);	// Add the code by lvjh, 2008-02-24
	
	if (encParam.param.encLogo.data.data != NULL)
	{
		ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_logo, &encParam);
	}
	else
	{
		//ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_logo_position, &encParam);
		ret = videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_logo, &encParam);
	}
	//
	if (encParam.param.encLogo.data.data != NULL)
	{
		ret = setLogoParam(devCmdHeader.nChannel, &param, pRecvBuf+sizeof(DEV_CMD_HEADER)+sizeof(LOGO_PARAM));
	}
	else
	{
		ret = setLogoParam(devCmdHeader.nChannel, &param, NULL);
	}
	
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getVideoMask(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	MASK_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getMaskParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(MASK_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(MASK_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(MASK_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

int videoSetMask(int nChannel, MASK_PARAM param)
{
	int ret = -1;
	int i = 0;
	VIDEO_ENC_PARAM vencParam;

	if (nChannel<0 || nChannel>=MAX_CHANNEL)
	{
		return -1;
	}	

	for (i=0; i<MAX_VIDEO_MASK_NUM; i++)
	{
		// add by zhb, 2007-11-07
		param.VideoMask[i].x = (param.VideoMask[i].x/16)*16;
		param.VideoMask[i].y = (param.VideoMask[i].y/16)*16;
		param.VideoMask[i].nWidth = (param.VideoMask[i].nWidth/16)*16;
		param.VideoMask[i].nHeight = (param.VideoMask[i].nHeight/16)*16;

		vencParam.param.encMask.nChannel = nChannel;
		vencParam.param.encMask.nIndex = i;
		vencParam.param.encMask.nShow = param.VideoMask[i].bMask;
		
		// 鍙傛暟鏍￠獙
		if (param.VideoMask[i].x>720 ||
			param.VideoMask[i].y>576 ||
			param.VideoMask[i].nWidth>720-param.VideoMask[i].x ||
			param.VideoMask[i].nHeight>576-param.VideoMask[i].y)
		{
			continue;
		}
		
		//printf("MASK: %d %d %d %d\n", param.VideoMask[i].x, param.VideoMask[i].y, param.VideoMask[i].nWidth, param.VideoMask[i].nHeight);

		if (!(param.VideoMask[i].x % 16))
		{
			vencParam.param.encMask.data.nxPos = param.VideoMask[i].x;
		}
		else
		{
			vencParam.param.encMask.data.nxPos = (param.VideoMask[i].x/16+1)*16;
		}
		if (!(param.VideoMask[i].y % 16))
		{
			vencParam.param.encMask.data.nyPos = param.VideoMask[i].y;
		}
		else
		{
			vencParam.param.encMask.data.nyPos = (param.VideoMask[i].y/16+1)*16;
		}
		if (!(param.VideoMask[i].nWidth % 16))
		{
			vencParam.param.encMask.data.nWidth = param.VideoMask[i].nWidth;
		}
		else
		{
			vencParam.param.encMask.data.nWidth = (param.VideoMask[i].nWidth/16+1)*16;
		}
		if (!(param.VideoMask[i].nHeight % 16))
		{
			vencParam.param.encMask.data.nHeight = param.VideoMask[i].nHeight;
		}
		else
		{
			vencParam.param.encMask.data.nHeight = (param.VideoMask[i].nHeight/16+1)*16;
		}

		ret = videoEncSetup(nChannel, 0, venc_setup_mask, &vencParam);
		if (ret < 0)
		{
			return -1;
		}
	}

	return 0;
}

static int SESSION_setVideoMask(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	MASK_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(MASK_PARAM));

	// MASK
	ret = videoInSetMask(devCmdHeader.nChannel, param.VideoMask[0]);
	//
	ret = setMaskParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getVideoMosaic(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MASK param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getMosaicParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_MASK);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_MASK);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_MASK));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

int videoSetMosaic(int nChannel, VIDEO_MASK param)
{
	int ret = -1;
	int i = 0;
	VIDEO_ENC_PARAM vencParam;

	if (nChannel<0 || nChannel>=MAX_CHANNEL)
	{
		return -1;
	}	

	// add by zhb, 2007-11-07
	param.x = (param.x/16)*16;
	param.y = (param.y/16)*16;
	param.nWidth = (param.nWidth/16)*16;
	param.nHeight = (param.nHeight/16)*16;

	vencParam.param.encMosaic.nChannel = nChannel;
	vencParam.param.encMosaic.nIndex = 0;
	vencParam.param.encMosaic.nShow = param.bMask;
		
	// 鍙傛暟鏍￠獙
	if (param.x>720 ||
		param.y>576 ||
		param.nWidth>720-param.x ||
		param.nHeight>576-param.y)
	{
		return -1;
	}
		
	//printf("MASK: %d %d %d %d\n", param.x, param.y, param.nWidth, param.nHeight);

	if (!(param.x % 16))
	{
		vencParam.param.encMosaic.data.nxPos = param.x;
	}
	else
	{
		vencParam.param.encMosaic.data.nxPos = (param.x/16+1)*16;
	}
	if (!(param.y % 16))
	{
		vencParam.param.encMosaic.data.nyPos = param.y;
	}
	else
	{
		vencParam.param.encMosaic.data.nyPos = (param.y/16+1)*16;
	}
	if (!(param.nWidth % 16))
	{
		vencParam.param.encMosaic.data.nWidth = param.nWidth;
	}
	else
	{
		vencParam.param.encMosaic.data.nWidth = (param.nWidth/16+1)*16;
	}
	if (!(param.nHeight % 16))
	{
		vencParam.param.encMosaic.data.nHeight = param.nHeight;
	}
	else
	{
		vencParam.param.encMosaic.data.nHeight = (param.nHeight/16+1)*16;
	}
	
	ret = videoEncSetup(nChannel, 0, venc_setup_mosaic, &vencParam);
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

static int SESSION_setVideoMosaic(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int i = 0;
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MASK param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_MASK));

	//
	ret = setMosaicParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_getVideoChnName(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_CHANNEL_NAME param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	//printf("getVideoChnName: %d\n", devCmdHeader.nChannel);

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoChnNameParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		//printf("skjlfsa\n");
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_CHANNEL_NAME);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_CHANNEL_NAME);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_CHANNEL_NAME));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setVideoChnName(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_CHANNEL_NAME param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_CHANNEL_NAME));

	//
	ret = setVideoChnNameParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = -1;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	//
	devCmdReturn.nCmdID = DEV_CHN_NAME_CHANGE;
	devCmdReturn.nCmdLen = sizeof(VIDEO_CHANNEL_NAME);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = devCmdHeader.nChannel;
		
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_CHANNEL_NAME));

	ret = NETSDK_SendAllMsg(return_buffer, sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_CHANNEL_NAME));
	
	return ret;
}

static int SESSION_getVideoMotion(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MOTION_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoMotionParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_MOTION_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_MOTION_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_MOTION_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setVideoMotion(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int i = 0;
	int j = 0;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MOTION_PARAM param;
	VIDEO_ENC_PARAM encParam;
	VENC_PARAM venParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_MOTION_PARAM));

	// MOTION
	getVideoEncParam(devCmdHeader.nChannel, 0, &venParam);

	//memset(encParam.param.chMotionArea, param.nSensibility, 1620);

	printf("SESSION_setVideoMotion: %d %d\n", param.nSensibility, param.mask[0]);

	// Change the code by lvjh, 2009-01-31
	//mdMacroConvertExt(param.mask, encParam.param.chMotionArea, venParam.nEncodeWidth, venParam.nEncodeHeight, param.nSensibility);
	memcpy(&encParam.param.mdParam, &param, sizeof(VIDEO_MOTION_PARAM));
	
	videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_md_area, &encParam);
	encParam.param.nMD = 1;
	videoEncSetup(devCmdHeader.nChannel, 0, venc_setup_md, &encParam);

	//
	ret = setVideoMotionParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getAudioInAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getAudioInAttrParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(AUDIO_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(AUDIO_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(AUDIO_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

static int SESSION_setAudioInAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(AUDIO_PARAM));
	/*
	//
	ret = setAudioInAttrParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}
	*/
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = -1;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getAudioOutAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getAudioOutAttrParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(AUDIO_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(AUDIO_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(AUDIO_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setAudioOutAttr(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(AUDIO_PARAM));
	/*
	//
	ret = setAudioOutAttrParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}
	*/
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = -1;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}


static int SESSION_getMultiDev(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024*5];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_MULTIDEVALL_INFO param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	printf("Entry SESSION_getMultiDev .....\n");
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
#if 1	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
#endif

	ret = getMultiDevParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	printf("getMultiDevParam = %d \n", pMsgHead->nSock);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd =  NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DVSNET_MULTIDEVALL_INFO);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DVSNET_MULTIDEVALL_INFO);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), & param, sizeof(DVSNET_MULTIDEVALL_INFO));
	printf("getMultiDevParam \n");
	printf("param =  %d\n", sizeof(param));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setMultiDev(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DVSNET_MULTIDEVALL_INFO param;

#if  1
if (pRecvBuf == NULL)
	{
		return -1;
	}
#endif

	printf("Entry SESSION_setMultiDev .....\n");
#if 1
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
#endif

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DVSNET_MULTIDEVALL_INFO));

	ret = setMultiDevParam(&param);
	if (ret)
	{
		return -1;
	}
	printf("setMultiDevParam \n");

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}



static int SESSION_getAudioEnc(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AENC_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getAudioEncParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(AENC_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(AENC_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(AENC_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setAudioEnc(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AENC_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(AENC_PARAM));

	// AENC
	if (param.nOnFlag == 1)
	{
		audioEncModuleResume(devCmdHeader.nChannel);
	}
	else
	{
		audioEncModulePause(devCmdHeader.nChannel);
	}

	//
	ret = setAudioEncParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

// Add the code by lvjh, 2008-05-12
static int SESSION_ResumeDefaultColor(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_IN_ATTR videoInAttr;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	
#ifdef CCD
	vadcDrv_SetBrightness(devCmdHeader.nChannel, DEFAULT_BRIGHTNESS);
	vadcDrv_SetContrast(devCmdHeader.nChannel, DEFAULT_CONTRAST);
	vadcDrv_SetSaturation(devCmdHeader.nChannel, DEFAULT_SATURATION);
	vadcDrv_SetHue(devCmdHeader.nChannel, DEFAULT_HUE);
#endif

#ifdef HD_CMOS
	vadcDrv_SetBrightness(devCmdHeader.nChannel, DEFAULT_BRIGHTNESS);
	vadcDrv_SetContrast(devCmdHeader.nChannel, DEFAULT_CONTRAST);
	vadcDrv_SetHue(devCmdHeader.nChannel, DEFAULT_HUE);
	vadcDrv_SetSaturation(devCmdHeader.nChannel, DEFAULT_SATURATION);
#endif

	videoInAttr.nBrightness = DEFAULT_BRIGHTNESS;
	videoInAttr.nHue = DEFAULT_HUE;
	videoInAttr.nContrast = DEFAULT_CONTRAST;
	videoInAttr.nSaturation = DEFAULT_SATURATION;
	videoInAttr.reserve = 0;
	setVideoInAttrParam(devCmdHeader.nChannel, &videoInAttr);
	
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getImageFlip(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_FLIP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoFlipParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_FLIP_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_FLIP_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_FLIP_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setImageFlip(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_FLIP_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_FLIP_PARAM));

	// 鍥惧儚鍊掔珛
	vadcDrv_SetImageFlip(devCmdHeader.nChannel, param.nFlip);
	
	ret = setVideoFlipParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getImageMirror(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MIRROR_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoMirrorParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_MIRROR_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_MIRROR_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_MIRROR_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setImageMirror(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_MIRROR_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_MIRROR_PARAM));

	// 鍥惧儚澧冨儚
	vadcDrv_SetImageMirror(devCmdHeader.nChannel, param.nMirror);
	
	ret = setVideoMirrorParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getImageHz(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_HZ_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getVideoHzParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(VIDEO_HZ_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(VIDEO_HZ_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(VIDEO_HZ_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setImageHz(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	VIDEO_HZ_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(VIDEO_HZ_PARAM));

	// 鍥惧儚澧冨儚
	vadcDrv_SetImageHz(devCmdHeader.nChannel, param.nHz);
	
	ret = setVideoHzParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getAudioInPath(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PATH_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getAudioInPathParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(AUDIO_PATH_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(AUDIO_PATH_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(AUDIO_PATH_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setAudioInPath(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	AUDIO_PATH_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(AUDIO_PATH_PARAM));

	acodecDrv_SetAnalogInputPath(devCmdHeader.nChannel, param.nPath);
	
	ret = setAudioInPathParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_get3G(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	G3_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	ret = get3gParam(&param);
	if (ret < 0)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-08-20
	ret = get_3g_ip(param.pWanIP);
	if (ret < 0)
	{
		memset(param.pWanIP, 0, 16);
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(G3_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(G3_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(G3_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_set3G(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	G3_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(G3_PARAM));

	G3_Setup(param);
	
	ret = set3gParam(&param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getScheduleSnapshot(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SCHEDULE_SNAPSHOT_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	ret = getScheduleSnapshotParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(SCHEDULE_SNAPSHOT_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(SCHEDULE_SNAPSHOT_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(SCHEDULE_SNAPSHOT_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setScheduleSnapshot(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SCHEDULE_SNAPSHOT_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(SCHEDULE_SNAPSHOT_PARAM));

	ret = scheduleSnapshotSetup(devCmdHeader.nChannel, param);
	if (ret)
	{
		return -1;
	}
	
	ret = setScheduleSnapshotParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

// Add the code by lvjh, 2009-09-16
static int SESSION_getPtzAutoCtrl(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_AUTO_CTRL param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	ret = getPtzAutoCtrlParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(PTZ_AUTO_CTRL);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(PTZ_AUTO_CTRL);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(PTZ_AUTO_CTRL));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setPtzAutoCtrl(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	PTZ_AUTO_CTRL param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(PTZ_AUTO_CTRL));

	ret = setAutoPtz(devCmdHeader.nChannel, param);
	if (ret)
	{
		return -1;
	}
	
	ret = setPtzAutoCtrlParam(devCmdHeader.nChannel, &param);
	if (ret)
	{
		return -1;
	}

	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

static int SESSION_getDeviceConfigure(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DEVICE_CONFIGURE param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	
	memset(&param, 0, sizeof(DEVICE_CONFIGURE));
	getMACConfigure(param.MAC);
	getIDConfigure(param.ID);
	//add by lvjh
	setP2PConfigure(&param.P2P);
	setP2PParam(&param.P2P);
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(DEVICE_CONFIGURE);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(DEVICE_CONFIGURE);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(DEVICE_CONFIGURE));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setDeviceConfigure(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	DEVICE_CONFIGURE param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(DEVICE_CONFIGURE));

	setMACConfigure(param.MAC);
	setIDConfigure(param.ID);
	//add by lvjh
	getP2PConfigure(&param.P2P);
	getP2PParam(&param.P2P);
		//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}

int SESSION_saveDevParamFile(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SYS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
		
	memset(&param, 0, sizeof(SYS_PARAM));
	getDevParamFile(&param);
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(SYS_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(SYS_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(SYS_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_ResumeDevParamFile(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	SYS_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(SYS_PARAM));

	saveDevParamFile(&param);
	
	//
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;	
}


#ifdef RECORD
int setRecord(int nChannel, RECORD_CHANNEL_PARAM recordParam)
{
	int ret = -1;

	RECORDSDK_CMD_PARAM cmdParam;
	
	// record SDK
	cmdParam.nChannel = nChannel;
	cmdParam.nOpt = RSDKCMD_SET_RECORD_PARAM;
	memcpy(&cmdParam.param.recordParam, &recordParam, sizeof(RECORD_CHANNEL_PARAM));

	ret = RECORDSDK_Operate(&cmdParam, NULL, NULL);
	
	return ret;
}

static int SESSION_getRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	RECORD_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	/*
	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}
	*/
	ret = getRecordParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(RECORD_CHANNEL_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(RECORD_CHANNEL_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(RECORD_CHANNEL_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	RECORD_PARAM param;
	RECORD_CHANNEL_PARAM recordParam;
	RECORDSDK_CMD_PARAM cmdParam;
	VENC_PARAM vencParam;
	AENC_PARAM aencParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(RECORD_PARAM));

	ret = setRecordParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_RECORD_PARAM;
	recordParam.nCoverMode = param.nCoverMode;
	recordParam.nAudioFlag = param.nAudioFlag;
	recordParam.nReserve = param.nReserve;

	getVideoEncParam(devCmdHeader.nChannel, 0, &vencParam);
	getAudioEncParam(devCmdHeader.nChannel, &aencParam);

	// Add the code by lvjh, 2009-01-03
	recordParam.avFormat.nVideoEncType = ENCODE_VIDEO_HISI;

	recordParam.avFormat.nImageHeight = vencParam.nEncodeHeight;
	recordParam.avFormat.nImageWidth= vencParam.nEncodeWidth;
#ifdef G726
	recordParam.avFormat.nAudioEncType = ENCODE_AUDIO_G726;
#else 
	recordParam.avFormat.nAudioEncType = ENCODE_AUDIO_G711;
#endif

	recordParam.avFormat.nAudioChannels= aencParam.nChannels;
	recordParam.avFormat.nAudioSamples= aencParam.nSampleRate;
	recordParam.avFormat.nAudioBitRate= aencParam.nBitRate;
	
	memcpy(&cmdParam.param.recordParam, &recordParam, sizeof(RECORD_CHANNEL_PARAM));
	RECORDSDK_Operate(&cmdParam, NULL, NULL);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getTimerRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TIMER_RECORD_CHANNEL_PARAM param;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	ret = getTimerRecordParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(TIMER_RECORD_CHANNEL_PARAM);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(TIMER_RECORD_CHANNEL_PARAM);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(TIMER_RECORD_CHANNEL_PARAM));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;
}

static int SESSION_setTimerRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	TIMER_RECORD_CHANNEL_PARAM param;

	RECORDSDK_CMD_PARAM cmdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(TIMER_RECORD_CHANNEL_PARAM));

	ret = setTimerRecordParam(devCmdHeader.nChannel, &param);
	if (ret < 0)
	{
		return -1;
	}
	
	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_TIMER_RECORD_PARAM;
	memcpy(&cmdParam.param.recordParam, &param, sizeof(TIMER_RECORD_CHANNEL_PARAM));
	RECORDSDK_Operate(&cmdParam, NULL, NULL);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;
}

static int SESSION_getManualRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;

	return ret;
}

static int SESSION_setManualRecord(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	MANUAL_RECORD_PARAM param;

	RECORDSDK_CMD_PARAM cmdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(MANUAL_RECORD_PARAM));

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_MANUAL_RECORD_PARAM;
	memcpy(&cmdParam.param.manualRecord, &param, sizeof(MANUAL_RECORD_PARAM));
	RECORDSDK_Operate(&cmdParam, NULL, NULL);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	return ret;
}

static int SESSION_queryRecordDate(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[2048];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	QUERY_RECDATE param;

	RECORDSDK_CMD_PARAM cmdParam;
	ACK_RECDATE ackRecDate;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	printf("Entry SESSION_queryRecordDate \n");

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(QUERY_RECDATE));

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_QUERY_RECDATE;
	printf("%d%d%d%d\n", param.nMonth, param.nYear, param.nType, param.nReserve);
	memcpy(&cmdParam.param.recDate, &param, sizeof(QUERY_RECDATE));
	RECORDSDK_Operate(&cmdParam, &ackRecDate, &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.addr = pMsgHead->addr;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(ACK_RECDATE);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(ACK_RECDATE);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &ackRecDate, sizeof(ACK_RECDATE));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;

}

static int SESSION_queryRecordFile(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char *return_buffer = NULL;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	QUERY_RECFILE param;

	RECORDSDK_CMD_PARAM cmdParam;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(QUERY_RECFILE));

	return_buffer = (char *)malloc(128*1024);
	if (return_buffer == NULL)
	{
		printf("malloc return_buffer: Error!\n");
		return -1;
	}
	memset(return_buffer, 0, 128*1024);

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_QUERY_RECFILE;
	memcpy(&cmdParam.param.recFile, &param, sizeof(QUERY_RECFILE));
	RECORDSDK_Operate(&cmdParam, return_buffer+sizeof(DEV_CMD_RETURN), &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+size;

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = size;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	free(return_buffer);

	//printf("SESSION_queryRecordFile: %d %d\n", ret, size);

	return ret;

}

static int SESSION_queryJpegDate(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[2048];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	QUERY_RECDATE param;

	RECORDSDK_CMD_PARAM cmdParam;
	ACK_RECDATE ackRecDate;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(QUERY_RECDATE));

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_QUERY_JPEGDATE;
	memcpy(&cmdParam.param.recDate, &param, sizeof(QUERY_RECDATE));
	RECORDSDK_Operate(&cmdParam, &ackRecDate, &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(ACK_RECDATE);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(ACK_RECDATE);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &ackRecDate, sizeof(ACK_RECDATE));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	return ret;

}

static int SESSION_queryJpegFile(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char *return_buffer = NULL;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	QUERY_RECFILE param;

	RECORDSDK_CMD_PARAM cmdParam;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(QUERY_RECFILE));

	return_buffer = (char *)malloc(256*1024);
	if (return_buffer == NULL)
	{
		printf("malloc return_buffer: Error!\n");
		return -1;
	}
	memset(return_buffer, 0, 256*1024);

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_QUERY_JPEGFILE;
	memcpy(&cmdParam.param.recFile, &param, sizeof(QUERY_RECFILE));
	RECORDSDK_Operate(&cmdParam, return_buffer+sizeof(DEV_CMD_RETURN), &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+size;

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = size;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));

	ret = NETSDK_SendMsg(&returnMsg, return_buffer);
	
	free(return_buffer);

	printf("SESSION_queryJpegFile: %d %d\n", ret, size);

	return ret;

}

static int SESSION_getHardDiskInfo(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	HARD_DISK_INFO param;

	RECORDSDK_CMD_PARAM cmdParam;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_GET_HARDDISK_INFO;
	RECORDSDK_Operate(&cmdParam, &param, &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+size;

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = size;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(HARD_DISK_INFO));
	
	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;

}

static int SESSION_setHardDiskFdisk(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	HD_FDISK_PARAM param;

	RECORDSDK_CMD_PARAM cmdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(HD_FDISK_PARAM));

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_HARDDISK_FDISK;
	memcpy(&cmdParam.param.fdisk, &param, sizeof(HD_FDISK_PARAM));
	RECORDSDK_Operate(&cmdParam, NULL, NULL);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
	
	return ret;

}

static int SESSION_setHardDiskFormat(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	HD_FORMAT_PARAM param;

	RECORDSDK_CMD_PARAM cmdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(HD_FORMAT_PARAM));

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_HARDDISK_FORMAT;
	memcpy(&cmdParam.param.fdisk, &param, sizeof(HD_FORMAT_PARAM));
	ret = RECORDSDK_Operate(&cmdParam, NULL, NULL);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = ret;
	devCmdReturn.nReserve = 0;

	NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	printf("Format(%d %d %d): %d\n", param.nDiskNo, param.nPartionNo, param.nReserve, ret);

	return ret;
}

// Add the code by lvjh, 2011-04-13

static int g_storage_format_flag = 0;

int format_storage_fun()
{
	int nRet = -1;
	
	RECORDSDK_CMD_PARAM cmdParam;
	
	cmdParam.nChannel = 0;
	cmdParam.nOpt = RSDKCMD_SET_HARDDISK_FORMAT;

	g_storage_format_flag = 1;
	
	nRet = RECORDSDK_Operate(&cmdParam, NULL, NULL);
	
	g_storage_format_flag = 0;
	
	return nRet;
}

static int format_storage_device()
{
	int nRet = -1;
	pthread_t threadID;
	
	nRet = pthread_create(&threadID, NULL, (void *)format_storage_fun, NULL);
	if (nRet)
	{		
		return -1;
	}

	return 0;
}

static int SESSION_setHardDiskFormat1(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	HD_FORMAT_PARAM param;

	RECORDSDK_CMD_PARAM cmdParam;

	if (pRecvBuf == NULL)
	{
		return -1;
	}
	
	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));
	memcpy(&param, pRecvBuf+sizeof(DEV_CMD_HEADER), sizeof(HD_FORMAT_PARAM));
	
	// Add the code by lvjh, 2011-04-25
	if (g_storage_format_flag == 1)
	{
		returnMsg.nSock = pMsgHead->nSock;
		returnMsg.addr  =  pMsgHead->addr;
		returnMsg.nflag = pMsgHead->nflag;
		returnMsg.nCmd = NETCMD_USER_CMD;
		returnMsg.nRight = pMsgHead->nRight;
		returnMsg.nErrorCode = 0;
		returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
	
		devCmdReturn.nCmdID = devCmdHeader.nCmdID;
		devCmdReturn.nCmdLen = 0;
		devCmdReturn.nResult = -1;
		devCmdReturn.nReserve = 0;
	
		NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
		
		printf("Format storage error: formating ...\n");
		
		return 0;
	}

	/*
	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_SET_HARDDISK_FORMAT;
	memcpy(&cmdParam.param.fdisk, &param, sizeof(HD_FORMAT_PARAM));
	ret = RECORDSDK_Operate(&cmdParam, NULL, NULL);
	*/
	
	// Add the code by lvjh, 2011-04-25
	system("busybox rm -rf /format.hex");
	
	format_storage_device();	
	
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = 0;
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;

	NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

	printf("Format(%d %d %d): %d\n", param.nDiskNo, param.nPartionNo, param.nReserve, 0);

	return 0;
}

static int SESSION_getFdiskProgress(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	char return_buffer[1024];
	MSG_HEAD returnMsg;
	DEV_CMD_HEADER devCmdHeader;
	DEV_CMD_RETURN devCmdReturn;
	HD_FORMAT_PROGRESS param;

	RECORDSDK_CMD_PARAM cmdParam;
	int size = 0;

	if (pRecvBuf == NULL)
	{
		return -1;
	}

	memcpy(&devCmdHeader, pRecvBuf, sizeof(DEV_CMD_HEADER));

	if (devCmdHeader.nChannel<0 || devCmdHeader.nChannel>=MAX_CHANNEL)
	{
		return -1;
	}

	// record SDK
	cmdParam.nChannel = devCmdHeader.nChannel;
	cmdParam.nOpt = RSDKCMD_GET_FDISK_PROCESS;
	RECORDSDK_Operate(&cmdParam, &param, &size);

	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr  =  pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	returnMsg.nCmd = NETCMD_USER_CMD;
	returnMsg.nRight = pMsgHead->nRight;
	returnMsg.nErrorCode = 0;
	returnMsg.nBufSize = sizeof(DEV_CMD_RETURN)+sizeof(HD_FORMAT_PROGRESS);

	devCmdReturn.nCmdID = devCmdHeader.nCmdID;
	devCmdReturn.nCmdLen = sizeof(HD_FORMAT_PROGRESS);
	devCmdReturn.nResult = 0;
	devCmdReturn.nReserve = 0;
	
	memcpy(return_buffer, &devCmdReturn, sizeof(DEV_CMD_RETURN));
	memcpy(return_buffer+sizeof(DEV_CMD_RETURN), &param, sizeof(HD_FORMAT_PROGRESS));
	
	ret = NETSDK_SendMsg(&returnMsg, return_buffer);

	return ret;
}

#endif


int recvCmdProcFun(MSG_HEAD *pMsgHead, char *pRecvBuf)
{
	int ret = -1;
	int nRight = 0;
	int nCmdSize = 0;

	MSG_HEAD returnMsg;

	if (pMsgHead == NULL)
	{
		return -1;
	}

	nRight = pMsgHead->nRight;
	nCmdSize = pMsgHead->nBufSize;

	memset(&returnMsg, 0, sizeof(MSG_HEAD));
	returnMsg.nSock = pMsgHead->nSock;
	returnMsg.addr = pMsgHead->addr;
	returnMsg.nflag = pMsgHead->nflag;
	//printf("pMsgHead->nflag = %d\n", pMsgHead->nflag);
	
	//printf("recvCmdProcFun ...\n");

	switch (pMsgHead->nCmd)
	{
	case NETCMD_UPDATE:
		{
			if (nRight != RIGHT_ADMIN)
			{
				returnMsg.addr  =  pMsgHead->addr;
				returnMsg.nflag = pMsgHead->nflag;
				returnMsg.nCmd = NETCMD_UPDATE;
				returnMsg.nRight = pMsgHead->nRight;
				returnMsg.nErrorCode = NETERR_NOT_RIGHT;

				ret = NETSDK_SendMsg(&returnMsg, NULL);
			}
			else
			{
				ret = updateImageFile(pMsgHead->nSock, pMsgHead->nBufSize, pRecvBuf, pMsgHead->addr);
			}
			
		}
		break;

	case NETCMD_USER_CMD:
		{
			printf("Entry NETCMD_USER_CMD:\n");
			DEV_CMD_HEADER devCmdHeader;
			DEV_CMD_RETURN devCmdReturn;
					
			if (pRecvBuf == NULL)
			{
				return -1;
			}
			if (pMsgHead->nBufSize < sizeof(DEV_CMD_HEADER))
			{
				return -1;
			}
			
			memcpy(&devCmdHeader, pRecvBuf,  sizeof(DEV_CMD_HEADER));

			if (devCmdHeader.nCmdID != 0x10029)
			{
				//printf("Receive CMD: %x, %d\n", devCmdHeader.nCmdID, pMsgHead->nBufSize);
			}

			// Add the code by lvjh, 2006-02-26
			switch (nRight)
			{
			case RIGHT_ADMIN:
#if 1
			case RIGHT_USER:
			case RIGHT_GUEST:
#endif
				break;
				
#if 0
			case RIGHT_USER:
				{
					if (devCmdHeader.nCmdID == MANUAL_SNAPSHOT||devCmdHeader.nCmdID == SET_IRPROBE_ALARM_STOP||devCmdHeader.nCmdID ==GET_VIDEO_OSD||devCmdHeader.nCmdID == GET_VIDEO_ENC_PARAM||devCmdHeader.nCmdID == GET_PROBE_STATUS
						|| devCmdHeader.nCmdID == CTRL_PTZ || devCmdHeader.nCmdID == GET_VIDEO_ENC_PARAM ||devCmdHeader.nCmdID ==  SET_VIDEO_ATTR||devCmdHeader.nCmdID == SET_IRPROBE_ALARM_START)
					{
						break;
					}
					else
					{
						returnMsg.addr  =  pMsgHead->addr;
						returnMsg.nflag = pMsgHead->nflag;
						returnMsg.nCmd = NETCMD_USER_CMD;
						returnMsg.nRight = pMsgHead->nRight;
						returnMsg.nErrorCode = NETERR_NOT_RIGHT;
						returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
						
						devCmdReturn.nCmdID = devCmdHeader.nCmdID;
						devCmdReturn.nCmdLen = 0;
						devCmdReturn.nResult = NETERR_NOT_RIGHT;
						devCmdReturn.nReserve = 0;
	
						ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
						return -1;
					}
				}
				break;
			
			case RIGHT_GUEST:
				{
					returnMsg.addr  =  pMsgHead->addr;
					returnMsg.nflag = pMsgHead->nflag;
					returnMsg.nCmd = NETCMD_USER_CMD;
					returnMsg.nRight = pMsgHead->nRight;
					returnMsg.nErrorCode = NETERR_NOT_RIGHT;
					returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
						
					devCmdReturn.nCmdID = devCmdHeader.nCmdID;
					devCmdReturn.nCmdLen = 0;
					devCmdReturn.nResult = NETERR_NOT_RIGHT;
					devCmdReturn.nReserve = 0;
	
					ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
				}
				return -1;
				
#endif
			default:
				{
					
					returnMsg.addr  =  pMsgHead->addr;
					returnMsg.nflag = pMsgHead->nflag;
					returnMsg.nCmd = NETCMD_USER_CMD;
					returnMsg.nRight = pMsgHead->nRight;
					returnMsg.nErrorCode = NETERR_NOT_RIGHT;
					returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);
						
					devCmdReturn.nCmdID = devCmdHeader.nCmdID;
					devCmdReturn.nCmdLen = 0;
					devCmdReturn.nResult = NETERR_NOT_RIGHT;
					devCmdReturn.nReserve = 0;
	
					ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);
				}
				return -1;			
			}

			printf("devCmdHeader.nCmdID = %p\n", devCmdHeader.nCmdID);
			switch (devCmdHeader.nCmdID)
			{
			case NETCMD_KEEP_ALIVE:
				printf("CMD: recvCmdProcFun ---->>>>NETCMD_KEEP_ALIVE\n");
				NET_HEAD netHead;
				netHead.nFlag = HDVS_FLAG;
				netHead.nCommand = NETCMD_KEEP_ALIVE;
				netHead.nBufSize = 0;
				
				ret = udpSendMsg(pMsgHead->nSock, &netHead, sizeof(NET_HEAD), pMsgHead->addr);
				if (ret <= 0 && (errno == EPIPE || errno == ENOTSOCK || errno == EBADF))
				{
					printf("NAT(%d): Keepalive Error!\n", pMsgHead->nSock);
					break;
				}
				//sleep(1);
				break;
			case GET_SYS_INFO:
				printf("CMD: GET_SYS_INFO\n");
				ret = SESSION_getSysInfo(pMsgHead, pRecvBuf);
				break;

			case SET_SYS_INFO:
				printf("CMD: SET_SYS_INFO\n");
				ret = SESSION_setSysInfo(pMsgHead, pRecvBuf);
				break;
				
			case GET_USER_INFO:
				printf("CMD: GET_USER_INFO\n");
				ret = SESSION_getUserInfo(pMsgHead, pRecvBuf);
				break;

			case SET_USER_INFO:
				printf("CMD: SET_USER_INFO\n");
				ret = SESSION_setUserInfo(pMsgHead, pRecvBuf);
				break;

#ifdef CCD
			case GET_VIDEO_STANDARD:
				printf("CMD: GET_VIDEO_STANDARD\n");
				ret = SESSION_getVideoInStandard(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_STANDARD:
				printf("CMD: SET_VIDEO_STANDARD\n");
				ret = SESSION_setVideoInStandard(pMsgHead, pRecvBuf);
				break;
#endif

			case GET_VIDEO_ATTR:
				printf("CMD: GET_VIDEO_ATTR\n");
				ret = SESSION_getVideoInAttr(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_ATTR:
				printf("CMD: SET_VIDEO_ATTR\n");
				ret = SESSION_setVideoInAttr(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_ENC_PARAM:
				printf("CMD: GET_VIDEO_ENC_PARAM\n");
				ret = SESSION_getVideoEnc(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_ENC_PARAM:
				printf("CMD: SET_VIDEO_ENC_PARAM\n");
				ret = SESSION_setVideoEnc(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_WM_PARAM:
				printf("CMD: GET_VIDEO_WM_PARAM\n");
				ret = SESSION_getVideoWM(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_WM_PARAM:
				printf("CMD: SET_VIDEO_WM_PARAM\n");
				ret = SESSION_setVideoWM(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_OSD:
				printf("CMD: GET_VIDEO_OSD\n");
				ret = SESSION_getVideoOSD(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_OSD:
				printf("CMD: SET_VIDEO_OSD\n");
				ret = SESSION_setVideoOSD(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_LOGO:
				printf("CMD: GET_VIDEO_LOGO\n");
				ret = SESSION_getVideoLogo(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_LOGO:
				printf("CMD: SET_VIDEO_LOGO\n");
				ret = SESSION_setVideoLogo(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_MASK:
				printf("CMD: GET_VIDEO_MASK\n");
				ret = SESSION_getVideoMask(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_MASK:
				printf("CMD: SET_VIDEO_MASK\n");
				ret = SESSION_setVideoMask(pMsgHead, pRecvBuf);
				break;

			case GET_CHANNEL_NAME:
				printf("CMD: GET_CHANNEL_NAME\n");
				ret = SESSION_getVideoChnName(pMsgHead, pRecvBuf);
				break;

			case SET_CHANNEL_NAME:
				printf("CMD: SET_CHANNEL_NAME\n");
				ret = SESSION_setVideoChnName(pMsgHead, pRecvBuf);
				break;

			case GET_VIDEO_MOTION_PARAM:
				printf("CMD: GET_VIDEO_MOTION_PARAM\n");
				ret = SESSION_getVideoMotion(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_MOTION_PARAM:
				printf("CMD: SET_VIDEO_MOTION_PARAM\n");
				ret = SESSION_setVideoMotion(pMsgHead, pRecvBuf);
				break;

			case GET_AUDIO_IN_ATTR:
				printf("CMD: GET_AUDIO_IN_ATTR\n");
				ret = SESSION_getAudioInAttr(pMsgHead, pRecvBuf);
				break;

			case SET_AUDIO_IN_ATTR:
				printf("CMD: SET_AUDIO_IN_ATTR\n");
				ret = SESSION_setAudioInAttr(pMsgHead, pRecvBuf);
				break;

			case GET_AUDIO_OUT_ATTR:
				printf("CMD: GET_AUDIO_OUT_ATTR\n");
				ret = SESSION_getAudioOutAttr(pMsgHead, pRecvBuf);
				break;

			case SET_AUDIO_OUT_ATTR:
				printf("CMD: SET_AUDIO_OUT_ATTR\n");
				ret = SESSION_setAudioOutAttr(pMsgHead, pRecvBuf);
				break;

			case GET_AUDIO_IN_ENC_PARAM:
				printf("CMD: GET_AUDIO_IN_ENC_PARAM\n");
				ret = SESSION_getAudioEnc(pMsgHead, pRecvBuf);
				break;

			case SET_AUDIO_IN_ENC_PARAM:
				printf("CMD: SET_AUDIO_IN_ENC_PARAM\n");
				ret = SESSION_setAudioEnc(pMsgHead, pRecvBuf);
				break;
				
			//设置多画面设备列表
			case SET_MULTIDEV_INFO:
				printf("CMD: SET_MULTIDEV_INFO\n");
				ret = SESSION_setMultiDev(pMsgHead, pRecvBuf);
				break;

			//获取多画面设备列表
			case GET_MULTIDEV_INFO:
				printf("CMD: GET_MULTIDEV_INFO\n");
				ret = SESSION_getMultiDev(pMsgHead, pRecvBuf);
				break;
	
#ifdef CCD
			case GET_VIDEO_LOST_ALARM_PARAM:
				printf("CMD: GET_VIDEO_LOST_ALARM_PARAM\n");
				ret = SESSION_getVideoLostAlarm(pMsgHead, pRecvBuf);
				break;
				

			case SET_VIDEO_LOST_ALARM_PARAM:
				printf("CMD: SET_VIDEO_LOST_ALARM_PARAM\n");
				ret = SESSION_setVideoLostAlarm(pMsgHead, pRecvBuf);
				break;
#endif
			case GET_VIDEO_MOTION_ALARM_PARAM:
				printf("CMD: GET_VIDEO_MOTION_ALARM_PARAM\n");
				ret = SESSION_getVideoMotionAlarm(pMsgHead, pRecvBuf);
				break;

			case SET_VIDEO_MOTION_ALARM_PARAM:
				printf("CMD: SET_VIDEO_MOTION_ALARM_PARAM\n");
				ret = SESSION_setVideoMotionAlarm(pMsgHead, pRecvBuf);
				break;

			case GET_PROBE_IN_ALARM_PARAM:
				printf("CMD: GET_PROBE_IN_ALARM_PARAM\n");
				ret = SESSION_getProbeInAlarm(pMsgHead, pRecvBuf);
				break;

			case SET_PROBE_IN_ALARM_PARAM:
				printf("CMD: SET_PROBE_IN_ALARM_PARAM\n");
				ret = SESSION_setProbeInAlarm(pMsgHead, pRecvBuf);
				break;

			case GET_PROBE_OUT_PARAM:
				printf("CMD: GET_PROBE_OUT_PARAM\n");
				ret = SESSION_getProbeOutAlarm(pMsgHead, pRecvBuf);
				break;

			case SET_PROBE_OUT_PARAM:
				printf("CMD: SET_PROBE_OUT_PARAM\n");
				ret = SESSION_setProbeOutAlarm(pMsgHead, pRecvBuf);
				break;

			case SET_IRPROBE_ALARM_START:
				printf("CMD: SET_IRPROBE_ALARM_START\n");
				ret = SESSION_setIrprobe_alarm_start(pMsgHead, pRecvBuf);
				break;

			case SET_IRPROBE_ALARM_STOP:
				printf("CMD: SET_IRPROBE_ALARM_STOP\n");
				ret = SESSION_setIrprobe_alarm_stop(pMsgHead, pRecvBuf);
				break;				

#if 1
/*********************无线报警命令 开始****************************************/
			case SET_IRPROBE_ALARM_PARAM:
				printf("CMD: SET_IRPROBE_ALARM_PARAM\n");
				ret = SESSION_setProbeInAlarm(pMsgHead, pRecvBuf);
				break;
			case GET_IRPROBE_ALARM_PARAM:
				printf("CMD: GET_IRPROBE_ALARM_PARAM\n");
				ret = SESSION_getProbeInAlarm(pMsgHead, pRecvBuf);
				break;		
			case SET_IRPROBE_ALARM_STUDY:
				printf("CMD: SET_IRPROBE_ALARM_STUDY\n");
				ret = SESSION_setAlarmStudy(pMsgHead, pRecvBuf);
				break;
			case SET_IRPROBE_ALARM_RESET:
				printf("CMD: SET_IRPROBE_ALARM_RESET\n");
				ret = SESSION_setAlarmReset(pMsgHead, pRecvBuf);
				break;
/*********************无线报警命令 结束****************************************/
#endif
				

			case GET_FTP_PARAM:
				printf("CMD: GET_FTP_PARAM\n");
				ret = SESSION_getFtp(pMsgHead, pRecvBuf);
				break;

			case SET_FTP_PARAM:
				printf("CMD: SET_FTP_PARAM\n");
				ret = SESSION_setFtp(pMsgHead, pRecvBuf);
				break;

				
			//add code by lvjh 2012-08-18
			#if 1
			case GET_RTSP_PARAM:
				printf("CMD: GET_RTSP_PARAM\n");
				ret = SESSION_getRtsp(pMsgHead, pRecvBuf);
				break;
			case SET_RTSP_PARAM:
				printf("CMD: SET_RTSP_PARAM\n");
				ret = SESSION_setRtsp(pMsgHead, pRecvBuf);
				break;
			#endif

			case GET_TFTP_PARAM:
				printf("CMD: GET_TFTP_PARAM\n");
				ret = SESSION_getTftp(pMsgHead, pRecvBuf);
				break;

			case SET_TFTP_PARAM:
				printf("CMD: SET_TFTP_PARAM\n");
				ret = SESSION_setTftp(pMsgHead, pRecvBuf);
				break;

			case GET_EMAIL_PARAM:
				printf("CMD: GET_EMAIL_PARAM\n");
				ret = SESSION_getEmail(pMsgHead, pRecvBuf);
				break;

			case SET_EMAIL_PARAM:
				printf("CMD: SET_EMAIL_PARAM\n");
				ret = SESSION_setEmail(pMsgHead, pRecvBuf);
				break;

			case GET_AUTO_SNAPSHOT_PARAM:
				printf("CMD: GET_AUTO_SNAPSHOT_PARAM\n");
				ret = SESSION_getJpegStoreIP(pMsgHead, pRecvBuf);
				break;

			case SET_AUTO_SNAPSHOT_PARAM:
				printf("CMD: SET_AUTO_SNAPSHOT_PARAM\n");
				ret = SESSION_setJpegStoreIP(pMsgHead, pRecvBuf);
				break;

			case GET_NETWORK_PARAM:
				printf("CMD: GET_NETWORK_PARAM\n");
				ret = SESSION_getNet(pMsgHead, pRecvBuf);
				break;

			case SET_NETWORK_PARAM:
				printf("CMD: SET_NETWORK_PARAM\n");
				ret = SESSION_setNet(pMsgHead, pRecvBuf);
				break;

			case GET_REMOTE_CONNECT_PARAM:
				printf("CMD: GET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_getRemoteConnect(pMsgHead, pRecvBuf);
				break;

			case SET_REMOTE_CONNECT_PARAM:
				printf("CMD: SET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_setRemoteConnect(pMsgHead, pRecvBuf);
				break;
				
			case NETCMD_P2P_REGDATAPORT:
				printf("CMD: SET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_DVSNET_REGISTER(pMsgHead, pRecvBuf);
				break;

			// register P2P server connect
			case GET_REMOTE_P2P_CONNECT_PARAM:
				printf("CMD: GET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_getP2PRemoteConnect(pMsgHead, pRecvBuf);
				break;
				
			case NETCMD_P2P_REQCONBACK:
				printf("CMD: SET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_setRemoteP2PConnect(pMsgHead, pRecvBuf);
				break;
				
			case NETCMD_P2P_ENDCONBACK:
				printf("CMD: SET_REMOTE_CONNECT_PARAM\n");
				ret = SESSION_setP2P_ENDCONBACK(pMsgHead, pRecvBuf);
				break;

			case GET_PPPOE_PARAM:
				printf("CMD: GET_PPPOE_PARAM\n");
				ret = SESSION_getPPPOE(pMsgHead, pRecvBuf);
				break;

			case SET_PPPOE_PARAM:
				printf("CMD: SET_PPPOE_PARAM\n");
				ret = SESSION_setPPPOE(pMsgHead, pRecvBuf);
				break;

			case GET_DDNS_PARAM:
				printf("CMD: GET_DDNS_PARAM\n");
				ret = SESSION_getDDNS(pMsgHead, pRecvBuf);
				break;

			case SET_DDNS_PARAM:
				printf("CMD: SET_DDNS_PARAM\n");
				ret = SESSION_setDDNS(pMsgHead, pRecvBuf);
				break;

			case GET_ALARM_STATUS:
				printf("CMD: GET_ALARM_STATUS\n");
				ret = SESSION_getAlarmStatus(pMsgHead, pRecvBuf);
				break;
				
 			case SET_ALARM_STATUS:
				printf("CMD: SET_ALARM_STATUS\n");
				ret = SESSION_setAlarmStatus(pMsgHead, pRecvBuf);
				break;
				
			case SET_ALL_IRALARM_STATUS:
				printf("CMD: SET_ALL_IRALARM_STATUS\n");
				ret = SESSION_setAllIrAlarmStatus(pMsgHead, pRecvBuf);
				break;

			case SET_TIME_REBOOT:
				printf("CMD: SET_TIME_REBOOT\n");
				ret = SESSION_setTimeReboot(pMsgHead, pRecvBuf);
				break;
				
 			case GET_TIME_REBOOT:
				printf("CMD: GET_TIME_REBOOT\n");
				ret = SESSION_getTimeReboot(pMsgHead, pRecvBuf);
				break;

 #ifdef YIYUAN
			case GET_YIYUAN_DDNS_PARAM:
				printf("CMD: GET_YIYUAN_DDNS_PARAM\n");
				ret = SESSION_getYIYUAN_DDNS(pMsgHead, pRecvBuf);
				break;
			
			case SET_YIYUAN_DDNS_PARAM:
				printf("CMD: SET_YIYUAN_DDNS_PARAM\n");
				ret = SESSION_setYIYUAN_DDNS(pMsgHead, pRecvBuf);
				break;
#endif


#ifdef RS_485
			case GET_CHANNEL_PTZ_PARAM:
				printf("CMD: GET_CHANNEL_PTZ_PARAM\n");
				ret = SESSION_getPTZ(pMsgHead, pRecvBuf);
				break;

			case SET_CHANNEL_PTZ_PARAM:
				printf("CMD: SET_CHANNEL_PTZ_PARAM\n");
				ret = SESSION_setPTZ(pMsgHead, pRecvBuf);
				break;

			case GET_RS485_COM_PARAM:
				printf("CMD: GET_RS485_COM_PARAM\n");
				ret = SESSION_getRs485COM(pMsgHead, pRecvBuf);
				break;

			case SET_RS485_COM_PARAM:
				printf("CMD: SET_RS485_COM_PARAM\n");
				ret = SESSION_setRs485COM(pMsgHead, pRecvBuf);
				break;

			case GET_RS232_COM_PARAM:
				printf("CMD: SET_RS485_COM_PARAM\n");
				ret = SESSION_getRs232COM(pMsgHead, pRecvBuf);
				break;

			case SET_RS232_COM_PARAM:
				printf("CMD: SET_RS232_COM_PARAM\n");
				ret = SESSION_setRs232COM(pMsgHead, pRecvBuf);
				break;

			case RS485_BYPASS_COM:
				printf("CMD: RS485_BYPASS_COM\n");
				ret = SESSION_rs485Through(pMsgHead, pRecvBuf);
				break;

			case RS232_BYPASS_COM:
				printf("CMD: RS232_BYPASS_COM\n");
				ret = SESSION_rs232Through(pMsgHead, pRecvBuf);
				break;

			case GET_PTZ_LIST:
				printf("CMD: GET_PTZ_LIST\n");
				ret = SESSION_getPTZList(pMsgHead, pRecvBuf);
				break;

			case ADD_PTZ:
				printf("CMD: ADD_PTZ\n");
				ret = SESSION_AddPTZ(pMsgHead, pRecvBuf);
				break;

			case DELETE_PTZ:
				printf("CMD: DELETE_PTZ\n");
				ret = SESSION_DeletePTZ(pMsgHead, pRecvBuf);
				break;
#endif
			case CTRL_PTZ:
				printf("CMD: CTRL_PTZ\n");
				ret = SESSION_CtrlPTZ(pMsgHead, pRecvBuf);
				break;

			case GET_REMOTE_TALK_IP:
				printf("CMD: GET_REMOTE_TALK_IP\n");
				ret = SESSION_getRemoteTalkIP(pMsgHead, pRecvBuf);
				break;

			case SET_REMOTE_TALK_IP:
				printf("CMD: SET_REMOTE_TALK_IP\n");
				ret = SESSION_setRemoteTalkIP(pMsgHead, pRecvBuf);
				break;

			case GET_DEV_TIME:
				printf("CMD: GET_DEV_TIME\n");
				ret = SESSION_getSysTime(pMsgHead, pRecvBuf);
				break;

			case SET_DEV_TIME:
				printf("CMD: SET_DEV_TIME\n");
				ret = SESSION_setSysTime(pMsgHead, pRecvBuf);
				break;
				
			case GET_NTP_PARAM:
				printf("CMD: GET_NTP_PARAM\n");
				ret = SESSION_getNtp(pMsgHead, pRecvBuf);
				break;

			case SET_NTP_PARAM:
				printf("CMD: SET_NTP_PARAM\n");
				ret = SESSION_setNtp(pMsgHead, pRecvBuf);
				break;

			case 0x1006c:
				printf("CMD: GET_UPNP_PARAM\n");
				ret = SESSION_getUpnp(pMsgHead, pRecvBuf);
				break;

			case 0x1006b:
				printf("CMD: SET_UPNP_PARAM\n");
				ret = SESSION_setUpnp(pMsgHead, pRecvBuf);
				break;
				
			case SET_P2P_PARAM:
				printf("CMD: SET_P2P_PARAM\n");
				//ret = SESSION_setP2P(pMsgHead, pRecvBuf);
				ret = SESSION_setRemoteP2PConnect(pMsgHead, pRecvBuf);

				break;
			case GET_P2P_PARAM:
				printf("CMD: GET_P2P_PARAM\n");
				ret = SESSION_getP2P(pMsgHead, pRecvBuf);
				break;

			case GET_WIFI_PARAM:
				printf("CMD: GET_WIFI_PARAM\n");
				ret = SESSION_getWifi(pMsgHead, pRecvBuf);
				break;

			case SET_WIFI_PARAM:
				printf("CMD: SET_WIFI_PARAM\n");
				ret = SESSION_setWifi(pMsgHead, pRecvBuf);
				break;
				
			case SCAN_WIFI_PARAM:
				printf("CMD: SCAN_WIFI_PARAM\n");
				ret = SESSION_scanWifi(pMsgHead, pRecvBuf);
				break;

			case MANUAL_SNAPSHOT:
				printf("CMD: MANUAL_SNAPSHOT\n");
				ret = SESSION_ManualSnapshop(pMsgHead, pRecvBuf);
				break;

			case GET_PROBE_STATUS:
				printf("CMD: GET_PROBE_STATUS\n");
				ret = SESSION_getProbeStatus(pMsgHead, pRecvBuf);
				break;

			case SET_PROBE_STATUS:
				printf("CMD: SET_PROBE_STATUS\n");
				ret = SESSION_setProbeStatus(pMsgHead, pRecvBuf);
				break;

			case SAVE_DEV_PARAM:
				printf("CMD: SAVE_DEV_PARAM\n");
				ret = SESSION_SaveSysParam(pMsgHead, pRecvBuf);
				break;

			case RESUME_DEV_DEFAULT_PARAM:
				printf("CMD: RESUME_DEV_DEFAULT_PARAM\n");
				ret = SESSION_ResumeDefaultParam(pMsgHead, pRecvBuf);
				break;

			case REBOOT_DEV:
				printf("CMD: REBOOT_DEV\n");
				ret = SESSION_RebootDev(pMsgHead, pRecvBuf);
				break;
				
			case RESUME_DEFAULT_COLOR:
				printf("CMD: RESUME_DEFAULT_COLOR\n");
				ret = SESSION_ResumeDefaultColor(pMsgHead, pRecvBuf);
				break;
		
#ifdef HD_CMOS
			case GET_IMAGE_FLIP:
				printf("CMD: GET_IMAGE_FLIP\n");
				ret = SESSION_getImageFlip(pMsgHead, pRecvBuf);
				break;

			case SET_IMAGE_FLIP:
				printf("CMD: SET_IMAGE_FLIP\n");
				ret = SESSION_setImageFlip(pMsgHead, pRecvBuf);
				break;
				
			case GET_IMAGE_MIRROR:
				printf("CMD: GET_IMAGE_MIRROR\n");
				ret = SESSION_getImageMirror(pMsgHead, pRecvBuf);
				break;

			case SET_IMAGE_MIRROR:
				printf("CMD: SET_IMAGE_MIRROR\n");
				ret = SESSION_setImageMirror(pMsgHead, pRecvBuf);
				break;

			case GET_IMAGE_HZ:
				printf("CMD: GET_IMAGE_HZ\n");
				ret = SESSION_getImageHz(pMsgHead, pRecvBuf);
				break;

			case SET_IMAGE_HZ:
				printf("CMD: SET_IMAGE_HZ\n");
				ret = SESSION_setImageHz(pMsgHead, pRecvBuf);
				break;
#endif

			case GET_AUDIO_INPUT:
				printf("CMD: GET_AUDIO_INPUT\n");
				ret = SESSION_getAudioInPath(pMsgHead, pRecvBuf);
				break;

			case SET_AUDIO_INPUT:
				printf("CMD: SET_AUDIO_INPUT\n");
				ret = SESSION_setAudioInPath(pMsgHead, pRecvBuf);
				break;
				
			case GET_G3_PARAM:
				printf("CMD: GET_G3_PARAM\n");
				ret = SESSION_get3G(pMsgHead, pRecvBuf);
				break;

			case SET_G3_PARAM:
				printf("CMD: SET_G3_PARAM\n");
				ret = SESSION_set3G(pMsgHead, pRecvBuf);
				break;	
				
			case GET_SCHEDULE_SNAPSHOT_PARAM:
				printf("CMD: GET_SCHEDULE_SNAPSHOT_PARAM\n");
				ret = SESSION_getScheduleSnapshot(pMsgHead, pRecvBuf);
				break;

			case SET_SCHEDULE_SNAPSHOT_PARAM:
				printf("CMD: SET_SCHEDULE_SNAPSHOT_PARAM\n");
				ret = SESSION_setScheduleSnapshot(pMsgHead, pRecvBuf);
				break;
				
			case GET_PTZ_AUTO_CTRL:
				printf("CMD: GET_PTZ_AUTO_CTRL\n");
				ret = SESSION_getPtzAutoCtrl(pMsgHead, pRecvBuf);
				break;
				
			case SET_PTZ_AUTO_CTRL:
				printf("CMD: SET_PTZ_AUTO_CTRL\n");
				ret = SESSION_setPtzAutoCtrl(pMsgHead, pRecvBuf);
				break;
				
			case GET_DEVICE_CONFIGURE:
				printf("CMD: GET_DEVICE_CONFIGURE\n");
				ret = SESSION_getDeviceConfigure(pMsgHead, pRecvBuf);
				break;

			case SET_DEVICE_CONFIGURE:
				printf("CMD: SET_DEVICE_CONFIGURE\n");
				ret = SESSION_setDeviceConfigure(pMsgHead, pRecvBuf);
				break;
			
			// Add the code by lvjh, 2010-05-24
			case SAVE_DEV_PARAM_FILE:
				printf("CMD: SAVE_DEV_PARAM_FILE\n");
				ret = SESSION_saveDevParamFile(pMsgHead, pRecvBuf);
				break;	
				
			case RESUME_DEV_PARAM_FILE:
				printf("CMD: RESUME_DEV_PARAM_FILE\n");
				ret = SESSION_ResumeDevParamFile(pMsgHead, pRecvBuf);
				break;						

#ifdef RECORD
			case GET_RECORD_PARAM:
				printf("CMD: GET_RECORD_PARAM\n");
				ret = SESSION_getRecord(pMsgHead, pRecvBuf);
				break;

			case SET_RECORD_PARAM:
				printf("CMD: SET_RECORD_PARAM\n");
				ret = SESSION_setRecord(pMsgHead, pRecvBuf);
				break;

			case GET_TIMER_RECORD_PARAM:
				printf("CMD: GET_TIMER_RECORD_PARAM\n");
				ret = SESSION_getTimerRecord(pMsgHead, pRecvBuf);
				break;

			case SET_TIMER_RECORD_PARAM:
				printf("CMD: SET_TIMER_RECORD_PARAM\n");
				ret = SESSION_setTimerRecord(pMsgHead, pRecvBuf);
				break;

			case GET_MANUAL_RECORD_PARAM:
				printf("CMD: GET_MANUAL_RECORD_PARAM\n");
				ret = SESSION_getManualRecord(pMsgHead, pRecvBuf);
				break;

			case SET_MANUAL_RECORD_PARAM:
				printf("CMD: SET_MANUAL_RECORD_PARAM\n");
				ret = SESSION_setManualRecord(pMsgHead, pRecvBuf);
				break;

			case QUERY_RECORD_DATE:
				printf("CMD: QUERY_RECORD_DATE\n");
				ret = SESSION_queryRecordDate(pMsgHead, pRecvBuf);
				break;

			case QUERY_RECORD_FILE:
				printf("CMD: QUERY_RECORD_FILE\n");
				ret = SESSION_queryRecordFile(pMsgHead, pRecvBuf);
				break;

			case QUERY_JPEG_DATE:
				printf("CMD: QUERY_JPEG_DATE\n");
				ret = SESSION_queryJpegDate(pMsgHead, pRecvBuf);
				break;

			case QUERY_JPEG_FILE:
				printf("CMD: QUERY_JPEG_FILE\n");
				ret = SESSION_queryJpegFile(pMsgHead, pRecvBuf);
				break;
				
			case GET_HARDDISK_INFO:
				printf("CMD: GET_HARDDISK_INFO\n");
				ret = SESSION_getHardDiskInfo(pMsgHead, pRecvBuf);
				break;

			case SET_HARDDISK_FDISK:
				printf("CMD: SET_HARDDISK_FDISK\n");
				ret = SESSION_setHardDiskFdisk(pMsgHead, pRecvBuf);
				break;

			case SET_HARDDISK_FORMAT:
				printf("CMD: SET_HARDDISK_FORMAT1\n");
				ret = SESSION_setHardDiskFormat1(pMsgHead, pRecvBuf);
				break;

			case GET_FDISK_PROGRESS:
				printf("CMD: GET_FDISK_PROGRESS\n");
				ret = SESSION_getFdiskProgress(pMsgHead, pRecvBuf);
				break;

			case REMOTE_RECORD_PLAYBACK:
				printf("CMD: REMOTE_RECORD_PLAYBACK\n");
				//ret = SESSION_remoteRecordPlayback(pMsgHead, pRecvBuf);
				break;

			case LOCAL_RECORD_PLAYBACK:
				printf("CMD: LOCAL_RECORD_PLAYBACK\n");
				//ret = SESSION_locateRecordPlayback(pMsgHead, pRecvBuf);
				break;

			case REMOTE_RECORD_BACKUP:
				printf("CMD: REMOTE_RECORD_BACKUP\n");
				//ret = SESSION_remoteRecordBackup(pMsgHead, pRecvBuf);
				break;

			case LOCAL_RECORD_BACKUP:
				printf("CMD: LOCAL_RECORD_BACKUP\n");
				//ret = SESSION_locateRecordBackup(pMsgHead, pRecvBuf);
				break;

			case DELETE_RECORD_FILE:
				printf("CMD: DELETE_RECORD_FILE\n");
				//ret = SESSION_deleteRecordFile(pMsgHead, pRecvBuf);
				break;			
#endif

			default:
				{
					returnMsg.nCmd = NETCMD_USER_CMD;
					returnMsg.nRight = pMsgHead->nRight;
					returnMsg.nErrorCode = 0;
					returnMsg.nBufSize = sizeof(DEV_CMD_RETURN);

					devCmdReturn.nCmdID = devCmdHeader.nCmdID;
					devCmdReturn.nCmdLen = 0;
					devCmdReturn.nResult = CMD_NOT_SUPPORT;
					devCmdReturn.nReserve = 0;

					ret = NETSDK_SendMsg(&returnMsg, (char *)&devCmdReturn);

					printf("CMD: not support(%x)\n", devCmdHeader.nCmdID);
				}
				break;
			}			
		}
		break;

	default:
		break;
	}

	//printf("recvCmdProcFun OK\n");

	return ret;
}

int CGI_ProcFun(int nCmd, char *pParam)
{
	int nRet = -1;
	
	printf("CGI_ProcFun: %d\n", nCmd);
	
	switch (nCmd)
	{
	case CTRL_PTZ:
		{
			PTZ_CMD param;

			memcpy(&param, pParam, sizeof(PTZ_CMD));
			
			nRet = ptzControl(0, param);
			if (param.nCmd == UP_START || param.nCmd == DOWN_START)
			{
				usleep(500*1000);
			}
			else
			{
				sleep(1);
			}
			param.nCmd = UP_STOP;
			nRet = ptzControl(0, param);
		}
		break;
		
	default:
		break;
	}
	
	return 0;
}



