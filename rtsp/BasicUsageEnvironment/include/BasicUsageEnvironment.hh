/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2011 Live Networks, Inc.  All rights reserved.
// Basic Usage Environment: for a simple, non-scripted, console application
// C++ header

#ifndef _BASIC_USAGE_ENVIRONMENT_HH
#define _BASIC_USAGE_ENVIRONMENT_HH

#ifndef _BASIC_USAGE_ENVIRONMENT0_HH
#include "BasicUsageEnvironment0.hh"
#endif
#include <stdio.h>

#define  CHN0_SOUR					"/mnt/mtd/dvs/mobile/tmpfs/ch0.264"
#define  CHN1_SOUR 					"/mnt/mtd/dvs/mobile/tmpfs/ch1.264"
#define  CHN2_SOUR 					"/mnt/mtd/dvs/mobile/tmpfs/ch2.264"

#define RTSPPORT_CONFIGURE_FILE		"/param/RTSPPort.conf"
#define USER_INFO_CONFIGURE_FILE	"/param/user_info.conf"


// IP 限制
typedef struct
{
	int nOnFlag;
	unsigned long nStartIP;
	unsigned long nStopIP;
	unsigned long nReserve;
}LOGON_IP_BIND_PARAM;

//用户信息
typedef struct
{
	char strName[32];			//用户名
	char strPsw[32];			//密码
	unsigned long nRight[4];		//128个功能权限

	LOGON_IP_BIND_PARAM ipBind;

}USER_INFO;						//最多10个用户

// 1152bytes
typedef struct
{
	USER_INFO Admin;
	USER_INFO Users[10];
	USER_INFO Guest;
	
}USER_INFO_PARAM;


int readConfigFile(char *filePath)
{
	char szConfig[7] = "554";
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

int getUSERConfigure(USER_INFO_PARAM *param)
{
	int ret = -1;
	FILE *fp = NULL;
	int nLen = sizeof(USER_INFO_PARAM);
	char pMAC[nLen];

	if (param == NULL)
	{
		return -1;
	}

	memset(pMAC, 0, nLen);

	fp = fopen(USER_INFO_CONFIGURE_FILE, "rb");
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


class BasicUsageEnvironment: public BasicUsageEnvironment0 {
public:
  static BasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

  // redefined virtual functions:
  virtual int getErrno() const;

  virtual UsageEnvironment& operator<<(char const* str);
  virtual UsageEnvironment& operator<<(int i);
  virtual UsageEnvironment& operator<<(unsigned u);
  virtual UsageEnvironment& operator<<(double d);
  virtual UsageEnvironment& operator<<(void* p);

protected:
  BasicUsageEnvironment(TaskScheduler& taskScheduler);
      // called only by "createNew()" (or subclass constructors)
  virtual ~BasicUsageEnvironment();
};


class BasicTaskScheduler: public BasicTaskScheduler0 {
public:
  static BasicTaskScheduler* createNew();
  virtual ~BasicTaskScheduler();

protected:
  BasicTaskScheduler();
      // called only by "createNew()"

protected:
  // Redefined virtual functions:
  virtual void SingleStep(unsigned maxDelayTime);

  virtual void setBackgroundHandling(int socketNum, int conditionSet, BackgroundHandlerProc* handlerProc, void* clientData);
  virtual void moveSocketHandling(int oldSocketNum, int newSocketNum);

protected:
  // To implement background operations:
  int fMaxNumSockets;
  fd_set fReadSet;
  fd_set fWriteSet;
  fd_set fExceptionSet;
};

#endif
