#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

#include "smtp.h"

int main()
{
	int nRet = -1;
	char buffer[256*1024];
	
	nRet = SMTP_Send_Ext("smtp.tositech.net", "jerryzhuang@tositech.net", "jerry766", 
				  "jerryzhuang@tositech.net", "714452040@qq.com", NULL, NULL, NULL, 
				  "Test Email", "Test Email With big attach!", "run.txt", buffer, 128000);
				  
	printf("SMTP_Send_Ext: %d\n", nRet);
	
	return 0;
}

