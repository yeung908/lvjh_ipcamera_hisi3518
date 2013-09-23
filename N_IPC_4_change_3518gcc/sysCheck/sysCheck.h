#ifndef __SYS_CHECK_H_
#define __SYS_CHECK_H_

#define KERNEL_OK			0x11
#define KERNEL_FAIL			0x22
#define SYS_OK				0x33
#define SYS_FAIL			0x44
#define APP_OK				0x55
#define APP_FAIL			0x66

#define UPDATE_START		0xAA
#define UPDATE_OK			0xBB

#define UPDATE_MODE 		1
#define WATCH_MODE 			2

int SYSCHECK_Init();
int SYSCHECK_ReadFlash();
int SYSCHECK_WriteFlash();
int SYSCHECK_GetKernetStatus();
int SYSCHECK_SetKernetStatus(unsigned long nStatus);
int SYSCHECK_GetRamdiskStatus();
int SYSCHECK_SetRamdiskStatus(unsigned long nStatus);
int SYSCHECK_GetAppStatus();
int SYSCHECK_SetAppStatus(unsigned long nStatus);
int SYSCHECK_GetAppRunFlag();
int SYSCHECK_SetAppRunFlag(unsigned long nStatus);
int SYSCHECK_GetAppFailedCount();
int SYSCHECK_SetAppFailedCount(unsigned long nCount);
int SYSCHECK_GetAppUpdateMode();

#endif


