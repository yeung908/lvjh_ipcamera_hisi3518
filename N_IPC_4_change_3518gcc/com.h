#ifndef _COM_H_
#define _COM_H_

#define MAX_COM_NUM 2

typedef struct
{
	int nBaudRate;
	int nDataBits;
	int nParity;
	int nStopBits;
	int nFlowCtrl;
	int nReserve;
}COM_PARAM, *pCOM_PARAM;

int COM_GetDevName(int devNo, char *devName);
int COM_Open(int devNo);
int COM_Close(int fd);
int COM_Setup(int fd, COM_PARAM param);
int COM_GetSetup(int fd,  COM_PARAM *param);
int COM_Send(int fd, char *buffer, int size);
int COM_Receive(int fd, char *buffer, int *size);

#endif
