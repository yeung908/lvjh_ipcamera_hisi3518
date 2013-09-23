#include <stdio.h>
#include <stdlib.h>

#include "com.h"
#include "comTransfer.h"

int g_rs232_com = -1;

int RS232_Open()
{
	int ret = -1;

	ret = COM_Open(0);
	if (ret < 0)
	{
		return -1;
	}

	g_rs232_com = ret;

	return 0;
}

int RS232_Close()
{
	int ret = -1;

	if (g_rs232_com < 0)
	{
		return -1;
	}

	ret = COM_Close(g_rs232_com);
	if (ret < 0)
	{
		return -1;
	}

	g_rs232_com = -1;

	return 0;
}

int RS232_Setup(unsigned long nBaudRate, unsigned long nDataBit, unsigned long nParity, unsigned long nStopBit)
{
	int ret = -1;
	COM_PARAM param;

	if (g_rs232_com < 0)
	{
		ret = COM_Open(0);
		if (ret < 0)
		{
			return -1;
		}
		g_rs232_com = ret;
	}

	param.nBaudRate = nBaudRate;
	param.nDataBits = nDataBit;
	param.nParity = nParity;
	param.nStopBits = nStopBit;
	param.nFlowCtrl = 0;
	
	ret = COM_Setup(g_rs232_com, param);
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int RS232_Send(char *buffer, int size)
{
	int ret = -1;

	if (buffer==NULL || size<=0)
	{
		return -1;
	}
	if (g_rs232_com < 0)
	{
		return -1;
	}

	ret = COM_Send(g_rs232_com, buffer, size);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

int RS232_Receive(char *buffer, int *size)
{
	int ret = -1;

	if (buffer==NULL || size==NULL)
	{
		return -1;
	}
	if (g_rs232_com < 0)
	{
		return -1;
	}

	ret = COM_Receive(g_rs232_com, buffer, size);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}
