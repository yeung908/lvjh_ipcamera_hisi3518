#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "cgi_param.h"
#include "param.h"


	
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define	MAXLINE			4096	/* max text line length */
#define	SERV_FIFO		"/tmp/fifo.serv"
#define    MAX_PIPE_LEN        1024

typedef enum{
	get = 1,
	set = 2,
}DIRECT;
typedef enum{
	RE_SYS_USER_PARAM                           = 0,
	RE_NET_PARAM                                      = 1,
	RE_VIDEO_PARAM                                  = 2,
	RE_VIDEO_MOVE_PARAM                     = 3,
	RE_VIDEO_LOST_PARAM                      = 4,
	RE_PROBE_IN_PARAM                           = 5,
	RE_PROBE_OUT_PARAM                       = 6,
	RE_SNAPSHOT_PARAM                          = 7,
	RE_TIME_RECORD_PARAM         	      = 8,
	RE_MOVE_DIRECT_PARAM                    = 9,
	RE_VIDEO_OSD_PARAM                          =10,
	RE_SAVE_DEV_PARAM                            =11,

}CGI_GET_PARAM_TYPE;

typedef struct _cgi_param_request
{
	int DataType;   //请求数据类型
	int getorset;    // 1:get 2:set
	char data[MAX_PIPE_LEN];     //数据内容
}CGI_PARAM_REQUEST;

typedef struct{
	NTP_PARAM ntp;
	SYS_INFO sysInfo;
	USER_INFO_PARAM userInfo;
}S_CGI_SYS_USER;

typedef struct{
	// 网络参数
	NET_PARAM network; 

	// Wlan
	WIFI_PARAM wlan;

	G3_PARAM g3;

	// DDNS
	DDNS_PARAM ddns;

	// PPPOE
	PPPOE_PARAM pppoe;

	// FTP参数 
	FTP_PARAM ftp;

	// TFTP参数
	TFTP_PARAM tftp;

	// SMTP参数
	EMAIL_PARAM email;

	// REMOTE CONNECT
	REMOTE_CONNECT_PARAM remoteConnectParam;

	DVSNET_P2P_PARAM  p2p;
}S_CGI_NET_PARAM;

typedef struct {
	// 编码参数
	VENC_PARAM videoEnc[MAX_CHANNEL][2];	// 双码流

	// 视频属性
	VIDEO_IN_ATTR videoInAttr[MAX_CHANNEL];

	// 音频路径
	AUDIO_PATH_PARAM audioInPath[MAX_CHANNEL];
		// 视频HZ
	VIDEO_HZ_PARAM videoHz[MAX_CHANNEL];

	// 视频倒立
	VIDEO_FLIP_PARAM videoFlip[MAX_CHANNEL];

	// 视频镜像
	VIDEO_MIRROR_PARAM videoMirror[MAX_CHANNEL];
}S_CGI_VIDEO_PARAM;

typedef struct{
	RECORD_PARAM recordParam[MAX_CHANNEL];
	TIMER_RECORD_CHANNEL_PARAM timerRecordParam[MAX_CHANNEL];
}S_CGI_RECORD_PARAM;

typedef struct{
	// OSD参数
	OSD_PARAM osd[MAX_CHANNEL];

	// LOGO参数
	LOGO_PARAM logo[MAX_CHANNEL];

	// MASK参数
	MASK_PARAM mask[MAX_CHANNEL];
}S_CGI_OSD_PARAM;


void  to_cgi_sys_usr_param(int fd)
{
	int len;
	S_CGI_SYS_USER s_cgi_sys_user;
	memset(&s_cgi_sys_user,0,sizeof(S_CGI_SYS_USER));
	
	getNtpParam(&(s_cgi_sys_user.ntp));
	getSysInfoParam(&(s_cgi_sys_user.sysInfo));
	getUserInfoParam(&(s_cgi_sys_user.userInfo));
	len = sizeof(S_CGI_SYS_USER);
	if(write(fd,&s_cgi_sys_user,len ) != len){
		printf("write sys_usr_param to cgi failed\n");
		return ;
	}

}

void to_cgi_net_param(int fd)
{
	int len;
	S_CGI_NET_PARAM s_cgi_net_parm;
	memset(&s_cgi_net_parm,0,sizeof(S_CGI_NET_PARAM));

	getNetParam(&(s_cgi_net_parm.network));
	getWifiParam(&(s_cgi_net_parm.wlan));
	get3gParam(&(s_cgi_net_parm.g3));
	getDDNSParam(&(s_cgi_net_parm.ddns));
	getPPPOEParam(&(s_cgi_net_parm.pppoe));
	getFtpParam(&(s_cgi_net_parm.ftp));
	getTftpParam(&(s_cgi_net_parm.tftp));
	getEmailParam(&(s_cgi_net_parm.email));
	getRemoteConnectParam(&(s_cgi_net_parm.remoteConnectParam));
	getP2PParam(&(s_cgi_net_parm.p2p));
	
	len = sizeof(S_CGI_NET_PARAM);
	if(write(fd,&s_cgi_net_parm,len ) != len){
		printf("write sys_net_param to cgi failed\n");
		return ;
	}
}
void to_cgi_video_param(int fd)
{
	int len;
	
	S_CGI_VIDEO_PARAM s_cgi_video_param;
	memset(&s_cgi_video_param,0,sizeof(S_CGI_VIDEO_PARAM));
	
	getVideoEncParam(0, 0, s_cgi_video_param.videoEnc[0]);
	getVideoEncParam(0,1, &(s_cgi_video_param.videoEnc[0][1]));
	getVideoInAttrParam(0, s_cgi_video_param.videoInAttr);
	getAudioInPathParam(0, s_cgi_video_param.audioInPath);
	getVideoHzParam(0,s_cgi_video_param.videoHz);
	getVideoFlipParam(0,s_cgi_video_param.videoFlip);
	getVideoMirrorParam(0,s_cgi_video_param.videoMirror);

	len = sizeof(S_CGI_VIDEO_PARAM);
	if(write(fd,&s_cgi_video_param,len ) != len){
		printf("write sys_video param to cgi failed\n");
		return ;
	}
	
}
void to_cgi_video_move_param(int fd)
{
	int len;
	VIDEO_MOTION_ALARM_PARAM videoMotionAlarm[MAX_CHANNEL];
	memset(videoMotionAlarm,0,sizeof(VIDEO_MOTION_ALARM_PARAM));
	getVideoMotionAlarmParam(0, videoMotionAlarm);
	len = sizeof(VIDEO_MOTION_ALARM_PARAM);
	if(write(fd,videoMotionAlarm,len ) != len){
		printf("write video move param to cgi failed\n");
		return ;
	}
}

void to_cgi_video_lost_parm(int fd)
{
	int len = 0;
	VIDEO_LOST_ALARM_PARAM videoLostAlarm[MAX_CHANNEL];
	memset(videoLostAlarm,0,sizeof(VIDEO_LOST_ALARM_PARAM));
	getVideoLostAlarmParam(0, videoLostAlarm);

	len = sizeof(VIDEO_LOST_ALARM_PARAM);
	if(write(fd,videoLostAlarm,len ) != len){
		printf("write video lost param to cgi failed\n");
		return ;
	}
}

void to_cgi_probe_in_param(int fd)
{
	int len;
	PROBE_IN_ALARM_PARAM  param;
	memset(&param,0,sizeof(PROBE_IN_ALARM_PARAM));
	getProbeInAlarmParam(0,&param);
	//sprintf(param.probeName,"zhangjing");
	len = sizeof(PROBE_IN_ALARM_PARAM);
	if(write(fd,&param,len ) != len){
		printf("write probe in param to cgi failed\n");
		return ;
	}
}

void to_cgi_probe_out_param(int fd)
{
	int len;
	PROBE_OUT_ALARM_PARAM  param;
	memset(&param,0,sizeof(PROBE_OUT_ALARM_PARAM));
	getProbeOutAlarmParam(0,&param);
	//sprintf(param.probeName,"zhangjing");
	len = sizeof(PROBE_OUT_ALARM_PARAM);
	if(write(fd,&param,len ) != len){
		printf("write probe out param to cgi failed\n");
		return ;
	}
}

void to_cgi_snapshot_param(int fd)
{
	int len;
	SCHEDULE_SNAPSHOT_PARAM  param;
	memset(&param,0,sizeof(SCHEDULE_SNAPSHOT_PARAM));
	 getScheduleSnapshotParam(0,&param);
	//param.nSnapshotInterval=12345;
	len = sizeof(SCHEDULE_SNAPSHOT_PARAM);
	if(write(fd,&param,len ) != len){
		printf("schedule snapshot  param to cgi failed\n");
		return ;
	}
}

void to_cgi_time_record_param(int fd)
{
	int len,i;
	#ifdef RECORD
	S_CGI_RECORD_PARAM param;
	memset(&param,0,sizeof(S_CGI_RECORD_PARAM));
	
	getRecordParam(0,param.recordParam);
	getTimerRecordParam(0,param.timerRecordParam);
	
	len = sizeof(S_CGI_RECORD_PARAM);
	if(write(fd,&param,len) != len){
		printf("time record  param to cgi failed\n");
		return ;
	}
	#endif
}

void to_cgi_move_dirtect_param(int fd)
{
	int len;
	VIDEO_MOTION_PARAM  param;
	memset(&param,0,sizeof(VIDEO_MOTION_PARAM));
	 getVideoMotionParam(0,&param);
	//param.nSensibility= 100;
	len = sizeof(VIDEO_MOTION_PARAM);
	if(write(fd,&param,len ) != len){
		printf("video motion param to cgi failed\n");
		return ;
	}
	
}

void to_cgi_video_osd_param(int fd)
{
	int len;
	S_CGI_OSD_PARAM s_cgi_osd_param;
	unsigned char logoData[MAX_LOGO_DATA];

	memset(&s_cgi_osd_param,0,sizeof(S_CGI_OSD_PARAM));
	memset(logoData,0,MAX_LOGO_DATA);
	
	getOsdParam(0,s_cgi_osd_param.osd);
	getLogoParam(0,s_cgi_osd_param.logo, logoData);
	//getMosaicParam(0,s_cgi_osd_param.mask);
         getMaskParam(0, s_cgi_osd_param.mask);
	
	len = sizeof(S_CGI_OSD_PARAM);
	if(write(fd,&s_cgi_osd_param,len ) != len){
		printf("video motion param to cgi failed\n");
		return ;
	}
}

void from_cgi_sys_usr_param(char *content){
	
	S_CGI_SYS_USER s_cgi_sys_user;
	
	memset(&s_cgi_sys_user,0,sizeof(S_CGI_SYS_USER));
	memcpy(&s_cgi_sys_user,content,sizeof(S_CGI_SYS_USER));
	
	setNtpParam(&(s_cgi_sys_user.ntp));
	setSysInfoParam(&(s_cgi_sys_user.sysInfo));
	setUserInfoParam(&(s_cgi_sys_user.userInfo));


}
void from_cgi_net_param(char *content){
	S_CGI_NET_PARAM s_cgi_net_parm;
	
	memset(&s_cgi_net_parm,0,sizeof(S_CGI_NET_PARAM));
	memcpy(&s_cgi_net_parm,content,sizeof(S_CGI_NET_PARAM));
	
	setNetParam(&(s_cgi_net_parm.network));
	setWifiParam(&(s_cgi_net_parm.wlan));
	set3gParam(&(s_cgi_net_parm.g3));
	setDDNSParam(&(s_cgi_net_parm.ddns));
	setPPPOEParam(&(s_cgi_net_parm.pppoe));
	setFtpParam(&(s_cgi_net_parm.ftp));
	setTftpParam(&(s_cgi_net_parm.tftp));
	setEmailParam(&(s_cgi_net_parm.email));
	setRemoteConnectParam(&(s_cgi_net_parm.remoteConnectParam));
	setP2PParam(&(s_cgi_net_parm.p2p));
}
void from_cgi_video_param(char *content){
	S_CGI_VIDEO_PARAM s_cgi_video_param;
	
	memset(&s_cgi_video_param,0,sizeof(S_CGI_VIDEO_PARAM));
	memcpy(&s_cgi_video_param,content,sizeof(S_CGI_VIDEO_PARAM));
	
	setVideoEncParam(0, 0, s_cgi_video_param.videoEnc[0]);
	setVideoEncParam(0,1, &(s_cgi_video_param.videoEnc[0][1]));
	setVideoInAttrParam(0, s_cgi_video_param.videoInAttr);
	setAudioInPathParam(0, s_cgi_video_param.audioInPath);
	setVideoFlipParam(0,s_cgi_video_param.videoFlip);
	setVideoMirrorParam(0,s_cgi_video_param.videoMirror);
}
void from_cgi_video_move_param(char *content){
	VIDEO_MOTION_ALARM_PARAM videoMotionAlarm[MAX_CHANNEL];
	
	memset(videoMotionAlarm,0,sizeof(VIDEO_MOTION_ALARM_PARAM));
	memcpy(videoMotionAlarm,content,sizeof(VIDEO_MOTION_ALARM_PARAM));
	
	setVideoMotionAlarmParam(0, videoMotionAlarm);
}
void from_cgi_video_lost_parm(char *content){
	VIDEO_LOST_ALARM_PARAM videoLostAlarm[MAX_CHANNEL];
	
	memset(videoLostAlarm,0,sizeof(VIDEO_LOST_ALARM_PARAM));
	memcpy(videoLostAlarm,content,sizeof(VIDEO_LOST_ALARM_PARAM)); 
	
	setVideoLostAlarmParam(0, videoLostAlarm);

}
void from_cgi_probe_in_param(char *content){
	PROBE_IN_ALARM_PARAM  param;
	
	memset(&param,0,sizeof(PROBE_IN_ALARM_PARAM));
	memcpy(&param,content,sizeof(PROBE_IN_ALARM_PARAM)); 
	
	setProbeInAlarmParam(0,&param);
}
void from_cgi_probe_out_param(char *content){
	PROBE_OUT_ALARM_PARAM  param;
	
	memset(&param,0,sizeof(PROBE_OUT_ALARM_PARAM));
	memcpy(&param,content,sizeof(PROBE_OUT_ALARM_PARAM)); 
	
	setProbeOutAlarmParam(0,&param);
}
void from_cgi_snapshot_param(char *content){
	SCHEDULE_SNAPSHOT_PARAM  param;
	
	memset(&param,0,sizeof(SCHEDULE_SNAPSHOT_PARAM));
	memcpy(&param,content,sizeof(SCHEDULE_SNAPSHOT_PARAM)); 
	
	setScheduleSnapshotParam(0,&param);
}
void from_cgi_time_record_param(char *content){
	#ifdef RECORD
	S_CGI_RECORD_PARAM param;
	
	memset(&param,0,sizeof(S_CGI_RECORD_PARAM));
	memcpy(&param,content,sizeof(S_CGI_RECORD_PARAM)); 

	setRecordParam(0,param.recordParam);
	setTimerRecordParam(0,param.timerRecordParam);
	

	#endif
}
void from_cgi_move_dirtect_param(char *content){
	VIDEO_MOTION_PARAM  param;
	
	memset(&param,0,sizeof(VIDEO_MOTION_PARAM));
	memcpy(&param,content,sizeof(VIDEO_MOTION_PARAM)); 
	
	 setVideoMotionParam(0,&param);
}
void from_cgi_video_osd_param(char *content){
	S_CGI_OSD_PARAM s_cgi_osd_param;
	unsigned char logoData[MAX_LOGO_DATA];

	memset(&s_cgi_osd_param,0,sizeof(S_CGI_OSD_PARAM));
	memset(logoData,0,MAX_LOGO_DATA);
	memcpy(&s_cgi_osd_param,content,sizeof(S_CGI_OSD_PARAM)); 
	
	setOsdParam(0,s_cgi_osd_param.osd);
	setLogoParam(0,s_cgi_osd_param.logo, logoData);
	//setMosaicParam(0,s_cgi_osd_param.mosaic);
	setMaskParam(0,s_cgi_osd_param.mask);
	
}
void from_cgi_recev_save_cmd(){
	saveParamToFile();
}
int send_param_to_cgi(int type,int fd)
{
	switch(type)
	{
		case RE_SYS_USER_PARAM:
			to_cgi_sys_usr_param(fd);
			break;
		case RE_NET_PARAM:
			to_cgi_net_param(fd);
			break;
		case RE_VIDEO_PARAM:
			to_cgi_video_param(fd);
			break;
		case RE_VIDEO_MOVE_PARAM:
			to_cgi_video_move_param(fd);
			break;
		case RE_VIDEO_LOST_PARAM:
			to_cgi_video_lost_parm(fd);
			break;
		case RE_PROBE_IN_PARAM:
			to_cgi_probe_in_param(fd);
			break;
		case RE_PROBE_OUT_PARAM:
			to_cgi_probe_out_param(fd);
			break;
		case RE_SNAPSHOT_PARAM:
			to_cgi_snapshot_param(fd);
			break;
		case RE_TIME_RECORD_PARAM:
			to_cgi_time_record_param(fd);
			break;
		case RE_MOVE_DIRECT_PARAM:
			to_cgi_move_dirtect_param(fd);
			break;
		case RE_VIDEO_OSD_PARAM:
			to_cgi_video_osd_param(fd);
			break;
		default:
			break;
	}
	return 0;
	
}
int get_param_for_cgi(int type,char *content){
	switch(type)
	{
		case RE_SYS_USER_PARAM:
			printf("CGI SET SYS USER PARAM\n");
			from_cgi_sys_usr_param(content);
			break;
		case RE_NET_PARAM:
			printf("CGI SET NET_PARAM\n");
			from_cgi_net_param(content);
			break;
		case RE_VIDEO_PARAM:
			printf("CGI SET VIDEO_PARAM\n");
			from_cgi_video_param(content);
			break;
		case RE_VIDEO_MOVE_PARAM:
			printf("CGI SET VIDEO_MOVE_PARAM\n");
			from_cgi_video_move_param(content);
			break;
		case RE_VIDEO_LOST_PARAM:
			printf("CGI SET VIDEO_LOST_PARAM\n");
			from_cgi_video_lost_parm(content);
			break;
		case RE_PROBE_IN_PARAM:
			printf("CGI SET PROBE_IN_PARAM\n");
			from_cgi_probe_in_param(content);
			break;
		case RE_PROBE_OUT_PARAM:
			printf("CGI SET PROBE_OUT_PARAM\n");
			from_cgi_probe_out_param(content);
			break;
		case RE_SNAPSHOT_PARAM:
			printf("CGI SET SNAPSHOT_PARAM\n");
			from_cgi_snapshot_param(content);
			break;
		case RE_TIME_RECORD_PARAM:
			printf("CGI SET TIME_RECORD_PARAM\n");
			from_cgi_time_record_param(content);
			break;
		case RE_MOVE_DIRECT_PARAM:
			printf("CGI SET MOVE_DIRECT_PARAM\n");
			from_cgi_move_dirtect_param(content);
			break;
		case RE_VIDEO_OSD_PARAM:
			printf("CGI SET RE_VIDEO_OSD_PARAM\n");
			from_cgi_video_osd_param(content);
			break;
		case RE_SAVE_DEV_PARAM:
			printf("CGI SET SAVE_DEV_PARAM\n");
			from_cgi_recev_save_cmd();
			break;
		default:
			break;
	}
	return 0;
}
int cgi_param_server()
{
	int		readfifo, writefifo, dummyfd, fd;
	char		*ptr, buff[MAXLINE], fifoname[MAXLINE];
	pid_t	pid;
	ssize_t	n;
	
	CGI_PARAM_REQUEST cgi_param_request;
	memset(&cgi_param_request,0,sizeof(CGI_PARAM_REQUEST));

		/* 4create server's well-known FIFO; OK if already exists */
	if ((mkfifo(SERV_FIFO, FILE_MODE) < 0) && (errno != EEXIST)){
		printf("can't create %s", SERV_FIFO);
		return -1;
	}
		/* 4open server's well-known FIFO for reading and writing */
	if((readfifo = open(SERV_FIFO, O_RDONLY, 0))<0){
		printf("only read open serv_fifo fauilt\n");
		return -1;
	}
	if((dummyfd = open(SERV_FIFO, O_WRONLY, 0))<0){/* never used */
		printf("only write open serv_fifo fauilt\n");
		return -1;
	}		

	printf("cgi_param_server listen client .......\n");
	while ( (n = read(readfifo, buff, MAXLINE)) > 0)
	{

		memcpy(&cgi_param_request,buff,sizeof(CGI_PARAM_REQUEST));
		
		snprintf(fifoname, sizeof(fifoname), "/tmp/fifo.%d", cgi_param_request.DataType);
		if(cgi_param_request.getorset == get)
		{
			if ( (writefifo = open(fifoname, O_WRONLY, 0)) < 0) 
			{
				printf("cannot open: %s", fifoname);
				continue;
			}
			printf("cgi_param_request.DataType = %d\n", cgi_param_request.DataType);
			send_param_to_cgi(cgi_param_request.DataType,writefifo);
			//write(writefifo, buff, n);
			
			close(fd);
			close(writefifo);
			
		}
		else if(cgi_param_request.getorset == set)
		{
			printf("cgi_param_request.DataType = %d\n", cgi_param_request.DataType);
			get_param_for_cgi(cgi_param_request.DataType,cgi_param_request.data);//
			close(fd);
			close(writefifo);
		}
		else
		{

		}
	}
}
int cgi_init_param_channel(void)
{
	pthread_t cig_server_thread;
	int ret = -1;
	
	ret = pthread_create(&cig_server_thread, NULL, (void *)cgi_param_server, NULL);
	if (ret){
		printf("create cgi server thread fault\n");
		return -1;
	}
	return 0;

}

