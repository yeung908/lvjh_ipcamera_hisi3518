#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 

#include "cgivars.h"
#if 0
#include "htmllib.h"
#include "template.h"
#endif


#define DEBUG	0

#define CTRL_PTZ	0x10000+290

#define UP_START	0x00
#define UP_STOP		0x01
#define DOWN_START	0x02
#define DOWN_STOP	0x03
#define LEFT_START	0x04
#define LEFT_STOP	0x05
#define RIGHT_START	0x06
#define RIGHT_STOP	0x07

typedef struct
{
	unsigned long nCmd;
	unsigned long nValue;
	
}PTZ_CMD;

#define MAX_SEND_SIZE 64 
struct mymsgbuf 
{ 
	long mtype; 
	char mtext[MAX_SEND_SIZE]; 
}; 

int test(char *data)
{
	int nRet = -1;
	FILE *fp = NULL;
	char buf[128];
	int len = 0;
	int i = 0;
	
	
	sprintf(buf, "/mnt/mtd/dvs/mobile/debug.txt");
	
	fp = fopen(buf, "w+b");
	if (fp == NULL)
	{
		return -1;
	}
	
	fwrite(data, 1, strlen(buf), fp);
	
	fclose(fp);
	
	return 0;	
}

int ptzCtrl(int nCmd, int nParam)
{
	int nRet = -1;
	key_t key; 
	int msgqueue_id; 
	struct mymsgbuf qbuf; 
	PTZ_CMD param;
	
	key = 888888;
	
	msgqueue_id = msgget(key, IPC_CREAT|0660);
	if (msgqueue_id < 0)
	{
		return -1;
	}
	
	qbuf.mtype = CTRL_PTZ; 
	param.nCmd = nCmd;
	param.nValue = 0;
	
	memcpy(qbuf.mtext, &param, sizeof(PTZ_CMD));
	
	nRet = msgsnd(msgqueue_id, (struct msgbuf *)&qbuf, MAX_SEND_SIZE, IPC_NOWAIT);
	if (nRet < 0)
	{
		printf("msgsnd: failed!\n");
		return -1;
	}
	
	return 0;
}

int CmdSession(char **postvars, int form_method)
{
	int nRet = -1;
	int i = 0;
	char data[1024];
	int nLen = 0;
	
	printf("DEBUG(%d): ...\n", form_method);
	
	if(form_method == POST)
	{
#if DEBUG
		for (i=0; postvars[i]; i+=2)
		{
			printf("DEBUG(POST): [%s] = [%s]\n<br>", postvars[i], postvars[i+1]);
		}
#endif

		if(!strcmp(postvars[0], "CTRL_PTZ") && !strcmp(postvars[1], "UP"))
		{
			ptzCtrl(UP_START, 0);
		}
		if(!strcmp(postvars[0], "CTRL_PTZ") && !strcmp(postvars[1], "DOWN"))
		{
			ptzCtrl(DOWN_START, 0);
		}
		if(!strcmp(postvars[0], "CTRL_PTZ") && !strcmp(postvars[1], "LEFT"))
		{
			ptzCtrl(LEFT_START, 0);
		}
		if(!strcmp(postvars[0], "CTRL_PTZ") && !strcmp(postvars[1], "RIGHT"))
		{
			ptzCtrl(RIGHT_START, 0);
		}
	}
	else
	{
#if DEBUG
		memset(data, 0, 1024);
		for (i=0; postvars[i]; i+=2)
		{
			nLen = sprintf(data+nLen, "%d: [%s] = [%s]\n", i, postvars[i], postvars[i+1]);
		}
		//test(data);
#endif
		ptzCtrl(UP_START, 0);
	}
		
	return 0;
}

int main() 
{
    char **postvars = NULL;
    char **getvars = NULL;
    int form_method; // POST = 1, GET = 0 
	
    form_method = getRequestMethod();

    if(form_method == POST) 
    {
        getvars = getGETvars();
        postvars = getPOSTvars();
        
        // CGI start here, set COM port
    	CmdSession(postvars, form_method);
    }
    else if(form_method == GET) 
    {
        getvars = getGETvars();
        // CGI start here, set COM port
    	//CmdSession(getvars, form_method);
    	ptzCtrl(RIGHT_START, 0);
    }
 
    cleanUp(form_method, getvars, postvars);
    
    fflush(stdout);
    
    exit(0);
}
