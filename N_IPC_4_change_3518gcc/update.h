#ifndef __UPDATE_H_
#define __UPDATE_H_

#define CRC_ERR 			0x1000
#define APP_UPDATE_START	0x1001
#define APP_UPDATE_STOP		0x1002
#define APP_UPDATE_ERR		0x1003

typedef struct
{
	char fileName[32];
	char filePath[128];
	
	unsigned long nFileVersion;
	unsigned long nFileLen;
	
	unsigned long nFileCrc;
	
	unsigned long nReserve;		// 1: KERNEL, 2: SYSTEM, 3: APP
	
}UPDATE_FILE_HEAD;

typedef struct
{
	char fileName[32];
	unsigned long nProgress;
	unsigned long nResult;

	unsigned long nReserve;
	
}UPDATE_RETURN;

int updateImageFile(int sockFd, int size, char *buffer, struct sockaddr_in addr, int nflag);

#endif
