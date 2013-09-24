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
		if (soap_ssl_accept(soap) != SOAP_OK)
		{ /* when soap_ssl_accept() fails, socket is closed and SSL data reset */
			soap_print_fault(soap, stderr);
			fprintf(stderr, "SSL request failed, continue with next call...\n");
		}
		else
		{
			soap_serve(soap);
		}

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

#if 0
	if(CRYPTO_thread_setup())
	{
		printf("Cannot setup thread mutex for OpenSSL\r\n");
		exit(1);
	}
#endif

//	soap_ssl_init();
	//初始话运行时环境
	soap_init(&ServerSoap);
	ServerSoap.accept_timeout = 10;
	ServerSoap.recv_timeout = 10;
	ServerSoap.send_timeout = 10;

	if(soap_ssl_server_context(&ServerSoap, 
				SOAP_SSL_DEFAULT, 
				"server.pem", /* keyfile: required when server must authenticate to clients (see SSL docs on how to obtain this file) */ 
				"password", /* password to read the key file */ 
				"cacert.pem", /* optional cacert file to store trusted certificates */ 
				NULL, /* optional capath to directory with trusted certificates */ 
				"dh512.pem", /* DH file, if NULL use RSA */ 
				NULL, /* if randfile!=NULL: use a file with random data to seed randomness */ 
				NULL /* optional server identification to enable SSL session cache (must be a unique name) */    ))
	{
		soap_print_fault(&ServerSoap, stderr);
		exit(1);
	}

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

	soap_destroy(&ServerSoap);
	soap_end(&ServerSoap);
	//分离运行时的环境
	soap_done(&ServerSoap);

	CRYPTO_thread_cleanup();

	return 0;
}

#include "openssl/ssl.h"
#include "unistd.h" /* defines _POSIX_THREADS if pthreads are available */ 
#if defined(WIN32) 
# define MUTEX_TYPE HANDLE 
# define MUTEX_SETUP(x) (x) = CreateMutex(NULL, FALSE, NULL) 
# define MUTEX_CLEANUP(x) CloseHandle(x) 
# define MUTEX_LOCK(x) WaitForSingleObject((x), INFINITE) 
# define MUTEX_UNLOCK(x) ReleaseMutex(x) 
# define THREAD_ID GetCurrentThreadID() 
#elif defined(_POSIX_THREADS) 
# define MUTEX_TYPE pthread_mutex_t 
# define MUTEX_SETUP(x) pthread_mutex_init(&(x), NULL) 
# define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x)) 
# define MUTEX_LOCK(x) pthread_mutex_lock(&(x)) 
# define MUTEX_UNLOCK(x) pthread_mutex_unlock(&(x)) 
# define THREAD_ID pthread_self() 
#else 
# error "You must define mutex operations appropriate for your platform" 
# error "See OpenSSL /threads/th-lock.c on how to implement mutex on your platform" 
#endif 

struct CRYPTO_dynlock_value { MUTEX_TYPE mutex; }; 
static MUTEX_TYPE *mutex_buf; 
static struct CRYPTO_dynlock_value *dyn_create_function(const char *file, int line) 
{ 
	struct CRYPTO_dynlock_value *value; 
	value = (struct CRYPTO_dynlock_value*)malloc(sizeof(struct CRYPTO_dynlock_value)); 
	if (value) 
		MUTEX_SETUP(value->mutex); 
	return value; 
}
 
static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line) 
{ 
	if (mode & CRYPTO_LOCK) 
		MUTEX_LOCK(l->mutex); 
	else
		MUTEX_UNLOCK(l->mutex); 
}
 
static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line) 
{ 
	MUTEX_CLEANUP(l->mutex); 
	free(l); 
}
 
void locking_function(int mode, int n, const char *file, int line) 
{ 
	if (mode & CRYPTO_LOCK) 
		MUTEX_LOCK(mutex_buf[n]); 
	else
		MUTEX_UNLOCK(mutex_buf[n]); 
}
 
unsigned long id_function() 
{ 
	return (unsigned long)THREAD_ID; 
}
 
int CRYPTO_thread_setup() 
{ 
	int i; 
	mutex_buf = (MUTEX_TYPE*)malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE)); 
	if (!mutex_buf) 
		return SOAP_EOM; 
	for (i = 0; i < CRYPTO_num_locks(); i++) 
		MUTEX_SETUP(mutex_buf[i]); 
	CRYPTO_set_id_callback(id_function); 
	CRYPTO_set_locking_callback(locking_function); 
	CRYPTO_set_dynlock_create_callback(dyn_create_function); 
	CRYPTO_set_dynlock_lock_callback(dyn_lock_function); 
	CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function); 
	return SOAP_OK; 
} 

void CRYPTO_thread_cleanup() 
{ 
	int i; 
	if (!mutex_buf) 
		return; 
	CRYPTO_set_id_callback(NULL); 
	CRYPTO_set_locking_callback(NULL); 
	CRYPTO_set_dynlock_create_callback(NULL); 
	CRYPTO_set_dynlock_lock_callback(NULL); 
	CRYPTO_set_dynlock_destroy_callback(NULL); 
	for (i = 0; i < CRYPTO_num_locks(); i++) 
		MUTEX_CLEANUP(mutex_buf[i]); 
	free(mutex_buf); 
	mutex_buf = NULL; 
}
