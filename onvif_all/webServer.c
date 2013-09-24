#include <pthread.h>
#include "onvif.nsmap"
#include "soapH.h"
#include "soapStub.h"
#include "openssl/ssl.h"


/* 宏与全局变量的定义 */
#define BACKLOG (100) 
#define MAX_THR (10)   
#define MAX_QUEUE (1000)


pthread_mutex_t queue_cs;//队列锁
pthread_cond_t queue_cv;//条件变量
SOAP_SOCKET queue[MAX_QUEUE];//数组队列
int head =0, tail =0;//队列头队列尾初始化         


void * process_queue(void *);//线程入口函数
int enqueue(SOAP_SOCKET);//入队列函数
SOAP_SOCKET dequeue(void);//出队列函数


/* 线程入口函数 */
void * process_queue(void * soap)
{
	struct soap * tsoap = (struct soap *)soap;
	for(;;)
	{
		tsoap->socket = dequeue();
		if(!soap_valid_socket(tsoap->socket))
		{
			break;
		}
		soap_serve(tsoap);
		soap_destroy(tsoap);
		soap_end(tsoap);
	}
	return NULL;
}
/* 入队列操作 */
int enqueue(SOAP_SOCKET sock)
{
	int status = SOAP_OK;
	int next;

	pthread_mutex_lock(&queue_cs);
	next = tail +1;
	if (next >= MAX_QUEUE) 
	{
		next = 0;
	}
	if (next == head) 
	{
		status = SOAP_EOM;
	}
	else
	{
		queue[tail] =sock;
		tail = next;
	}
	pthread_cond_signal(&queue_cv);
	pthread_mutex_unlock(&queue_cs);

	return status;
}

/* 出队列操作 */
SOAP_SOCKET dequeue()
{
	SOAP_SOCKET sock;

	pthread_mutex_lock(&queue_cs);
	while (head == tail )
	{
		pthread_cond_wait(&queue_cv,&queue_cs);
	}

	sock = queue[head++];
	if (head >= MAX_QUEUE)
	{
		head =0;
	}
	pthread_mutex_unlock(&queue_cs);

	return sock;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////
//主函数
int main(int argc,char ** argv)
{
	struct soap ServerSoap;
	struct ip_mreq mcast;
	int count = 0;

//	soap_ssl_init();
	//初始话运行时环境
	soap_init(&ServerSoap);
	ServerSoap.accept_timeout = 10;
	ServerSoap.recv_timeout = 10;
	ServerSoap.send_timeout = 10;

	soap_set_namespaces(&ServerSoap, namespaces);

	if(!soap_valid_socket(soap_bind(&ServerSoap, NULL, 4000, 10)))
	{
		soap_print_fault(&ServerSoap, stderr);
		exit(1);
	}

	//如果没有参数，当作CGI程序处理
	struct soap * soap_thr[MAX_THR];
	pthread_t tid[MAX_THR];
	int i = 0, port = 3702;//atoi(argv[1]);
	SOAP_SOCKET m, s;

	//锁和条件变量初始化
	pthread_mutex_init(&queue_cs,NULL);
	pthread_cond_init(&queue_cv,NULL);

	//生成服务线程
	for(i = 0; i < MAX_THR; i++)
	{
		soap_thr[i] = soap_copy(&ServerSoap);
		fprintf(stderr,"Starting thread %d \r\n",i);
		pthread_create(&tid[i],NULL,(void*(*)(void*))process_queue,(void*)soap_thr[i]);
	}

	for(;;)
	{
#if 0
		if(soap_serve(&ServerSoap))
		{
			soap_print_fault(&ServerSoap, stderr);
		}

		soap_destroy(&ServerSoap);
		soap_end(&ServerSoap);
		printf("1aaaaaaaaaaaaaaaaaaa %x\r\n", ServerSoap.ip);
		printf("Accepted count %d, connection from IP = %d.%d.%d.%d socket = %d \r\n",
				count, ((ServerSoap.ip)>>24)&0xFF, ((ServerSoap.ip)>>16)&0xFF, ((ServerSoap.ip)>>8)&0xFF, (ServerSoap.ip)&0xFF, (ServerSoap.socket));
#else
		count++;
		//接受客户端的连接
		s = soap_accept(&ServerSoap);
		printf("accept_flags = %d, s = %d\r\n", ServerSoap.accept_flags, s);
		if(!soap_valid_socket(s)) 
		{
			if(ServerSoap.errnum) 
			{
				soap_print_fault(&ServerSoap,stderr);
				continue;
			}else
			{
				printf("Server timed out \r\n");
				continue;
			}
		}

		//客户端的IP地址
		printf("Accepted count %d, connection from IP = %lu.%lu.%lu.%lu socket = %d \r\n",
				count, ((ServerSoap.ip)>>24)&0xFF, ((ServerSoap.ip)>>16)&0xFF, ((ServerSoap.ip)>>8)&0xFF, (ServerSoap.ip)&0xFF, (ServerSoap.socket));
		//请求的套接字进入队列，如果队列已满则循环等待
		while(enqueue(s) == SOAP_EOM)
		{
			usleep(1000);
		}
#endif
	}

	//服务结束后的清理工作
	for(i = 0; i < MAX_THR; i++)
	{
		while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM) 
		{
			usleep(1000);
		}
	}

	for(i=0; i< MAX_THR; i++)
	{
		fprintf(stderr,"Waiting for thread %d to terminate ..",i);
		pthread_join(tid[i],NULL);
		fprintf(stderr,"terminated ");
		soap_done(soap_thr[i]);
		free(soap_thr[i]);
	}

	pthread_mutex_destroy(&queue_cs);
	pthread_cond_destroy(&queue_cv);

	//分离运行时的环境
	soap_done(&ServerSoap);

	return 0;
}

