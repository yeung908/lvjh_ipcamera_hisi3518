#ifndef __DDNS_H_
#define __DDNS_H_

int DDNS_Open();
int DDNS_Close();
//int DDNS_Setup(char *user, char *pass, char *server, int port, char *auth, char *domain, int type);
//int DDNS_GetSetup(char *user, char *pass, char *server, int port, char *auth, char *domain, int *type);
int DDNS_Setup(char *user, char *pass, char *server, int port, char *auth, char *domain, int *type, int webport);
int DDNS_GetSetup(char *user, char *pass, char *server, int port, char *auth, char *domain, int *type, int *webport);
int DDNS_Start();
int DDNS_Stop();
int set_register_ddns();
int reset_register_ddns();
int ddnsYiYuanPause();
int ddnsYiYuanResume(int alarm_type);

#if 0
typedef enum 
{
	HOBSS_TYPE 		= 1,
	YIYUAN_TYPE 	= 2,
	51DDNS_TYPE  	= 3,
}DDNS_SERVER_TYPE;
#endif


typedef enum
{
	DDNS_HOBSS 		= 1,
	DDNS_YIYUAN 	= 2,
	DDNS_51DDNS  	= 3,
	DDNS_OTHRE  	= 4,

}DDNS_SERVER_TYPE;


#if 0
#define 51DDNS 			1
#define HOSS			2
#endif
int set_snapshot_buffer_size(char *snapshot_buffer, int snapshot_size);

#endif
