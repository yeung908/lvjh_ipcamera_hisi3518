#ifndef _ONVIFSERVICE_H_INCLUDED
#define _ONVIFSERVICE_H_INCLUDED
#include <pthread.h>
#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

typedef unsigned long       DWORD;
typedef unsigned short      WORD;

#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

struct ONVIF_ConnThread
{
	pthread_t hthread;
	int		  sock;
	struct sockaddr_in remote_addr;
};

int ONVIF_ServiceStart();
void ONVIF_ServiceStop();
#endif
