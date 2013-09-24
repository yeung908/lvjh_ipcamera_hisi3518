#ifndef __LOG_H_
#define __LOG_H_

#ifndef CHINESE0
#define CHINESE0			0
#endif

#ifndef CHINESE1
#define CHINESE1			1
#endif

#ifndef ENGLISH
#define ENGLISH			2
#endif

/* 日志类型 */
#define ALL_TYPE_LOG		0x0000
#define SYSTEM_LOG			0x0100
#define ALARM_LOG			0x0200
#define NET_LOG				0x0300
#define OPERATE_LOG			0x0400

/* 定义系统日志类型 */
#define TIMER_POWER_OFF		0x0100
#define TIMER_POWER_ON		0x0101
#define USER_LOGIN			0x0110
#define USER_LOGOUT			0x0111
#define USER_ADD			0x0112
#define USER_DEL			0x0113
#define USER_MODIFY			0x0114

/* 定义报警日志类型 */
#define	MOTION_ALARM		0x0200
#define LOST_ALARM			0x0201
#define INPUT_ALARM			0x0202

/* 定义网络日志类型 */
#define REMOTE_LOGIN		0x0300
#define REMOTE_LOGOUT		0x0301
#define REMOTE_PLAY			0x0302

/* 定义本地操作日志类型 */
#define SET_SYSTEM_TIME		0x0400
#define SET_RECORD_PARAM	0x0401
#define SET_TIME_RECORD		0x0402
#define SET_MOTION_ALARM	0x0403
#define SET_LOST_ALARM		0x0404
#define SET_INPUT_ALARM		0x0405
#define SET_NET_PARAM		0x0406
#define SET_HOST_PARAM		0x0407
#define SET_CHANNEL_PARAM	0x0408
#define SET_COM_PARAM		0x0409
#define SET_FACTORY_PARAM	0x040A
#define SET_ONE_PIC			0x040B
#define SET_FOUR_PIC		0x040C
#define SET_NINE_PIC		0x040D
#define SET_SIXTEEN_PIC		0x040E
#define SET_NORMAL_RECORD   0x040F

#define START_LOCAL_PLAY	0x0420
#define STOP_LOCAL_PLAY		0x0421
#define START_LOCAL_BACKUP	0x0422
#define STOP_LOCAL_BACKUP	0x0423

#define DISK_PARTITION		0x0430
#define DISK_FORMAT			0x0431
#define DISK_DELETE			0x0432

#define POWER_ON			0x0433
#define POWER_OFF			0x0434
#define RESTART_SYS			0x0435

#define SET_TIME_POWER_ON_OFF   0x0436

typedef struct __LOG_CODE
{
	int type;
	unsigned char *text;
}LOG_CODE;

typedef struct __LOG_RECORD
{
	int type;

	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
	
	unsigned char port;
	
	unsigned int  ip;
	unsigned char user_name[24];
}LOG_RECORD;

typedef struct __LOG_RECORD_LIST
{
	LOG_RECORD *record;
	struct __LOG_RECORD_LIST *next;	
}LOG_RECORD_LIST;


int get_log_text_by_type(int type,char *text);
int write_log_file(int type,unsigned char port,unsigned int ip,unsigned char *user_name);
int find_log_file(int type,int year,int month,int day);
int query_log_file(int type,int year,int month,int day);
int free_query_log_file();
int get_log_record_num();
int get_first_log_record(LOG_RECORD *record);
int get_next_log_record(LOG_RECORD *record);

#endif

