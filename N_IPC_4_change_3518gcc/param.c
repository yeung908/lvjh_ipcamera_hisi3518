#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>

#include "param.h"
#include "util.h"
#include "alarmProc.h"
#include "vadcDrv.h"
#include "ptz.h"
#include "videoEncAppModule.h"

// 全局变量
SYS_PARAM g_sys_param;
pthread_mutex_t g_sys_param_mutex;

/***********************************************************************************
函数功能:获取系统参数(这个地方基本上是写死了，只有服务器ID号上从
		参数配置文件中获取到到
函数参数:sysinfo(存放到指针)
***********************************************************************************/
int getSysInfo(SYS_INFO *sysInfo)
{
	int ret = 0;
	char pID[48];
	
	if (sysInfo == NULL)
	{
		return -1;
	}
	
	memset(pID, 0, 48);
	ret = getIDConfigure(pID);
	if (ret == 0)
	{
		memcpy(sysInfo->strDeviceID, pID, 48);
	}

	sysInfo->nHardwareVersion = HARDWARE_VERSION;
	
	sysInfo->nSoftwareVersion = SOFTWARE_VERSION;
	sysInfo->nSoftwareBuildDate = SOFTWARE_COMPILE_DATE;


	sysInfo->nCpuType = 0x03;	

	sysInfo->nCpuFreq = 0xF0;
	sysInfo->nRamSize = 0x40;
	sysInfo->nChnNum = MAX_CHANNEL;

	sysInfo->nVideoEncType = ENCODE_VIDEO_HISI; // H264
#ifdef G726
	sysInfo->nAudioEncType = ENCODE_AUDIO_G726; // G726  change by zhangjing 2013-06-09
#else
	sysInfo->nAudioEncType = ENCODE_AUDIO_G711; // G711  change by zhangjing 2013-06-09
#endif

#ifdef CCD
	sysInfo->byDeviceType = 0x05;				// CCD IP CAMERA
#endif
#ifdef HD_CMOS
//mody by lv old:nothing--start
#ifdef CCD_CMOS
			sysInfo->byDeviceType = 0x04;				// HD CMOS IP CAMERA
#else
//mody by lv old:nothing--end
			sysInfo->byDeviceType = 0x06;				// HD CMOS IP CAMERA
#endif
#endif

		
	sysInfo->byAlarmInNum = MAX_PROBE_IN;
	sysInfo->byAlarmOutNum = MAX_PROBE_OUT;
	sysInfo->byRS232Num = 0x00;
#ifdef RS_485		
	sysInfo->byRS485Num = 0x01;
#else
	sysInfo->byRS485Num = 0x00;
#endif		
	sysInfo->byDiskNum = 0x00;
	sysInfo->byVGANum = 0x00;
	sysInfo->byUSBNum = 0x01;
	
	sysInfo->nDoubleStreamFlag = 1;
	
	printf("SOFTWARE VERSION(2): %x %x\n", sysInfo->nSoftwareBuildDate, SOFTWARE_COMPILE_DATE);

	return 0;	
}


int initSystemParam(int flag)
{
	int i = 0;
	int j = 0;
	int ret = -1;
	int nParamFlag = HARDWARE_RESET;
	
	NET_PARAM network; 
#ifdef WIFI_RALINK
	WIFI_PARAM wifiParam;
#endif
	
	ret = pthread_mutex_init(&g_sys_param_mutex, NULL);
	if (ret != 0)
	{
		printf("initSystemParam() Failed!\n");
		return -1;
	}
	
	memset(&g_sys_param, 0, sizeof(SYS_PARAM));
	
	if (flag == HARDWARE_RESET)
	{
		nParamFlag = HARDWARE_RESET;
	}
	else
	{
		ret = getParamFromFile();
		if (ret != 0)
		{
			nParamFlag = HARDWARE_RESET;
		}
		else
		{
			nParamFlag = SOFTWARE_RESET;
		}
	}
	

	// 初始化
	if (nParamFlag == HARDWARE_RESET)
	{
		setParamStatusToFile(HARDWARE_RESET);
	}
	else
	{
		setParamStatusToFile(SOFTWARE_RESET);
	}
	
	// 系统信息
	getSysInfo(&g_sys_param.sysInfo);
	strcpy(g_sys_param.sysInfo.strDeviceName, "IP_CAMERA");
	g_sys_param.sysInfo.nLanguage = 0;	// 0:中文，1:英文
	
	// 
	strcpy(g_sys_param.userInfo.Admin.strName, ADMIN_USER);
	strcpy(g_sys_param.userInfo.Admin.strPsw, ADMIN_PASSWD);
	g_sys_param.userInfo.Admin.nRight[0] = 0xFFFFFFFF;
	g_sys_param.userInfo.Admin.nRight[1] = 0xFFFFFFFF;
	g_sys_param.userInfo.Admin.nRight[2] = 0xFFFFFFFF;
	g_sys_param.userInfo.Admin.nRight[3] = 0xFFFFFFFF;	
	strcpy(g_sys_param.userInfo.Guest.strName, GUEST_USER);
	strcpy(g_sys_param.userInfo.Guest.strPsw, GUEST_USER);
	g_sys_param.userInfo.Guest.nRight[0] = 0x00;
	g_sys_param.userInfo.Guest.nRight[1] = 0x00;
	g_sys_param.userInfo.Guest.nRight[2] = 0x00;
	g_sys_param.userInfo.Guest.nRight[3] = 0x00;
	setUSERConfigure(&g_sys_param.userInfo);

	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.videoStandard[i].nStandard = 0x00;	// PAL
	}
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.videoInAttr[i].nBrightness = DEFAULT_BRIGHTNESS;
		g_sys_param.videoInAttr[i].nContrast = DEFAULT_CONTRAST;
		g_sys_param.videoInAttr[i].nHue = DEFAULT_HUE;
		g_sys_param.videoInAttr[i].nSaturation = DEFAULT_SATURATION;
		g_sys_param.videoInAttr[i].reserve = 0x00;

	}

	for (i=0; i<MAX_CHANNEL; i++)
	{
		// HD_CMOS
#ifdef HD_CMOS
		g_sys_param.videoEnc[i][0].nEncodeMode = 0x00;
#ifdef CCD_CMOS
		g_sys_param.videoEnc[i][0].nEncodeWidth = 640;
		g_sys_param.videoEnc[i][0].nEncodeHeight = 480;
#else
		g_sys_param.videoEnc[i][0].nEncodeWidth = 1280;
		g_sys_param.videoEnc[i][0].nEncodeHeight = 720;
#endif
		g_sys_param.videoEnc[i][0].nKeyInterval = 100;
		g_sys_param.videoEnc[i][0].nFramerate = 25;  //old 30 to 25
		g_sys_param.videoEnc[i][0].nBitRate = 1000000;
		g_sys_param.videoEnc[i][0].nMaxQuantizer = 31;
		g_sys_param.videoEnc[i][0].nMinQuantizer = 2;
		g_sys_param.videoEnc[i][0].reserve = 0;	
		
		g_sys_param.videoEnc[i][1].nEncodeMode = 0x00;

#ifdef CCD_CMOS
		g_sys_param.videoEnc[i][1].nEncodeWidth = 320;
		g_sys_param.videoEnc[i][1].nEncodeHeight = 240;
		g_sys_param.videoEnc[i][1].nBitRate = 200000;

#else
		g_sys_param.videoEnc[i][1].nEncodeWidth = 400;
		g_sys_param.videoEnc[i][1].nEncodeHeight = 304;
#endif
		
		g_sys_param.videoEnc[i][1].nKeyInterval = 20;
		g_sys_param.videoEnc[i][1].nFramerate = 13;
		g_sys_param.videoEnc[i][1].nBitRate = 400000;
		g_sys_param.videoEnc[i][1].nMaxQuantizer = 31;
		g_sys_param.videoEnc[i][1].nMinQuantizer = 2;
		g_sys_param.videoEnc[i][0].reserve = 0;	
		
#endif
		
		// CCD
#ifdef CCD		
		g_sys_param.videoEnc[i][0].nEncodeMode = 0x00;
		g_sys_param.videoEnc[i][0].nEncodeWidth = 720;
		g_sys_param.videoEnc[i][0].nEncodeHeight = 576;
		g_sys_param.videoEnc[i][0].nKeyInterval = 100;
		g_sys_param.videoEnc[i][0].nFramerate = 25;
		//g_sys_param.videoEnc[i][0].nBitRate = 2048000;
		g_sys_param.videoEnc[i][0].nBitRate = 2000000;
		g_sys_param.videoEnc[i][0].nMaxQuantizer = 31;
		g_sys_param.videoEnc[i][0].nMinQuantizer = 2;
		g_sys_param.videoEnc[i][0].reserve = 0;	
		
		g_sys_param.videoEnc[i][1].nEncodeMode = 0x00;
		g_sys_param.videoEnc[i][1].nEncodeWidth = 352;
		g_sys_param.videoEnc[i][1].nEncodeHeight = 288;
		g_sys_param.videoEnc[i][1].nKeyInterval = 100;
		g_sys_param.videoEnc[i][1].nFramerate = 25;
		g_sys_param.videoEnc[i][1].nBitRate = 384000;
		g_sys_param.videoEnc[i][1].nMaxQuantizer = 31;
		g_sys_param.videoEnc[i][1].nMinQuantizer = 2;
		g_sys_param.videoEnc[i][1].reserve = 0;	
		
#endif		
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.audioEnc[i].nOnFlag = 0x01;		// 音频开发
		g_sys_param.audioEnc[i].nChannels = 0x01;
		g_sys_param.audioEnc[i].nSampleRate = AUDIOSAMPLES;

		g_sys_param.audioEnc[i].nBitRate = AUDIOSAMPLES;
		g_sys_param.audioEnc[i].nMode = 0x03;
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.osd[i].TimeOSD.bShow = 0x01;
		g_sys_param.osd[i].TimeOSD.x = 16;
		g_sys_param.osd[i].TimeOSD.y = 20;
		g_sys_param.osd[i].TimeOSD.nColor = 0x00;

		g_sys_param.osd[i].BitsOSD.bShow = 0x01;
		g_sys_param.osd[i].BitsOSD.x = 16;
		g_sys_param.osd[i].BitsOSD.y = 40;
		g_sys_param.osd[i].BitsOSD.nColor = 0x00;

		for (j=0; j<4; j++)
		{
			g_sys_param.osd[i].TitleOSD[j].bShow = 0x01;
			g_sys_param.osd[i].TitleOSD[j].x = 16;
			g_sys_param.osd[i].TitleOSD[j].y = 60;
			g_sys_param.osd[i].TitleOSD[j].nColor = 0x00;
			strcpy(g_sys_param.osd[i].TitleOSD[j].sTitle, "   ");
			
		}
	}

	// 视频丢失报警参数
	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.videoLostAlarm[i].nLinkProbe = 0;
		g_sys_param.videoLostAlarm[i].nLinkProbeTime = 5;
		g_sys_param.videoLostAlarm[i].nLinkRecord = i;
		g_sys_param.videoLostAlarm[i].nLinkRecordTime = 2;
		g_sys_param.videoLostAlarm[i].nLinkSnapshot = i;
		g_sys_param.videoLostAlarm[i].nLinkSnapshotInterval = 10;
		g_sys_param.videoLostAlarm[i].nLinkSnapshotUploadFlag = 0;
		g_sys_param.videoLostAlarm[i].nLinkSnapshotUploadMode = 0;
		for (j=0; j<8; j++)
		{
			g_sys_param.videoLostAlarm[i].day[j].nOnFlag = 0;
			
			g_sys_param.videoLostAlarm[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.videoLostAlarm[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.videoLostAlarm[i].day[j].time_segment[1].end_minute = 59;
		}
		
		g_sys_param.videoLostAlarm[i].reserve = 0;
	}
	
	// 视频移动报警参数
	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.videoMotionAlarm[i].nLinkProbe = 0;
		g_sys_param.videoMotionAlarm[i].nLinkProbeTime = 5;
		g_sys_param.videoMotionAlarm[i].nLinkRecord = i;
		g_sys_param.videoMotionAlarm[i].nLinkRecordTime = 2;
		g_sys_param.videoMotionAlarm[i].nLinkSnapshot = i;
		g_sys_param.videoMotionAlarm[i].nLinkSnapshotInterval = 5;
		g_sys_param.videoMotionAlarm[i].nLinkSnapshotUploadFlag = 0;
		g_sys_param.videoMotionAlarm[i].nLinkSnapshotUploadMode = 0;
		for (j=0; j<8; j++)
		{
			g_sys_param.videoMotionAlarm[i].day[j].nOnFlag = 0;
			
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.videoMotionAlarm[i].day[j].time_segment[1].end_minute = 59;
		}
	
		g_sys_param.videoMotionAlarm[i].reserve = 0;
		
	}
	
	// 探头报警参数
	for (i=0; i<MAX_PROBE_IN; i++)
	{
		g_sys_param.probeInAlarm[i].nLinkProbe = 0;
		g_sys_param.probeInAlarm[i].nLinkProbeTime = 5;
		g_sys_param.probeInAlarm[i].nLinkRecord = i;
		g_sys_param.probeInAlarm[i].nLinkRecordTime = 2;
		g_sys_param.probeInAlarm[i].nLinkSnapshot = i;
		g_sys_param.probeInAlarm[i].nLinkSnapshotInterval = 5;
		g_sys_param.probeInAlarm[i].nLinkSnapshotUploadFlag = 0;
		g_sys_param.probeInAlarm[i].nLinkSnapshotUploadMode = 0;
		g_sys_param.probeInAlarm[i].bPresetNo[0] = 1;
		g_sys_param.probeInAlarm[i].reserve = 1;
		
		for (j=0; j<8; j++)
		{
			#ifdef HBSX
				g_sys_param.probeInAlarm[i].day[j].nOnFlag = 0;
			#else 
				g_sys_param.probeInAlarm[i].day[0].nOnFlag = 1;
			
			#endif
			
			g_sys_param.probeInAlarm[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.probeInAlarm[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.probeInAlarm[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.probeInAlarm[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.probeInAlarm[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.probeInAlarm[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.probeInAlarm[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.probeInAlarm[i].day[j].time_segment[1].end_minute = 59;
		}
		
		//memset(g_sys_param.probeInAlarm[i].bPresetNo, 0, 32);
		memset(g_sys_param.probeInAlarm[i].probeName, 0, 32);	
		g_sys_param.probeInAlarm[i].reserve = 0;
	}

	//定时重启参数
	
	for (j=0; j<8; j++)
	{
		g_sys_param.timerRebootParam.bEnable  = 0;
		g_sys_param.timerRebootParam.nRebootInterval = 24;
		g_sys_param.timerRebootParam.nRebootHour = 10;
		g_sys_param.timerRebootParam.nRebootMinute = 00;
		g_sys_param.timerRebootParam.dwReserved = 0;
		g_sys_param.lastRebootTime = 0;

		
//		saveRebootHourParamFile(&g_sys_param.timerRebootParam.nRebootHour);
//		writeConfigFile(PARAM_REBOOT_HOUR_FILE, g_sys_param.timerRebootParam.nRebootHour, 0);
	}
	

	for (i=0; i<MAX_PROBE_OUT; i++)
	{
		for (j=0; j<8; j++)
		{
			g_sys_param.probeOutAlarm[i].day[j].nOnFlag = 0;
			
			g_sys_param.probeOutAlarm[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.probeOutAlarm[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.probeOutAlarm[i].day[j].time_segment[1].end_minute = 59;
		}
		
		memset(g_sys_param.probeInAlarm[i].probeName, 0, 32);	
	}


	//设置报警状态参数
	g_sys_param.alarm_status.nAlarmInStatus = 1;
	for(i = 0; i < 12; i++)
	#ifdef HBSX
	g_sys_param.alarm_status.nIrAlarmStatus[i] = 0;
	#else 
	g_sys_param.alarm_status.nIrAlarmStatus[i] = 1;
	
	#endif
	g_sys_param.alarm_status.nMotionStatus = 1;
	g_sys_param.alarm_status.nAlarmInStatus = 1;
		
	
	// 网络参数
	if (nParamFlag == HARDWARE_RESET)
	{
		char mac[24];
		
		g_sys_param.network.nDhcpOnFlag = 0x00;
		//g_sys_param.network.nDhcpOnFlag = 0x01;
		strcpy(g_sys_param.network.byServerIp, DEFAULT_WIRE_IP);
		strcpy(g_sys_param.network.byServerMask, DEFAULT_WIRE_MASK);
		strcpy(g_sys_param.network.byGateway, DEFAULT_WIRE_GATEWAY);
		strcpy(g_sys_param.network.byDnsAddr, DEFAULT_WIRE_GATEWAY);
		strcpy(g_sys_param.network.byMultiAddr, DEFAULT_MULTI_ADDR);
		g_sys_param.network.wServerPort = DEFAULT_CMD_PORT;
		g_sys_param.network.wMultiPort = DEFAULT_MULTI_PORT;
		g_sys_param.network.wWebPort = DEFAULT_WEB_PORT;
		g_sys_param.rtsp.nRtspPort	 =  DEFAULT_RTSP_PORT;
		
		// Add the code by lvjh, 2009-09-08
		memset(mac, 0, 24);
		ret = getMACConfigure(mac);
		if (ret == 0)
		{
			memcpy(g_sys_param.network.strPhyAddr, mac, 6);
		}

#ifdef WIFI_RALINK
		g_sys_param.wlan.nOnFlag = 0x0;
		g_sys_param.wlan.Reserve = 1;
		g_sys_param.wlan.nDhcpOnFlag = 0x0;
		g_sys_param.wlan.pReserve[0] = 1;
		strcpy(g_sys_param.wlan.byServerIp, DEFAULT_WIRELESS_IP);
		strcpy(g_sys_param.wlan.byServerMask, DEFAULT_WIRELESS_MASK);
		strcpy(g_sys_param.wlan.byGateway, DEFAULT_WIRELESS_GATEWAY);
		strcpy(g_sys_param.wlan.byDnsAddr, DEFAULT_WIRELESS_GATEWAY);
#endif
	}


//初始化p2p参数
	g_sys_param.upnp.bEnabled = 0;
	g_sys_param.upnp.dwReserved = 0; 
	g_sys_param.upnp.nInterval = 0;
	g_sys_param.upnp.nPort = 0;
//启动rtsp
	g_sys_param.rtsp.bEnable = 1;
	//g_sys_param.rtsp.nRtspPort = 554;

	
//移动报警参数	
	for (i=0; i<MAX_CHANNEL; i++){
		memset(&g_sys_param.motionDetect[i], 0, sizeof(VIDEO_MOTION_PARAM));
		g_sys_param.motionDetect[i].nSensibility = 40;
	}
	


//初始化ftp 参数
	strcpy(g_sys_param.ftp.strPath, "./");
	strcpy(g_sys_param.ftp.strName, "admin");
	strcpy(g_sys_param.ftp.strPsw, "admin");
	strcpy(g_sys_param.ftp.strIP, "192.168.1.135");
	g_sys_param.ftp.nPort = 21;

//初始化ntp服务器
	strcpy(g_sys_param.ntp.Server, "ntp.sjtu.edu.cn");
	g_sys_param.ntp.nInterval = 1;
	g_sys_param.ntp.TimeZone = 8;

	

	
//初始化upnp 参数
	g_sys_param.newUpnpInfo.bEnabled = 0;
	g_sys_param.newUpnpInfo.nReserved = 0;
	g_sys_param.newUpnpInfo.upnpDataStatus = 0;
	g_sys_param.newUpnpInfo.upnpWebStatus = 0;
	g_sys_param.newUpnpInfo.upnpRtspStatus = 0;
	g_sys_param.newUpnpInfo.upnpWebPort = 80;
	g_sys_param.newUpnpInfo.upnpDataPort  = 4000;
	g_sys_param.newUpnpInfo.upnpRtspPort  = 554;


// 初始化ddns 状态	
	//亿源
	g_sys_param.ddns.reserve = 0;
	g_sys_param.ddns.nOnFlag = 0;
	g_sys_param.ddns.authType = 4;
	g_sys_param.ddns.port  = 80;
#ifdef HBSX
	g_sys_param.ddns.reserve = 0;
	g_sys_param.ddns.nOnFlag = 0;
	g_sys_param.ddns.authType = 4;
	g_sys_param.ddns.port  = 86;

#endif
	g_sys_param.yiyuanAlarmDdnsParam.nOnFlag = 0;
	g_sys_param.yiyuanAlarmDdnsParam.nPort = 80;
	g_sys_param.yiyuanAlarmDdnsParam.nTime = 60;
	strcpy(g_sys_param.yiyuanAlarmDdnsParam.sIpAddr, "218.87.154.55");

	// Add the code by lvjh, 2009-09-16
	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.ptzAutoCtrl[i].bEnabled = 0;
		g_sys_param.ptzAutoCtrl[i].nAutoTimeLen = 5;
		g_sys_param.ptzAutoCtrl[i].dwReserved = 0;
	}
		
#ifdef RS_485				// Add the code by lvjh, 2011-03-03
	strcpy(g_sys_param.ptz[0].strName, "pelco-d.cod");
	g_sys_param.ptz[0].nAddr = 1;
	g_sys_param.ptz[0].nRate = 63;
	
	g_sys_param.rs485[0].nBaudRate = 9600;
	g_sys_param.rs485[0].nDataBits = 8;
	g_sys_param.rs485[0].nParity = 0;
	g_sys_param.rs485[0].nStopBits = 1;
	g_sys_param.rs485[0].nFlowCtrl = 0;
	g_sys_param.rs485[0].nReserve = 0;
#endif

#ifdef RECORD
	for (i=0; i<MAX_CHANNEL; i++)
	{
		g_sys_param.recordParam[i].nCoverMode = 1;
		g_sys_param.recordParam[i].nAudioFlag = 1;
		g_sys_param.recordParam[i].nReserve = 0;
		g_sys_param.snapshot[i].dwReserved = 0;
		g_sys_param.snapshot[i].nSnapshotInterval = 10;
		g_sys_param.snapshot[i].nSnapshotUploadMode = 0;
		
		for (j=0; j<8; j++)
		{
			g_sys_param.timerRecordParam[i].day[j].nOnFlag = 0;
			
			g_sys_param.timerRecordParam[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.timerRecordParam[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.timerRecordParam[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.timerRecordParam[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.timerRecordParam[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.timerRecordParam[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.timerRecordParam[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.timerRecordParam[i].day[j].time_segment[1].end_minute = 59;

			g_sys_param.snapshot[i].day[j].nOnFlag = 0;
			g_sys_param.snapshot[i].day[j].time_segment[0].start_hour = 0;
			g_sys_param.snapshot[i].day[j].time_segment[0].start_minute = 0;
			g_sys_param.snapshot[i].day[j].time_segment[0].end_hour = 23;
			g_sys_param.snapshot[i].day[j].time_segment[0].end_minute = 59;
			
			g_sys_param.snapshot[i].day[j].time_segment[1].start_hour = 0;
			g_sys_param.snapshot[i].day[j].time_segment[1].start_minute = 0;
			g_sys_param.snapshot[i].day[j].time_segment[1].end_hour = 23;
			g_sys_param.snapshot[i].day[j].time_segment[1].end_minute = 59;
		}
	}
#endif
	
	return 0;
}

int deInitSystemParam()
{	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memset(&g_sys_param, 0, sizeof(SYS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	pthread_mutex_destroy(&g_sys_param_mutex);
	
	return 0;	
}

int getSysInfoParam(SYS_INFO *param)
{
	int ret = -1;
	char pID[48];

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.sysInfo, sizeof(SYS_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setSysInfoParam(SYS_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(g_sys_param.sysInfo.strDeviceName, param->strDeviceName, 32);
	g_sys_param.sysInfo.nLanguage = param->nLanguage;
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getNtpParam(NTP_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.ntp, sizeof(NTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setNtpParam(NTP_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.ntp, param, sizeof(NTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getUserInfoParam(USER_INFO_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.userInfo, sizeof(USER_INFO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setUserInfoParam(USER_INFO_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.userInfo, param, sizeof(USER_INFO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoInStandardParam(int nChannel, VIDEO_STANDARD_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoStandard[nChannel], sizeof(VIDEO_STANDARD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoInStandardParam(int nChannel, VIDEO_STANDARD_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoStandard[nChannel], param, sizeof(VIDEO_STANDARD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoInAttrParam(int nChannel, VIDEO_IN_ATTR *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoInAttr[nChannel], sizeof(VIDEO_IN_ATTR));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoInAttrParam(int nChannel, VIDEO_IN_ATTR *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoInAttr[nChannel], param, sizeof(VIDEO_IN_ATTR));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoEncParam(int nChannel, int type, VENC_PARAM *param)
{
	int nStandard = 0;
	
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	if (type)
	{
		memcpy(param, &g_sys_param.videoEnc[nChannel][1], sizeof(VENC_PARAM));
	}
	else
	{
		memcpy(param, &g_sys_param.videoEnc[nChannel][0], sizeof(VENC_PARAM));
	}


	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoEncParam(int nChannel, int type, VENC_PARAM *param)
{	
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
		
	pthread_mutex_lock(&g_sys_param_mutex);

	if (type)
	{
		memcpy(&g_sys_param.videoEnc[nChannel][1], param, sizeof(VENC_PARAM));
	}
	else
	{
		memcpy(&g_sys_param.videoEnc[nChannel][0], param, sizeof(VENC_PARAM));
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoWMParam(int nChannel, VIDEO_WM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoWM[nChannel], sizeof(VIDEO_WM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoWMParam(int nChannel, VIDEO_WM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoWM[nChannel], param, sizeof(VIDEO_WM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoChnNameParam(int nChannel, VIDEO_CHANNEL_NAME *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoChnName[nChannel], sizeof(VIDEO_CHANNEL_NAME));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoChnNameParam(int nChannel, VIDEO_CHANNEL_NAME *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoChnName[nChannel], param, sizeof(VIDEO_CHANNEL_NAME));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getOsdParam(int nChannel, OSD_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())

	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
		
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.osd[nChannel], sizeof(OSD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setOsdParam(int nChannel, OSD_PARAM *param)
{	
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.osd[nChannel], param, sizeof(OSD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getLogoParam(int nChannel, LOGO_PARAM *param, char *data)
{		
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.logo[nChannel], sizeof(LOGO_PARAM));
	//memcpy(param, &g_sys_param.logo[nChannel], sizeof(LOGO_PARAM)-sizeof(long));

	if (data != NULL)
	{
		memcpy(data, g_sys_param.logoData, g_sys_param.logo[nChannel].nDataLen);
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	//printf("getLogoParam(): OK!\n");
	return 0;
}

int setLogoParam(int nChannel, LOGO_PARAM *param, char *data)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
		
	if (param->nDataLen > MAX_LOGO_DATA)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.logo[nChannel], param, sizeof(LOGO_PARAM));
	//memcpy(&g_sys_param.logo[nChannel], param, sizeof(LOGO_PARAM)-sizeof(long));
	//g_sys_param.logo[nChannel].nDataLen = 0;
	if (data != NULL)
	{
		memcpy(g_sys_param.logoData, data, g_sys_param.logo[nChannel].nDataLen);
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);

	//printf("setLogoParam(): OK!\n");

	return 0;
}

int getMaskParam(int nChannel, MASK_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.mask[nChannel], sizeof(MASK_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setMaskParam(int nChannel, MASK_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.mask[nChannel], param, sizeof(MASK_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getMosaicParam(int nChannel, VIDEO_MASK *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.mosaic[nChannel], sizeof(VIDEO_MASK));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setMosaicParam(int nChannel, VIDEO_MASK *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.mosaic[nChannel], param, sizeof(VIDEO_MASK));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int getVideoMotionParam(int nChannel, VIDEO_MOTION_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.motionDetect[nChannel], sizeof(VIDEO_MOTION_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);

	
	return 0;
}

int setVideoMotionParam(int nChannel, VIDEO_MOTION_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.motionDetect[nChannel], param, sizeof(VIDEO_MOTION_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getAudioInAttrParam(int nChannel, AUDIO_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.audioInAttr[nChannel], sizeof(AUDIO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setAudioInAttrParam(int nChannel, AUDIO_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.audioInAttr[nChannel], param, sizeof(AUDIO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getAudioOutAttrParam(int nChannel, AUDIO_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.audioOutAttr[nChannel], sizeof(AUDIO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setAudioOutAttrParam(int nChannel, AUDIO_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.audioOutAttr[nChannel], param, sizeof(AUDIO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getAudioEncParam(int nChannel, AENC_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.audioEnc[nChannel], sizeof(AENC_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setAudioEncParam(int nChannel, AENC_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.audioEnc[nChannel], param, sizeof(AENC_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoLostAlarmParam(int nChannel, VIDEO_LOST_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoLostAlarm[nChannel], sizeof(VIDEO_LOST_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoLostAlarmParam(int nChannel, VIDEO_LOST_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoLostAlarm[nChannel], param, sizeof(VIDEO_LOST_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoMotionAlarmParam(int nChannel, VIDEO_MOTION_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoMotionAlarm[nChannel], sizeof(VIDEO_MOTION_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoMotionAlarmParam(int nChannel, VIDEO_MOTION_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoMotionAlarm[nChannel], param, sizeof(VIDEO_MOTION_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getProbeInAlarmParam(int nChannel, PROBE_IN_ALARM_PARAM *param)
{
#if 0
	if (nChannel<=0 || nChannel>=MAX_PROBE_IN)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
#endif

	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.probeInAlarm[nChannel], sizeof(PROBE_IN_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setProbeInAlarmParam(int nChannel, PROBE_IN_ALARM_PARAM *param)
{
#if 0
	if (nChannel<=0 || nChannel>=MAX_PROBE_IN)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
#endif
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.probeInAlarm[nChannel], param, sizeof(PROBE_IN_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getProbeOutAlarmParam(int nChannel, PROBE_OUT_ALARM_PARAM *param)
{
#if 0
	if (nChannel<0 || nChannel>=MAX_PROBE_OUT)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
#endif	
	
	pthread_mutex_lock(&g_sys_param_mutex);

	
	memcpy(param, &g_sys_param.probeOutAlarm[nChannel], sizeof(PROBE_OUT_ALARM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setProbeOutAlarmParam(int nChannel, PROBE_OUT_ALARM_PARAM *param)
{
	if (nChannel<0 || nChannel>=MAX_PROBE_OUT)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.probeOutAlarm[nChannel], param, sizeof(PROBE_OUT_ALARM_PARAM));
	

	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int setIrProbeAlarmStartParam(void)
{
	pthread_mutex_lock(&g_sys_param_mutex);
	g_sys_param.irProbeAlarmFlags = 1;
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setIrProbeAlarmStopParam(void)
{
	pthread_mutex_lock(&g_sys_param_mutex);
	g_sys_param.irProbeAlarmFlags = 0;
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getIrProberAlarmParam(void)
{
	return g_sys_param.irProbeAlarmFlags;

}

// add code by lvjh 2012
int getRtspParam(DVSNET_RTSP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.rtsp, sizeof(DVSNET_RTSP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRtspParam(DVSNET_RTSP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.rtsp, param, sizeof(DVSNET_RTSP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int getFtpParam(FTP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.ftp, sizeof(FTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setFtpParam(FTP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.ftp, param, sizeof(FTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getTftpParam(TFTP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.tftp, sizeof(TFTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setTftpParam(TFTP_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.tftp, param, sizeof(TFTP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getEmailParam(EMAIL_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.email, sizeof(EMAIL_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setEmailParam(EMAIL_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.email, param, sizeof(EMAIL_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getJpegIPParam(char *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.JpegStoreIP, MAX_IP_LEN);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setJpegIPParam(char *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.JpegStoreIP, param, MAX_IP_LEN);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int generateMultiAddr(char *pMultiAddr)
{
	unsigned char ipSegment1 = 224;
	unsigned char ipSegment2 = 0;
	unsigned char ipSegment3 = 0;
	unsigned char ipSegment4 = 0;
                                                                                                                                               
	srand((unsigned)time(NULL));
                                                                                                                                               
	if (pMultiAddr)
	{
		ipSegment1 += (unsigned char)(random()%15);
		ipSegment2 += (unsigned char)(random()%255);
		ipSegment3 += (unsigned char)(random()%255);
		ipSegment4 += (unsigned char)(random()%255);
                                                                                                                                               
		sprintf(pMultiAddr, "%d.%d.%d.%d", ipSegment1, ipSegment2, ipSegment3, ipSegment4);
		return 0;
	}
	else
	{
		return -1;
	}
}

int getNetParam(NET_PARAM *param)
{
	int ret = -1;
	char mac[24];

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.network, sizeof(NET_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setNetParam(NET_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.network, param, sizeof(NET_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getPPPOEParam(PPPOE_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.pppoe, sizeof(PPPOE_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setPPPOEParam(PPPOE_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.pppoe, param, sizeof(PPPOE_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getDDNSParam(DDNS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.ddns, sizeof(DDNS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setDDNSParam(DDNS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.ddns, param, sizeof(DDNS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	

	return 0;
}


int getLastRebootTimeParam(time_t *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.lastRebootTime, sizeof(time_t));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setLastRebootTimeParam(time_t *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.lastRebootTime, param, sizeof(time_t));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
		

	return 0;
}



int getTimeRebootParam(TIME_REBOOT_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.timerRebootParam, sizeof(TIME_REBOOT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setTimeRebootParam(TIME_REBOOT_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.timerRebootParam, param, sizeof(TIME_REBOOT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	

	return 0;
}


int getAlarmStatusParam(ALARM_STATUS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.alarm_status, sizeof(ALARM_STATUS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setAlarmStatusParam(ALARM_STATUS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.alarm_status, param, sizeof(ALARM_STATUS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	

	return 0;
}


int getYiyuanAlarmDDNSParam(YIYUAN_ALARM_DDNS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.yiyuanAlarmDdnsParam, sizeof(YIYUAN_ALARM_DDNS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setYiyuanAlarmDDNSParam(YIYUAN_ALARM_DDNS_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.yiyuanAlarmDdnsParam, param, sizeof(YIYUAN_ALARM_DDNS_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	

	return 0;
}

int getUpnpParam(UPNP_PORTMAPPING_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.upnp, sizeof(UPNP_PORTMAPPING_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setNewUpnpParam(DVSNET_UPNP_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.newUpnpInfo, param, sizeof(DVSNET_UPNP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getNewUpnpParam(DVSNET_UPNP_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.newUpnpInfo, sizeof(DVSNET_UPNP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setUpnpParam(UPNP_PORTMAPPING_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.upnp, param, sizeof(UPNP_PORTMAPPING_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}



int setUpnpIP_PORT_Param(UPNP_PORT_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.upnpInfo, sizeof(USER_INFO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getUpnpIP_PORT_Param(UPNP_PORT_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.upnpInfo, param, sizeof(USER_INFO_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}



int setP2PParam(DVSNET_P2P_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	memcpy(&g_sys_param.p2p, param, sizeof(DVSNET_P2P_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setP2PRegisterParam(DVSNET_REGISTER_INFO *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	memcpy(&g_sys_param.p2p_register, param, sizeof(DVSNET_REGISTER_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int setRemoteP2PConnectParam(DVSNET_REQCONBACK_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.remoteP2PConnectParam, param, sizeof(DVSNET_REQCONBACK_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getRemoteP2PConnectParam(DVSNET_REQCONBACK_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.remoteP2PConnectParam, sizeof(DVSNET_REQCONBACK_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setP2PBACKParam(DVSNET_ENDCONBACK_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.P2P_endconback, param, sizeof(DVSNET_ENDCONBACK_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int getP2PParam(DVSNET_P2P_PARAM *param)
{	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	//printf("g_sys_param.p2p = %d\n", g_sys_param.p2p.nPort);
	memcpy(param, &g_sys_param.p2p, sizeof(DVSNET_P2P_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int getWifiParam(WIFI_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.wlan, sizeof(WIFI_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


#if 1

List* CreateList()
{
	List *head;
	head = (List *)malloc(sizeof(List));
	head->next = NULL;
	return head;
}
List* Insert(List *head,DVSNET_WIFI_DEVICE *wifi)
{
	List *p,*q,*pr;
	p=head->next;
	pr=head;
	q = (List *)malloc(sizeof(List));
	memcpy(&(q->wifi_param),wifi,sizeof(DVSNET_WIFI_DEVICE));
	q->next = NULL;
	if (p == NULL)
	{
		head->next=q;
		return head;
	}
	else
	{
		while(p!=NULL)
		{
			if (p->wifi_param.nSignal<q->wifi_param.nSignal)
			{
				pr->next=q;
				q->next=p;
				break;
			}else
			{
				pr=p;
				p=p->next;
			}	
		}	
		if (p==NULL)
		{
			pr->next=q;
		}
	}
	return head;
}

void ListPrintf(List* head)
{
	List *p=head->next;
	while (p != NULL)
	{
		printf("%ld\t",p->wifi_param.nSignal);
		printf("SSID :%s\n",p->wifi_param.sSSID);
		p = p->next;
	}

}

int List_To_Array(List *head,struct SDK_NetWifiDeviceAll  *wifi_buffer)
{
	int i=0;
	List *p=head->next;
	while(p != NULL)
	{
		memcpy(&(wifi_buffer->vNetWifiDeviceAll[i++]),&(p->wifi_param),sizeof(DVSNET_WIFI_DEVICE));
		wifi_buffer->nDevNumber=i;
		p = p->next;
	}
	return 0;
}
void delete_All(List* head)
{
	List* p;
	while((p=head)!=NULL)
	{
		head=p->next;
		free(p);
		p=NULL;
	}
}
int GetEncryption(char *security)
{
	char ptr[24];
	bzero(ptr,24);
	strcpy(ptr,security);
	
	if(!strcmp(ptr,"WPAPSK/TKIP")||!strcmp(ptr,"WPAPSK/AES"))
	{
		return 2;
	}
	else if(!strcmp(ptr,"WPAPSKWPA2PSK/TKIPAES")||!strcmp(ptr,"WPAPSKWPA2PSK/AES")||!strcmp(ptr,"WPA2PSK/TKIPAES")||!strcmp(ptr,"WPA2PSK/AES"))
	{
		return  3;
	}
	else if(!strcmp(ptr,"WEP"))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


int GetAuthMode(char *security)
{
	char ptr[24];
	bzero(ptr,24);
	strcpy(ptr,security);
	
	 if(!strcmp(ptr,"WPAPSK/AES")||!strcmp(ptr,"WPAPSKWPA2PSK/AES")||!strcmp(ptr,"WPA2PSK/AES"))
	{
		return 4;
	}
	else if(!strcmp(ptr,"WPAPSKWPA2PSK/TKIPAES")||!strcmp(ptr,"WPAPSK/TKIP")||!strcmp(ptr,"WPA2PSK/TKIPAES"))
	{
		return	3;
	}
	else
	{
		return 0;
	}
	
}


int GetWifiMode(char *mode)
{
	char ptr[8];
	bzero(ptr,8);
	strcpy(ptr,mode);
	if(!strcmp(ptr,"11b/g"))
	{
		return 1;
	}
	else if(!strcmp(ptr,"11b/g/n"))
	{
		return 0;
	}
	else 
	{
		return -1;
	}
}
int GetNetType(char *type)
{
	char ptr[8];
	bzero(ptr,8);
	strcpy(ptr,type);
	if(!strcmp(ptr,"In"))
	{
		return 0;
	}
	else 
	{
		return -1;
	}
}

void ltrim(char *dstr,char *sstr)
{
        int i;
        char tmp[4096];
        strcpy(dstr,sstr);
        for (i=0;dstr[i]==' ';i++);
        strcpy(tmp,dstr+i);
        strcpy(dstr,tmp);
}
void rtrim(char *dstr,char *sstr)
{
        int i;
        strcpy(dstr,sstr);
        for (i=strlen(dstr)-1;dstr[i]==' ';i--)
                dstr[i]=0;
}
void alltrim(char *dstr,char *sstr)
{
        int i;
        strcpy(dstr,sstr);
        rtrim(dstr,dstr);
        ltrim(dstr,dstr);
}


void Wifi_Param_Struct(struct SDK_NetWifiDeviceAll *wifi_buffer)
{
	FILE *fp;
	WIFIINFO wifi_info;
	WIFIINFO bssid_wifiinfo;
	
	DVSNET_WIFI_DEVICE wifi_param;
	struct SDK_NetWifiDeviceAll  wifi_sbuffer;
	int ret = -1;
	int get_number = 0;
	int i;
	int countd=0,counta=0;
	char buffer_cache[2048];
	char buffer[128];
	List *head;
	
	memset(buffer_cache,0,2048);
	memset(buffer,0,128);
	memset(&wifi_info,0,sizeof(WIFIINFO));
	memset(&wifi_sbuffer,0,sizeof(struct SDK_NetWifiDeviceAll));
	memset(&wifi_param,0,sizeof(DVSNET_WIFI_DEVICE));
	
	head = CreateList();
	fp = fopen(WIFI_CONF_PATH,"r");
	if(fp == NULL)
	{
		printf("open %s file fail\n",WIFI_CONF_PATH);
		return ;
	}
	ret = fread(buffer_cache,1,2048,fp);
	for(i=0;i<ret;i++)
	{
		if(buffer_cache[i]== 0x0a)
		{
			counta++;
			if(counta > 1 && (ret-i) > 2)
			{
				//printf("%s",&buffer_cache[i+1]);
				memset(buffer,0,128);
				memset(&wifi_info,0,sizeof(WIFIINFO));
				memset(&wifi_param,0,sizeof(DVSNET_WIFI_DEVICE));
				memcpy(buffer,&buffer_cache[i+1],99);

				get_number = sscanf(buffer,"%[^ ]%[^:]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%s",\
							wifi_info.ch,wifi_info.ssid,wifi_info.bssid,wifi_info.security,wifi_info.signal,wifi_info.w_mode,wifi_info.nt);

				if(get_number != 7)  
				{
					printf("i =%d\n",i);
					break;
				}
#if 0
			    printf("%s  %s  %s  %s  %s  %s  %s\n",wifi_info.ch,wifi_info.ssid,\
					                  wifi_info.bssid,wifi_info.security,wifi_info.signal,wifi_info.w_mode,wifi_info.nt);
#endif
			    memcpy(wifi_param.sSSID,wifi_info.ssid, 24);
				memset(wifi_info.ssid, 0, 24);
				alltrim(wifi_info.ssid,wifi_param.sSSID);
				memset(wifi_param.sSSID, 0, 36);
     			strcpy(wifi_param.sSSID,wifi_info.ssid);
				wifi_param.nChannel = atoi(wifi_info.ch);
				wifi_param.nEncryptType = GetEncryption(wifi_info.security);
				wifi_param.nAuthMode = GetAuthMode(wifi_info.security);;
				wifi_param.nSignal = atoi(wifi_info.signal);
				wifi_param.nNetType = GetNetType(wifi_info.nt);
				#if 0
				get_number = sscanf(buffer,"%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%[^ ]%*[ ]%s",\
						wifi_info.ch,wifi_info.ssid,wifi_info.bssid,wifi_info.security,wifi_info.signal,wifi_info.w_mode,wifi_info.nt);
				strcpy(wifi_param.bSSID, wifi_info.bssid);
				printf("######wifi_info.bssid = %s\n", wifi_info.bssid);
				printf("######wifi_param.bSSID = %s\n", wifi_param.bSSID);
				#endif
				
				Insert(head,&wifi_param);
				
			}
		}
	}
	fclose(fp);
     //ListPrintf(head);
#if 1	
	List_To_Array(head,&wifi_sbuffer);
	

//	for(i=0;i<wifi_sbuffer.nDevNumber;i++)
//	{
//		printf("wifi:%d\tsingal :%ld\tssid:%s\n",i,wifi_sbuffer.vNetWifiDeviceAll[i].nSignal,wifi_sbuffer.vNetWifiDeviceAll[i].sSSID);
//	}
#endif
	//printf("counter :%d\n",wifi_buffer.nDevNumber);
	delete_All(head);
	memcpy(wifi_buffer,&wifi_sbuffer,sizeof(struct SDK_NetWifiDeviceAll));
	return;
}

#endif

int setWifiParam(WIFI_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.wlan, param, sizeof(WIFI_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}


int getRemoteConnectParam(REMOTE_CONNECT_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.remoteConnectParam, sizeof(REMOTE_CONNECT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRemoteConnectParam(REMOTE_CONNECT_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.remoteConnectParam, param, sizeof(REMOTE_CONNECT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getPtzParam(int nChannel, PTZ_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.ptz[nChannel], sizeof(PTZ_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;	
}

int setPtzParam(int nChannel, PTZ_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.ptz[nChannel], param, sizeof(PTZ_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;	
}

int getRs485Param(int nChannel, COM_PARAM *param)
{	
	if (nChannel<0 || nChannel>=MAX_COM_NUM)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.rs485[nChannel], sizeof(COM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRs485Param(int nChannel, COM_PARAM *param)
{	
	if (nChannel<0 || nChannel>=MAX_COM_NUM)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.rs485[nChannel], param, sizeof(COM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getRs232Param(int nChannel, COM_PARAM *param)
{
	/*
	if (nChannel<0 || nChannel>=MAX_COM_NUM)
	{
		return -1;
	}
	*/
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.rs232, sizeof(COM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRs232Param(int nChannel, COM_PARAM *param)
{
	/*
	if (nChannel<0 || nChannel>=MAX_COM_NUM)
	{
		return -1;
	}
	*/
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.rs232, param, sizeof(COM_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getRemoteTalkIPParam(char *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.remoteTalkIP, 16);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRemoteTalkIPParam(char *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.remoteTalkIP, param, 16);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setTimeParam(DATE_PARAM param)
{	
	int ret = -1;
	
	ret = setSystemTime(param.year, param.month, param.day, param.hour, param.minute, param.second);
	if (ret < 0)
	{
		return -1;
	}	
	
	return 0;
}

int getVideoFlipParam(int nChannel, VIDEO_FLIP_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoFlip[nChannel], sizeof(VIDEO_FLIP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoFlipParam(int nChannel, VIDEO_FLIP_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoFlip[nChannel], param, sizeof(VIDEO_FLIP_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoMirrorParam(int nChannel, VIDEO_MIRROR_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoMirror[nChannel], sizeof(VIDEO_MIRROR_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoMirrorParam(int nChannel, VIDEO_MIRROR_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoMirror[nChannel], param, sizeof(VIDEO_MIRROR_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoHzParam(int nChannel, VIDEO_HZ_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.videoHz[nChannel], sizeof(VIDEO_HZ_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setVideoHzParam(int nChannel, VIDEO_HZ_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.videoHz[nChannel], param, sizeof(VIDEO_HZ_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getAudioInPathParam(int nChannel, AUDIO_PATH_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.audioInPath[nChannel], sizeof(AUDIO_PATH_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setAudioInPathParam(int nChannel, AUDIO_PATH_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}

	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.audioInPath[nChannel], param, sizeof(AUDIO_PATH_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int get3gParam(G3_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.g3, sizeof(G3_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int set3gParam(G3_PARAM *param)
{
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.g3, param, sizeof(G3_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getScheduleSnapshotParam(int nChannel, SCHEDULE_SNAPSHOT_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.snapshot[nChannel], sizeof(SCHEDULE_SNAPSHOT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setScheduleSnapshotParam(int nChannel, SCHEDULE_SNAPSHOT_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.snapshot[nChannel], param, sizeof(SCHEDULE_SNAPSHOT_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getPtzAutoCtrlParam(int nChannel, PTZ_AUTO_CTRL *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.ptzAutoCtrl[nChannel], sizeof(PTZ_AUTO_CTRL));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setPtzAutoCtrlParam(int nChannel, PTZ_AUTO_CTRL *param)
{
		if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	
	if (param == NULL)
	{
		return -1;
	}

	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.ptzAutoCtrl[nChannel], param, sizeof(PTZ_AUTO_CTRL));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

// save param to flash
int saveParamToFlash()
{
	int ret = -1;
	int fd = -1;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = write(fd, &g_sys_param, sizeof(SYS_PARAM));
	if (ret != sizeof(SYS_PARAM))
	{
		close(fd);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}	
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	close(fd);
	
	return 0;
}

int getParamFromFlash()
{
	int ret = -1;
	int fd = -1;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = read(fd, &g_sys_param, sizeof(SYS_PARAM));
	if (ret != sizeof(SYS_PARAM))
	{
		close(fd);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}
	close(fd);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;	
}

int getParamStatusFromFlash()
{
	int ret = -1;
	int fd = -1;
	int status = 0;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}	

	ret = lseek(fd, 0x0, SEEK_SET); // offset:?
	if (ret == -1)
	{
		close(fd);
		return -1;
	}
		
	ret = read(fd, &status, sizeof(int));
	if (ret != sizeof(int))
	{
		close(fd);		
		return -1;
	}
	close(fd);		
	
	return status;	
}

int setParamStatusToFlash(int status)
{
	int ret = -1;
	int fd = -1;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}	
	
	ret = lseek(fd, 0x0, SEEK_SET); // offset:?
	if (ret == -1)
	{
		close(fd);
		return -1;
	}
	
	ret = write(fd, &status, sizeof(int));
	if (ret != sizeof(int))
	{
		close(fd);		
		return -1;
	}
	close(fd);		
	
	return 0;
}






int getBootStausFromFlash()
{
	int ret = -1;
	int fd = -1;
	int status = 0;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}
	
	ret = lseek(fd, 0x0, SEEK_SET); // offset:?
	if (ret == -1)
	{
		close(fd);
		return -1;
	}	
	
	ret = read(fd, &status, sizeof(int));
	if (ret != sizeof(int))
	{
		close(fd);		
		return -1;
	}
	close(fd);		
	
	return status;	
}

int setBootStatusToFlash(int status)
{
	int ret = -1;
	int fd = -1;
	
	fd = open("/dev/flash", O_RDWR);
	if (fd < 0)
	{
		return -1;
	}
	
	ret = lseek(fd, 0x0, SEEK_SET); // offset:?
	if (ret == -1)
	{
		close(fd);
		return -1;
	}
	
	ret = write(fd, &status, sizeof(int));
	if (ret != sizeof(int))
	{
		close(fd);		
		return -1;
	}

	close(fd);		
	
	return 0;
}

// save param to file
int saveParamToFile()
{
	int ret = -1;
	FILE *fp = NULL;

	fp = fopen(PARAM_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", PARAM_CONFIGURE_FILE);

		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = fwrite(&g_sys_param, 1, sizeof(SYS_PARAM), fp);
	if (ret != sizeof(SYS_PARAM))
	{
		fclose(fp);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}
	fflush(fp);
	
	setParamStatusToFile(0);
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	fclose(fp);
	
	return 0;
}

int getDevParamFile(SYS_PARAM *param)
{
	int ret = -1;
	FILE *fp = NULL;
	char mac[24];

	fp = fopen(PARAM_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", PARAM_CONFIGURE_FILE);
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = fread(param, 1, sizeof(SYS_PARAM), fp);
	if (ret != sizeof(SYS_PARAM))
	{
		fclose(fp);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}

	pthread_mutex_unlock(&g_sys_param_mutex);
	
	fclose(fp);
	
	return 0;	
}

int saveDevParamFile(SYS_PARAM *param)
{
	int ret = -1;
	FILE *fp = NULL;
	
	if (param == NULL)
	{
		return -1;
	}

	fp = fopen(PARAM_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", PARAM_CONFIGURE_FILE);

		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = fwrite(param, 1, sizeof(SYS_PARAM), fp);
	if (ret != sizeof(SYS_PARAM))
	{
		fclose(fp);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	fflush(fp);
	fclose(fp);
	
	return 0;
}

int getParamFromFile()
{
	int ret = -1;
	FILE *fp = NULL;
	char mac[24];

	fp = fopen(PARAM_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", PARAM_CONFIGURE_FILE);
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	ret = fread(&g_sys_param, 1, sizeof(SYS_PARAM), fp);
	if (ret != sizeof(SYS_PARAM))
	{
		fclose(fp);
		pthread_mutex_unlock(&g_sys_param_mutex);
		return -1;
	}

	
	getSysInfo(&g_sys_param.sysInfo);
	//add code by lvjh 20120818
	setUSERConfigure(&g_sys_param.userInfo);
	
	// Add the code by lvjh, 2009-09-08
	memset(mac, 0, 24);
	ret = getMACConfigure(mac);
	if (ret == 0)
	{
		memcpy(g_sys_param.network.strPhyAddr, mac, 6);
	}

	pthread_mutex_unlock(&g_sys_param_mutex);
	
	fclose(fp);
	
	return 0;	
}

int getParamStatusFromFile()
{
	int ret = -1;
	FILE *fp = NULL;
	int status = 0;

	fp = fopen(PARAM_STATUS_FILE, "r+b");
	if (fp == NULL)
	{
		return -1;
	}

	ret = fseek(fp, 0x4, SEEK_SET);
	if (ret == -1)
	{
		fclose(fp);
		return -1;
	}
			
	ret = fread(&status, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);
		return -1;
	}
	fclose(fp);		
	
	return status;	
}

int setParamStatusToFile(int status)
{
	int ret = -1;
	FILE *fp = NULL;

	fp = fopen(PARAM_STATUS_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}	
	
	ret = fseek(fp, 0x4, SEEK_SET);
	if (ret == -1)
	{
		fclose(fp);
		return -1;
	}
	
	ret = fwrite(&status, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);		
		return -1;
	}
	fflush(fp);
	fclose(fp);		
	
	return 0;
}

int getBootStausFromFile()
{
	int ret = -1;
	FILE *fp = NULL;
	int status = 0;

	fp = fopen(PARAM_STATUS_FILE, "r+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fseek(fp, 0x0, SEEK_SET); 
	if (ret == -1)
	{
		fclose(fp);
		return -1;
	}	
	
	ret = fread(&status, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);		
		return -1;
	}
	fclose(fp);		
	
	return status;	
}

int setBootStatusToFile(int status)
{
	int ret = -1;
	FILE *fp = NULL;

	fp = fopen(PARAM_STATUS_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fseek(fp, 0x0, SEEK_SET);
	if (ret == -1)
	{
		fclose(fp);
		return -1;
	}
	
	ret = fwrite(&status, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}

// Add the code by lvjh, 2008-04-07
int setNetworkConfigure(int param)
{
	int ret = -1;
	FILE *fp = NULL;

	if (param<=0 || param>7)
	{
		param = 1;
	}

	fp = fopen(NETWORK_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(&param, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);		
		return -1;
	}
	fflush(fp);
	fclose(fp);		
	
	return 0;
}

int getNetworkConfigure()
{
	int ret = -1;
	FILE *fp = NULL;
	int param = 0;

	fp = fopen(NETWORK_CONFIGURE_FILE, "r+b");
	if (fp == NULL)
	{
		return 2;
	}
	
	ret = fread(&param, 1, sizeof(int), fp);
	if (ret != sizeof(int))
	{
		fclose(fp);		
		return 2;
	}
	fflush(fp);
	fclose(fp);
	
	if (param<=0 || param>7)
	{
		param = 1;
	}
	
	return param;	
}

int setDebugConfigure(int param)
{
	return 0;
}

int getDebugConfigure()
{
	return 0;
}

int setIDConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = 0;
	char pID[48];

	if (param == NULL)
	{
		return -1;
	}

	memset(pID, 0, 48);

	nLen = strlen(param);
	if (nLen <= 0)
	{
		return -1;
	}
	if (nLen >= 48)
	{
		memcpy(pID, param, 48);
		pID[47] = '\0';
	}
	else
	{
		strcpy(pID, param);
	}

	fp = fopen(ID_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(pID, 1, 48, fp);
	if (ret != 48)
	{
		fclose(fp);		
		return -1;
	}
	fflush(fp);
	fclose(fp);		
	
	return 0;
}

int getIDConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = 0;
	char pID[48];

	if (param == NULL)
	{
		return -1;
	}

	memset(pID, 0, 48);

	fp = fopen(ID_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fread(pID, 1, 48, fp);
	if (ret <= 0)
	{
		fclose(fp);		
		return -1;
	}
	fclose(fp);	
	pID[strlen(pID)] = '\0';

	nLen = strlen(pID);
	if (nLen <= 0)
	{
		return -1;
	}
	if (nLen >= 48)
	{
		memcpy(param, pID, 48);
		pID[47] = '\0';
	}
	else
	{
		strcpy(param, pID);
	}
	
	//printf("ID: %s\n", pID);

	return 0;
}

int setMACConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = 0;
	char pMAC[6];

	if (param == NULL)
	{
		return -1;
	}

	memset(pMAC, 0, 6);
	memcpy(pMAC, param, 6);

	fp = fopen(MAC_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(pMAC, 1, 6, fp);
	if (ret != 6)
	{
		fclose(fp);		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}

int getMACConfigure(char *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = 0;
	char pMAC[6];

	if (param == NULL)
	{
		return -1;
	}

	memset(pMAC, 0, 6);

	fp = fopen(MAC_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: MAC_CONFIGURE_FILE!\n");
		return -1;
	}
	
	ret = fread(pMAC, 1, 6, fp);
	if (ret != 6)
	{
		printf("Fread: %d\n", ret);
		fclose(fp);		
		return -1;
	}
	fclose(fp);	

	memcpy(param, pMAC, 6);

	//printf("getMACConfigure: %02x:%02x:%02x:%02x:%02x:%02x\n", pMAC[0], pMAC[1], pMAC[2], pMAC[3], pMAC[4], pMAC[5]);
	
	return 0;
}


int setUSERConfigure(USER_INFO_PARAM *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(USER_INFO_PARAM);
	char pUSER_INFO[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pUSER_INFO, 0, nLen);
	memcpy(pUSER_INFO, param, sizeof(USER_INFO_PARAM));

	fp = fopen(USER_INFO_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(pUSER_INFO, 1, sizeof(USER_INFO_PARAM), fp);
	if (ret != sizeof(USER_INFO_PARAM))
	{
		fclose(fp);		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}


int get_ip_wifi_configure(UPNP_WIFI_IP_PORT_INFO *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(UPNP_WIFI_IP_PORT_INFO);
	char pTmp[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pTmp, 0, nLen);

	fp = fopen(IP_WIFI_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: MAC_CONFIGURE_FILE!\n");
		return -1;
	}
	
	ret = fread(pTmp, 1, nLen, fp);
	if (ret != nLen)
	{
		printf("Fread: %d\n", ret);
		memcpy(param, pTmp, nLen);
		fclose(fp);		
		return -1;
	}
	fclose(fp);	

	memcpy(param, pTmp, nLen);

	return 0;
}


int getRebootHourParamFile(void)
{
	int ret = -1;
	FILE *fp = NULL;
	char mac[24];
	int rebootHour;

	fp = fopen(PARAM_REBOOT_HOUR_FILE, "rb");
	if (fp == NULL)
	{
		
		printf("Can not open the file: %s.\n", PARAM_REBOOT_HOUR_FILE);
		return -1;
	}
	
	ret = fread(rebootHour, 1, sizeof(rebootHour), fp);
	if (ret != sizeof(rebootHour))
	{
		fclose(fp);
		return -1;
	}

	fclose(fp);
	
	return 0;	
}



int saveRebootHourParamFile(int *rebootHour)
{
	int ret = -1;
	FILE *fp = NULL;
	
	fp = fopen(PARAM_REBOOT_HOUR_FILE, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", PARAM_REBOOT_HOUR_FILE);

		return -1;
	}

	ret = fwrite(rebootHour, 1, sizeof(rebootHour), rebootHour);
	if (ret != sizeof(rebootHour))
	{
		fclose(fp);
		return -1;
	}
	fflush(fp);
	fclose(fp);
	
	return 0;
}


int save_ip_wifi_configure(UPNP_WIFI_IP_PORT_INFO *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(UPNP_WIFI_IP_PORT_INFO);
	char pUSER_INFO[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pUSER_INFO, 0, nLen);
	memcpy(pUSER_INFO, param, sizeof(UPNP_WIFI_IP_PORT_INFO));

	fp = fopen(IP_WIFI_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(pUSER_INFO, 1, sizeof(UPNP_WIFI_IP_PORT_INFO), fp);
	if (ret != sizeof(UPNP_WIFI_IP_PORT_INFO))
	{
		fclose(fp);		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}



int getUSERP2PPORTConfigure(UPNP_PORT_INFO *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(UPNP_PORT_INFO);
	char pMAC[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pMAC, 0, nLen);

	fp = fopen(UPNP_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: MAC_CONFIGURE_FILE!\n");
		return -1;
	}
	
	ret = fread(pMAC, 1, nLen, fp);
	if (ret != nLen)
	{
		printf("Fread: %d\n", ret);
		memcpy(param, pMAC, nLen);
		fclose(fp);		
		return -1;
	}
	fclose(fp);	

	memcpy(param, pMAC, nLen);

	return 0;
}

#if 0
int getUSERP2PPORTConfigure(UPNP_PORT_INFO *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(UPNP_PORT_INFO);
	char pUSER_INFO[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pUSER_INFO, 0, nLen);
	memcpy(pUSER_INFO, param, sizeof(UPNP_PORT_INFO));

	fp = fopen(UPNP_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fread(pUSER_INFO, 1, sizeof(UPNP_PORT_INFO), fp);
	if (ret != sizeof(UPNP_PORT_INFO))
	{
		fclose(fp);		
		return -1;
	}
	printf("fread retrurn = %d\n", pUSER_INFO);
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}
#endif

void writeConfigFile(char *filePath, int value, int defaultValue)
{
	char szConfig[7] = {0};
	int iPicSaveConfig;
	FILE *file;
	
	sprintf(szConfig, "%d", defaultValue);
	if(access(filePath, 0) == -1){
		file = fopen(filePath, "w+");
		if(file == NULL){
		perror("open config file \n");
		exit(EXIT_FAILURE);
		} 
		lseek(file, 0, SEEK_SET);
		if(fwrite(szConfig, 7, sizeof(char), file) < 0){
		perror("write config file \n");
		exit(EXIT_FAILURE);
		}
		fclose(file);
	}
	
	sprintf(szConfig, "%d", value);
	//memcpy(szConfig, value, 7);
	//memcpy(szConfig, param, 7);
	file = fopen(filePath, "w+");
	if(file == NULL){
		perror("open config file \n");
		exit(EXIT_FAILURE);
		} 
		lseek(file, 0, SEEK_SET);
		if(fwrite(szConfig, 7, sizeof(char), file) < 0){
		perror("write config file \n");
		exit(EXIT_FAILURE);
	}
	fclose(file);
	return;
}

int readConfigFile(char *filePath)
{
	char szConfig[7] = "1";
	int iPicSaveConfig;
	FILE *file;
 
	memset(szConfig, '\0', 5);
	if(access(filePath, 0) == -1){
		file = fopen(filePath, "w+");
		if(file == NULL){
		perror("open config file \n");
		exit(EXIT_FAILURE);
		} 
		lseek((int)file, 0, SEEK_SET);
		if(fwrite(szConfig, 7, sizeof(char), file) < 0){
		perror("read config file \n");
		exit(EXIT_FAILURE);
		}
		fclose(file);
	}
	
	file = fopen(filePath, "r+");
	if(file == NULL){
		perror("open config file \n");
		exit(EXIT_FAILURE);
	} 
	lseek((int)file, 0, SEEK_SET);
	if(fread(szConfig, 7, sizeof(char), file) < 0){
		perror("read config file \n");
		exit(EXIT_FAILURE);
	}

	iPicSaveConfig = atoi(szConfig);
	fclose(file);
	return iPicSaveConfig;
}


int setRtspPortConfigure(int param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = 0;
	char pMAC[6];

	if (param == NULL)
	{
		return -1;
	}

	memset(pMAC, 0, 6);
	memcpy(pMAC, param, 6);


	if(access(RTSPPORT_CONFIGURE_FILE, 0) == -1){
		fp = fopen(RTSPPORT_CONFIGURE_FILE, "w+");
		if(fp == NULL){
		perror("open config file \n");
		exit(EXIT_FAILURE);
		} 
	}
	ret = fwrite(pMAC, 1, 6, fp);
	if (ret != 6)
	{
		fclose(fp);		
		return -1;
	}
	
	fflush(fp);
	fclose(fp);		
	
	return 0;
}
int setP2PConfigure(P2PSERVER *P2P)
{
	int ret = -1;
	FILE *fp = NULL;

	if (P2P == NULL)
	{
		return -1;
	}

	fp = fopen(P2P_CONFIGURE_FILE, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	ret = fwrite(P2P, 1, sizeof(P2PSERVER), fp);
	if (ret != sizeof(P2PSERVER))
	{
		fclose(fp);		
		return -1;
	}
	fflush(fp);	
	fclose(fp);		
	
	return 0;
}

int getP2PConfigure(P2PSERVER *P2P)
{
	int ret = -1;
	FILE *fp = NULL;

	if (P2P == NULL)
	{
		return -1;
	}

	fp = fopen(P2P_CONFIGURE_FILE, "rb");
	if (fp == NULL)
	{
		printf("Can not open the file: P2P_CONFIGURE_FILE!\n");
		return -1;
	}
	
	ret = fread(P2P, 1, sizeof(P2PSERVER), fp);
	if (ret != sizeof(P2PSERVER))
	{
		memset(P2P, 0, sizeof(P2PSERVER));
		
		fclose(fp);		
		return -1;
	}
	fclose(fp);	
	
	return 0;
}

#ifdef RECORD

int getRecordSDKParam(RECORD_SETUP *setup)
{
	int i = 0;
	int j = 0;
	int k = 0;
	RECORD_CHANNEL_PARAM recordParam;

	if (setup == NULL)
	{
		return -1;
	}

	memset(setup, 0, sizeof(RECORD_SETUP));

	pthread_mutex_lock(&g_sys_param_mutex);
	
	for (i=0; i<MAX_CHANNEL; i++)
	{
		recordParam.nCoverMode = g_sys_param.recordParam[i].nCoverMode;
		recordParam.nAudioFlag = g_sys_param.recordParam[i].nAudioFlag;
		recordParam.nReserve = g_sys_param.recordParam[i].nReserve;
		recordParam.avFormat.nVideoEncType = ENCODE_VIDEO_HISI;


#ifdef CCD
		if (g_sys_param.videoStandard[i].nStandard == 0x00)	// PAL
		{
			switch (g_sys_param.videoEnc[i][0].nEncodeHeight)
			{
			case 576:
			case 480:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 576;
				break;
				
			case 288:
			case 240:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 288;
				break;
				
			case 144:
			case 120:
			case 128:
			case 112:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 144;
				break;
			}
		}
		else
		{
			switch (g_sys_param.videoEnc[i][0].nEncodeHeight)
			{
			case 576:
			case 480:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 480;
				break;
				
			case 288:
			case 240:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 240;
				break;
				
			case 144:
			case 120:
			case 128:
			case 112:
				g_sys_param.videoEnc[i][0].nEncodeHeight = 120;
				break;
			}
		}
		recordParam.avFormat.nImageHeight = g_sys_param.videoEnc[i][0].nEncodeHeight;
#else
		recordParam.avFormat.nImageHeight = g_sys_param.videoEnc[i][0].nEncodeHeight;
#endif		
		
		recordParam.avFormat.nImageHeight = g_sys_param.videoEnc[i][0].nEncodeHeight;
		recordParam.avFormat.nImageWidth= g_sys_param.videoEnc[i][0].nEncodeWidth;
#ifdef G726
		recordParam.avFormat.nAudioEncType= ENCODE_AUDIO_G726;
#else
		recordParam.avFormat.nAudioEncType= ENCODE_AUDIO_G711;
#endif


		recordParam.avFormat.nAudioChannels= g_sys_param.audioEnc[i].nChannels;
		recordParam.avFormat.nAudioSamples= AUDIOSAMPLES;
		recordParam.avFormat.nAudioBitRate= AUDIOSAMPLES;
	
		memcpy(&setup->record_param[i], &recordParam, sizeof(RECORD_CHANNEL_PARAM));
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{
		memcpy(&setup->timer_record_param[i], &g_sys_param.timerRecordParam[i], sizeof(TIMER_RECORD_CHANNEL_PARAM));
	}

	for (i=0; i<MAX_CHANNEL; i++)
	{
		for (j=0; j<8; j++)
		{
			setup->videomotion_record_param[i].day[j].nOnFlag = g_sys_param.videoMotionAlarm[i].day[j].nOnFlag;
			setup->videomotion_record_param[i].day[j].nTime = g_sys_param.videoMotionAlarm[i].nLinkRecordTime;
			setup->videomotion_record_param[i].day[j].nChnBits = g_sys_param.videoMotionAlarm[i].nLinkRecord;
			for (k=0; k<MAX_TIME_SEGMENT; k++)
			{
				setup->videomotion_record_param[i].day[j].time_segment[k].start_hour = g_sys_param.videoMotionAlarm[i].day[j].time_segment[k].start_hour;
				setup->videomotion_record_param[i].day[j].time_segment[k].end_hour = g_sys_param.videoMotionAlarm[i].day[j].time_segment[k].end_hour;
				setup->videomotion_record_param[i].day[j].time_segment[k].start_minute = g_sys_param.videoMotionAlarm[i].day[j].time_segment[k].start_minute;
				setup->videomotion_record_param[i].day[j].time_segment[k].end_minute = g_sys_param.videoMotionAlarm[i].day[j].time_segment[k].end_minute;
			}
		}
	}

	for (i=0; i<MAX_PROBE_IN; i++)
	{
		for (j=0; j<8; j++)
		{
			setup->detector_record_param[i].day[j].nOnFlag = g_sys_param.probeInAlarm[i].day[j].nOnFlag;
			setup->detector_record_param[i].day[j].nTime = g_sys_param.probeInAlarm[i].nLinkRecordTime;
			setup->detector_record_param[i].day[j].nChnBits = g_sys_param.probeInAlarm[i].nLinkRecord;
			for (k=0; k<MAX_TIME_SEGMENT; k++)
			{
				setup->detector_record_param[i].day[j].time_segment[k].start_hour = g_sys_param.probeInAlarm[i].day[j].time_segment[k].start_hour;
				setup->detector_record_param[i].day[j].time_segment[k].end_hour = g_sys_param.probeInAlarm[i].day[j].time_segment[k].end_hour;
				setup->detector_record_param[i].day[j].time_segment[k].start_minute = g_sys_param.probeInAlarm[i].day[j].time_segment[k].start_minute;
				setup->detector_record_param[i].day[j].time_segment[k].end_minute = g_sys_param.probeInAlarm[i].day[j].time_segment[k].end_minute;
			}
		}
	}

	pthread_mutex_unlock(&g_sys_param_mutex);
	/*
	printf("RECORD_SETUP: %d\n", sizeof(RECORD_SETUP));
	for (j=0; j<8; j++)
	printf("VideoMotion Record: %d %d %d %d %d %d %d %d %d\n", 
			setup->videomotion_record_param[0].day[j].nOnFlag,
			setup->videomotion_record_param[0].day[j].time_segment[0].start_hour,
			setup->videomotion_record_param[0].day[j].time_segment[0].start_minute,
			setup->videomotion_record_param[0].day[j].time_segment[0].end_hour,
			setup->videomotion_record_param[0].day[j].time_segment[0].end_minute,
			setup->videomotion_record_param[0].day[j].time_segment[1].start_hour,
			setup->videomotion_record_param[0].day[j].time_segment[1].start_minute,
			setup->videomotion_record_param[0].day[j].time_segment[1].end_hour,
			setup->videomotion_record_param[0].day[j].time_segment[1].end_minute);
	*/
	return 0;
}

int getRecordParam(int nChannel, RECORD_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.recordParam[nChannel], sizeof(RECORD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setRecordParam(int nChannel, RECORD_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.recordParam[nChannel], param, sizeof(RECORD_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getTimerRecordParam(int nChannel, TIMER_RECORD_CHANNEL_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.timerRecordParam[nChannel], sizeof(TIMER_RECORD_CHANNEL_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setTimerRecordParam(int nChannel, TIMER_RECORD_CHANNEL_PARAM *param)
{
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.timerRecordParam[nChannel], param, sizeof(TIMER_RECORD_CHANNEL_PARAM));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getVideoMotionRecordParam(int nChannel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param)
{
	int j = 0;
	int k = 0;
	
	if (nChannel<0 || nChannel>=get_channel_num())
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	for (j=0; j<8; j++)
	{
		param->day[j].nOnFlag = g_sys_param.videoMotionAlarm[nChannel].day[j].nOnFlag;
		param->day[j].nTime = g_sys_param.videoMotionAlarm[nChannel].nLinkRecordTime;
		param->day[j].nChnBits = g_sys_param.videoMotionAlarm[nChannel].nLinkRecord;
		for (k=0; k<MAX_TIME_SEGMENT; k++)
		{
			param->day[j].time_segment[k].start_hour = g_sys_param.videoMotionAlarm[nChannel].day[j].time_segment[k].start_hour;
			param->day[j].time_segment[k].end_hour = g_sys_param.videoMotionAlarm[nChannel].day[j].time_segment[k].end_hour;
			param->day[j].time_segment[k].start_minute = g_sys_param.videoMotionAlarm[nChannel].day[j].time_segment[k].start_minute;
			param->day[j].time_segment[k].end_minute = g_sys_param.videoMotionAlarm[nChannel].day[j].time_segment[k].end_minute;
		}
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getProbeRecordParam(int nChannel, DETECTOR_RECORD_CHANNEL_PARAM *param)
{
	int j = 0;
	int k = 0;

	if (nChannel<0 || nChannel>=MAX_PROBE_IN)
	{
		return -1;
		
	}
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	for (j=0; j<8; j++)
	{
		param->day[j].nOnFlag = g_sys_param.probeInAlarm[nChannel].day[j].nOnFlag;
		param->day[j].nTime = g_sys_param.probeInAlarm[nChannel].nLinkRecordTime;
		param->day[j].nChnBits = g_sys_param.probeInAlarm[nChannel].nLinkRecord;
		for (k=0; k<MAX_TIME_SEGMENT; k++)
		{
			param->day[j].time_segment[k].start_hour = g_sys_param.probeInAlarm[nChannel].day[j].time_segment[k].start_hour;
			param->day[j].time_segment[k].end_hour = g_sys_param.probeInAlarm[nChannel].day[j].time_segment[k].end_hour;
			param->day[j].time_segment[k].start_minute = g_sys_param.probeInAlarm[nChannel].day[j].time_segment[k].start_minute;
			param->day[j].time_segment[k].end_minute = g_sys_param.probeInAlarm[nChannel].day[j].time_segment[k].end_minute;
		}
	}
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

#endif

#ifdef MOBILE_PLATFORM
int setMpInfoParam(MP_REGISTER_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.mpInfo, param, sizeof(MP_REGISTER_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int getMpInfoParam(MP_REGISTER_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.mpInfo, sizeof(MP_REGISTER_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

#endif


#if 1
int getMultiDevParam(DVSNET_MULTIDEVALL_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(param, &g_sys_param.multiDevAll, sizeof(DVSNET_MULTIDEVALL_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}

int setMultiDevParam(DVSNET_MULTIDEVALL_INFO *param)
{
	if (param == NULL)
	{
		return -1;
	}
	
	pthread_mutex_lock(&g_sys_param_mutex);
	
	memcpy(&g_sys_param.multiDevAll, param, sizeof(DVSNET_MULTIDEVALL_INFO));
	
	pthread_mutex_unlock(&g_sys_param_mutex);
	
	return 0;
}



#endif



