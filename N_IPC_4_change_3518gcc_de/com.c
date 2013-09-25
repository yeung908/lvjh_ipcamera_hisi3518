#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include "com.h"

static int convert_baudrate(unsigned int baudrate)
{
	switch (baudrate)
	{
	case 2400:
		return B2400;

	case 4800:
		return B4800;

	case 9600:
		return B9600;

	case 19200:
		return B19200;
		
	case 38400:
		return B38400;
		
	case 57600:
		return B57600;
		
	case 115200:
		return B115200;
	
	default:
		return B9600;
	}
}

int convert_baudrate2(unsigned int  baudrate)
{
   	int ibaudrate = 0;
   
   	switch(baudrate)
	{
	case 0:
		ibaudrate = 1200;
        break;

	case 1:
        ibaudrate = 2400;
        break;

	case 2:
        ibaudrate = 4800;
        break;

	case 3:
        ibaudrate = 9600;
        break;

	case 4:
        ibaudrate = 19200;
        break;

	case 5:
        ibaudrate = 38400;
        break;

	case 6:
        ibaudrate = 43000;
        break;

	case 7:
        ibaudrate = 56000;
        break;

	case 8:
        ibaudrate = 57600;
        break;

	case 9:
        ibaudrate = 115200;
        break;

	default:
        ibaudrate = 19200;
        break;
    }
  	
	return ibaudrate;
}

int COM_GetDevName(int devNo, char *devName)
{	
	if (devName == NULL)
	{
		return -1;
	}
	if (devNo<0 || devNo>MAX_COM_NUM)
	{
		return -1;
	}
	
	sprintf(devName, "/dev/ttyAMA%d", devNo);
	
	return 0;
}

int COM_Open(int devNo)
{
	int fd = -1;
	int ret = -1;
	char devName[32];
	
	ret = COM_GetDevName(devNo, devName);
	if (ret < 0)
	{
		return -1;
	}
	
	fd = open(devName, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
	{
		printf("COM_Open(%d %s): Failed!\n", devNo, devName);
		return -1;
	}
	
	return fd;
}

int COM_Close(int fd)
{
	if (fd < 0)
	{
		return -1;
	}
	
	close(fd);
	
	return 0;
}

int COM_Setup(int fd, COM_PARAM param)
{
	int ret = -1;
	struct termios termios_old;
	struct termios termios_new;
	int baudrate = 0;
	int fctl = 0;
	int databit = 0;
	int stopbit = 0;
	int parity = 0;
	
	if (fd < 0)
	{
		return -1;
	}
	
	bzero(&termios_old, sizeof(termios_old));
	bzero(&termios_new, sizeof(termios_new));
	
	cfmakeraw(&termios_new);
	tcgetattr(fd, &termios_old);
	
	// baudrates
	baudrate = convert_baudrate(param.nBaudRate);
	cfsetispeed(&termios_new, baudrate);		
	cfsetospeed(&termios_new, baudrate);		
	termios_new.c_cflag |= CLOCAL;			
	termios_new.c_cflag |= CREAD;
				
	fctl = param.nFlowCtrl;
	switch (fctl)
	{
	case '0':
		termios_new.c_cflag &= ~CRTSCTS;				// no flow control
		break;
		
	case '1':
		termios_new.c_cflag |= CRTSCTS;	    		// hardware flow control
		break;
		
	case '2':
		termios_new.c_iflag |= IXON | IXOFF |IXANY;  	//software flow control
		break;
	}

	// data bits
	termios_new.c_cflag &= ~CSIZE;		
	databit = param.nDataBits;
	switch (databit)
	{
	case '5':
		termios_new.c_cflag |= CS5;
		break;
		
	case '6':
		termios_new.c_cflag |= CS6;
		break;
		
	case '7':
		termios_new.c_cflag |= CS7;
		break;
		
	default:
		termios_new.c_cflag |= CS8;
		break;
	}

	// parity check
	parity = param.nParity;
	switch (parity)
	{
	case '0':
		termios_new.c_cflag &= ~PARENB;		// no parity check
		break;
		
	case '1':
		termios_new.c_cflag |= PARENB;			// odd check
		termios_new.c_cflag &= ~PARODD;
		break;
		
	case '2':
		termios_new.c_cflag |= PARENB;			// even check
		termios_new.c_cflag |= PARODD;
		break;
	}

	// stop bits
	stopbit = param.nStopBits;
	if (stopbit == '2')
	{
		termios_new.c_cflag |= CSTOPB;			// 2 stop bits
	}
	else
	{
		termios_new.c_cflag &= ~CSTOPB;		// 1 stop bits
	}

	//other attributions default
	termios_new.c_oflag &= ~OPOST;			
	termios_new.c_cc[VMIN]  = 1;		
	termios_new.c_cc[VTIME] = 1;				// unit: (1/10)second

	tcflush(fd, TCIFLUSH);			
	ret = tcsetattr(fd, TCSANOW, &termios_new);	// TCSANOW
	tcgetattr(fd, &termios_old);
	
	return ret;		
	
}

int COM_GetSetup(int fd, COM_PARAM *param)
{
	if (fd < 0)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	return 0;
}

int COM_Send(int fd, char *buffer, int size)
{
	int ret = -1;
	
	if (fd < 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
	
	ret = write(fd, buffer, size);	
	if (ret == size)
	{
		return 0;
	}
	else
	{
		tcflush(fd, TCOFLUSH);
		return -1;
	}
}

int COM_Receive(int fd, char *buffer, int *size)
{
	int ret = -1;
	fd_set fs_read;
	struct timeval timeout;
	
	if (fd < 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}
	if (size == NULL)
	{
		return -1;
	}
	
	FD_ZERO(&fs_read);
	FD_SET(fd, &fs_read);
	
#ifndef CYG	
	ioctl(fd, FIONREAD, &size);
	if (size == 0)
	{
		return -1;
	}
#endif	

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	ret = select(fd+1, &fs_read, NULL, NULL, &timeout);
	if (FD_ISSET(fd,&fs_read))
	{
		ret = read(fd, buffer, size);
		if (ret > 0)
		{
			return 0;
		}
		else
		{
			return -1;
		}
  	}
	
	return -1;
}

