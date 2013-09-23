#ifndef __PARAM_H_
#define __PARAM_H_

#include "videoEnc/videoEncModule.h"
#include "audioEnc/audioEncModule.h"
#include "videoIn/videoInModule.h"
#include "com.h"
#include "ptz.h"
#include "rtc.h"
#include "ntpSetup.h"
#include "wlanSetup.h"
#include "3g.h"

#ifdef RECORD
#include "include/recordStruct.h"
#include "include/recordSDK.h"
#endif

#define HARDWARE_VERSION			0x00010003
#define SOFTWARE_VERSION			0x00030052
#define SOFTWARE_VERSION_EXT		0x000D0030
#define SOFTWARE_COMPILE_DATE		0x07DD00802
#define AUDIOSAMPLES				8000


#define PARAM_FILE_PATH				"/param"
#define PARAM_CONFIGURE_FILE		"/param/param.cur"
#define PARAM_STATUS_FILE			"/param/param.stat"
#define DEVICE_CONFIGURE_FILE		"/param/dev.conf"
#define DEBUG_CONFIGURE_FILE		"/param/debug.conf"
#define NETWORK_CONFIGURE_FILE		"/param/network.conf"
//#define MAC_CONFIGURE_FILE			"/mnt/mtd/dvs/etc/mac.conf"
#define MAC_CONFIGURE_FILE			"/param/mac.conf"
#define USER_INFO_CONFIGURE_FILE	"/param/user_info.conf"
#define RTSPPORT_CONFIGURE_FILE		"/param/RTSPPort.conf"
#define WEBSERVERPORTPATH 			"/param/webServerPort.conf"
#define WEBSERVER_MOBILE_CONFIG 	"/param/webserMobileConfig.conf"
#define UPNP_CONFIGURE_FILE			"/param/upnp.conf"
#define IP_WIFI_CONFIGURE_FILE		"/param/ip_wifi.conf"
#define PARAM_REBOOT_HOUR_FILE		"/param/param.hour"



#define ID_CONFIGURE_FILE			"/param/id.conf"
#define P2P_CONFIGURE_FILE			"/param/p2p.conf"


#define ETH_WIRE_DEV				"eth0"
//#define ETH_WIRELESS_DEV			"rausb0"
#define ETH_WIRELESS_DEV			"ra0"



#define MAX_CHANNEL					1
#define MAX_PROBE_IN				12
#define MAX_PROBE_OUT				1

#define MAX_TIME_SEGMENT			2
#define MAX_COM_NUM					2
#define MAX_IP_LEN					16
#define MAX_LOGO_DATA				30*1000
#define MAX_VIDEO_MASK_NUM			4

#define SOFTWARE_RESET				1001
#define HARDWARE_RESET				1002
#define UPNP_MAPPING_PROT			19000	
#define WIFI_CONF_PATH 				"/param/wifi.conf"
#define WIFI_STATUS_CONF_PATH 		"/param/wifi_status.conf"
#define HTTPD_STATUS_CONF_PATH 		"/param/httpd.conf"


#define NET_MAX_AP_NUMBER 			40

#ifdef IPHONE
#define DEFAULT_WIRE_IP				"192.168.188.188"
#define DEFAULT_WIRE_GATEWAY		"192.168.188.1"
#else
#define DEFAULT_WIRE_IP		    	"192.168.100.19" //"192.168.1.250"


#define DEFAULT_WIRE_GATEWAY		"192.168.100.1"
#endif

#define DEFAULT_WIRE_MASK			"255.255.255.0"

#define DEFAULT_WIRELESS_IP			"192.168.100.17"
#define DEFAULT_WIRELESS_GATEWAY	"192.168.100.1"
#define DEFAULT_WIRELESS_MASK		"255.255.255.0"

#define DEFAULT_MULTI_ADDR			"234.5.6.7" //"224.0.0.2"
#define DEFAULT_WEB_PORT			80
#define DEFAULT_RTSP_PORT			554


#ifdef IPHONE
#define DEFAULT_CMD_PORT			9500
#define DEFAULT_MEDIA_PORT			9500
#define DEFAULT_MULTI_PORT			9500
#else
#define DEFAULT_CMD_PORT			4000
#define DEFAULT_MEDIA_PORT			3000//4000
#define DEFAULT_MULTI_PORT			6000 //4000
#endif

/*
#ifdef CCD
#define DEFAULT_BRIGHTNESS			0x80
#define DEFAULT_CONTRAST			0x80
#define DEFAULT_HUE					0x00
#define DEFAULT_SATURATION			0x80
#endif
*/

#ifdef CCD
#define DEFAULT_BRIGHTNESS			0xA5
#define DEFAULT_CONTRAST			0xBE
#define DEFAULT_HUE					0x00
#define DEFAULT_SATURATION			0xBE
#endif


#ifdef HD_CMOS
#define DEFAULT_BRIGHTNESS			39
#define DEFAULT_CONTRAST			255
//#define DEFAULT_HUE					0x80
#define DEFAULT_HUE					0x00
#define DEFAULT_SATURATION			202
#endif

#define SPECIAL_IP_MANUAL_UPLOAD	0x10
#define SPECIAL_IP_AUTO_UPLOAD		0x08
#define FTP_UPLOAD					0x02
#define TFTP_UPLOAD					0x04
#define EMAIL_UPLOAD				0x01
#define LOCAL_STORE					0x20
#define SCHEDULE_EMAIL_UPLOAD		0x21



#define MAX_USER_COUNT				10	//最大用户数
#define ADMIN_USER					"888888" //"admin"
#define ADMIN_PASSWD 				"888888" //"admin"

//#define ADMIN_USER					"888888" //"admin"
#define GUEST_USER					"1"      //"guest"

//视频
#define ENCODE_VIDEO_DIVX			0x58564944
#define ENCODE_VIDEO_XVID			0x44495658
#define ENCODE_VIDEO_HISI			0x49534948
#define ENCODE_VIDEO_H264			0x34363248
#define ENCODE_VIDEO_MJPEG			0x47504A4D

//音频
#define ENCODE_AUDIO_MP3			0x55
#define ENCODE_AUDIO_G722 			0x65
#define ENCODE_AUDIO_G723 			0x14
//#define ENCODE_AUDIO_G711			0x25
#define ENCODE_AUDIO_G711			0x6
#define ENCODE_AUDIO_G726			0x64

// 系统信息: 256 bytes
typedef struct
{
	char strDeviceName[32];				//服务器名字	
	char strDeviceID[48];				//服务器ID
	unsigned long nLanguage;
	
	// 以下参数不能修改
	unsigned long nHardwareVersion;		// 硬件版本,高16位是主版本,低16位是次版本
	unsigned long nSoftwareVersion;		//应用程序版本号,高16位是主版本号，低16位为次版本号
	unsigned long nSoftwareBuildDate;	//应用程序编译日期 0xYYYYMMDD
	
	unsigned long nCpuType;				//CPU类型
	unsigned long nCpuFreq;				//CPU主频
	unsigned long nRamSize;				//RAM的大小
	
	unsigned long nChnNum;				//AV通道个数
	unsigned long nVideoEncType;		//视频编码类型
	unsigned long nAudioEncType;		//音频编码类型
	
	unsigned char byDeviceType;			//设备类型, 1:DVR 2:ATM DVR 3:DVS 4:COMS IPCAMERA 5:CCD IPCAMERA 6:HDCOMS IPCAMERA
	
	unsigned char byAlarmInNum;			//探头输入数
	unsigned char byAlarmOutNum;		//探头输出数
	
	unsigned char byRS232Num;			//232串口个数
	unsigned char byRS485Num;			//485串口个数
	
	unsigned char byDiskNum;			//硬盘个数
	
	unsigned char byVGANum;				//VGA口的个数
	unsigned char byUSBNum;				//USB口的个数
	
	//unsigned long nReserve[32];		//保留
	
	unsigned long nDoubleStreamFlag;	//双码流标识
	unsigned long nReserve[31];			//保留
	
}SYS_INFO;

// IP 限制
typedef struct
{
	int nOnFlag;
	unsigned long nStartIP;
	unsigned long nStopIP;
	unsigned long nReserve;
}LOGON_IP_BIND_PARAM;


//用户信息
typedef struct
{
	char strName[32];			//用户名
	char strPsw[32];			//密码
	unsigned long nRight[4];		//128个功能权限

	LOGON_IP_BIND_PARAM ipBind;

}USER_INFO;						//最多10个用户

// 1152bytes
typedef struct
{
	USER_INFO Admin;
	USER_INFO Users[MAX_USER_COUNT];
	USER_INFO Guest;
	
}USER_INFO_PARAM;

//视频制式
typedef struct
{
	unsigned long nStandard;		// 0: PAL, 1: NTSC
	
}VIDEO_STANDARD_PARAM;

// 视频输入参数
typedef struct
{	
	unsigned long nBrightness; 	//亮度
	unsigned long nHue;  		//色彩
	unsigned long nContrast;		//对比度
	unsigned long nSaturation;  	//饱和度
	//unsigned long acutance;  	//锐度
		
	unsigned long reserve;
}VIDEO_IN_ATTR;

typedef struct
{
	char strName[32];
	unsigned long nReserve;
}VIDEO_CHANNEL_NAME;

/*
// 视频编码参数
typedef struct
{
	unsigned long nEncodeMode;		// 0: CBR, 1: VBR
	
	unsigned long nEncodeWidth;		// 图像宽度
	unsigned long nEncodeHeight;		// 图像高度
	unsigned long nKeyInterval;		// I帧间隔
	unsigned long nFramerate;		// 目标帧率 
	unsigned long nBitRate;			// 目标码率
	unsigned long nMaxQueue;		// 最大量化系数
	unsigned long nMinQueue;		// 最小量化系数
	
	unsigned long reserve;			// 当nEncodeMode为CBR时，reserve为0时，表示画质优先，reserve为1时，表示帧率优先
	
}VENC_PARAM;
*/

//WIFI 扫描参数
typedef struct
{
	char ch[8];
	char signal[8];
	char w_mode[8];
	char nt[8];
	char ssid[24];
	char bssid[24];
	char security[24];	
}WIFIINFO;

typedef struct tagDVSNET_WIFI_DEVICE
{
  char sSSID[36];   //WIFI 名字
 // char bSSID[36];   //WIFI MAC 地址
  unsigned long     nSignal;//信号强度
  unsigned long     nChannel;//通道号
  unsigned long     nNetType;//WIFI	模式
  unsigned long     nEncryptType;//认证类型
  unsigned long     nAuthMode;//加密类型
  unsigned long 	WPAFormat; // 密码类型0 为ascll码，1位16进制
  unsigned long     nReserved;
}DVSNET_WIFI_DEVICE;
typedef struct SDK_NetWifiDeviceAll
{
	int nDevNumber;
	DVSNET_WIFI_DEVICE vNetWifiDeviceAll[NET_MAX_AP_NUMBER];
}WIFI_DEVICE_ALL;

 typedef struct NODE
{
	DVSNET_WIFI_DEVICE wifi_param;
	struct NODE *next;
}List;


// 视频水印参数
typedef struct
{
    unsigned long nOnFlag;			// 启用标志
    char strKey[8];					// 数字水印密钥字符串
    char strSymbol[16];				// 数字水印标识字符串
    unsigned long nDensity;			// 数字水印安全级:0、1、2
    
}VIDEO_WM_PARAM;

//时间OSD
typedef struct 
{
	unsigned long bShow;			//是否显示
	unsigned long nFormat;			//格式: 1: XXXX/XX/XX 2:XXXX/XX/XX XX:XX:XX
	unsigned short x;				//X坐标
	unsigned short y;				//Y坐标
	unsigned long nColor;
}TIME_OSD;

//标题OSD
typedef struct 
{
	unsigned long bShow;			//是否显示
	unsigned short x;				//X坐标
	unsigned short y;				//Y坐标
	char sTitle[32];				//标题内容
	unsigned long nColor;
	
}TITLE_OSD;

//码率OSD
typedef struct 
{
	unsigned long bShow;			//是否显示
	unsigned short x;				//X坐标
	unsigned short y;				//Y坐标
	unsigned long nColor;

}BITSTREAM_OSD;

//OSD参数设置
typedef struct 
{
	TIME_OSD		TimeOSD;		//时间OSD
	BITSTREAM_OSD	BitsOSD;		//码率OSD
	TITLE_OSD		TitleOSD[4];	//标题OSD
}OSD_PARAM;

// LOGO参数
typedef struct
{
	unsigned long bShow;			//显示标志
	unsigned long bTranslucent;		//是否透明
	unsigned long nTransColor;		//透明色
	unsigned short x;				//X坐标
	unsigned short y;				//Y坐标
	unsigned long nWidth;			//宽, 必须是偶数
	unsigned long nHeight;			//高, 必须是偶数
	unsigned long nDataLen;			//图像数据长度	
	
}LOGO_PARAM;	// 实际发送、接收这个结构时，数据大小应为：sizeof(LOGO_PARAM)-30*1000+Width*Height*3

/*
// MASK参数
typedef struct
{
	unsigned long bMask;			//是否启用
	unsigned short x;				//X坐标
	unsigned short y;				//Y坐标
	unsigned short nWidth;			//宽度, 必须是偶数
	unsigned short nHeight;			//高度, 必须是偶数
}VIDEO_MASK;
*/

//视频遮挡设置
typedef struct
{
	VIDEO_MASK VideoMask[MAX_VIDEO_MASK_NUM];
}MASK_PARAM;

/*
// 视频移动检测参数
typedef struct
{
	unsigned char nSensibility;				//视频移动检测的灵敏度
	unsigned char mask[11*9];				//视频移动检测的宏块
	unsigned long nReserve;					//保留
	
}VIDEO_MOTION_PARAM;
*/

//音频属性
typedef struct
{
	unsigned long nBitRate;
	unsigned long nChannel;
	unsigned long nBitsPerChannel;
	
	unsigned long reserve;
}AUDIO_PARAM;

/*
//音频编码参数
typedef struct 
{
	unsigned long nOnFlag;		// 是否启用音频编码
	unsigned long nChannels;	// 输入通道数，可以是单通道，也可以是双通道
	unsigned long nSampleRate;	// 输入采样率
	unsigned long nBitRate;		// 输出码流
	unsigned long nMode;		// 编码模式：0：立体声，1：MS立体声，2：双通道，3：单通道
	
}AENC_PARAM;
*/

//音频解码参数
typedef struct 
{
	unsigned long nChannels;
	unsigned long nSampleRate;
	unsigned long nBitRate;
	unsigned long nMode;		//解码器类型
	
}ADEC_PARAM;

//ftp上传设置参数
/*
typedef struct
{
	char strIP[16];
	char strName[32];
	char strPsw[32];
	unsigned long nPort;	
	
}FTP_PARAM;
*/

//add code by lvjh 2012-08-18
typedef struct tagDVSNET_RTSP_PARAM
{
	unsigned long bEnable;
	unsigned long nRtspPort;
	unsigned long nRTPStartPort;
	unsigned long nReserve;
}DVSNET_RTSP_PARAM;

typedef struct
{
	char strIP[48];
	char strName[16];
	char strPsw[16];
	char strPath[32];
	unsigned long nPort;	

}FTP_PARAM;

//tftp上传设置参数
typedef struct
{
	char strIP[16];
	unsigned long nPort;	

}TFTP_PARAM;

//email上传设置参数
typedef struct
{
	char strIP[64];
	char strName[32];
	char strPsw[32];

	char strFrom[64];		//
	char strTo[128];		//考虑到多EMAIL
	char strCc[64];			//考虑到多EMAIL
	char strBcc[64];		//考虑到多EMAIL

	char strHeader[128];	//邮件标题

	unsigned long nReserve;
	
}EMAIL_PARAM;

#ifndef RECORD

typedef struct
{
	unsigned char start_hour;
	unsigned char start_minute;
	unsigned char end_hour;
	unsigned char end_minute;

}TIME_SEGMENT;

typedef struct
{
	unsigned long nOnFlag;
	TIME_SEGMENT time_segment[MAX_TIME_SEGMENT];

}TIME_SEGMENTS;

#endif

// 视频丢失报警参数
typedef struct
{		
	unsigned long nLinkProbe;				//探头报警联动
	unsigned long nLinkProbeTime;			//探头报警联动输出的时间,单位为秒
	
	unsigned long nLinkRecord;				//录像联动,按位操作
	unsigned long nLinkRecordTime;			//联动录像的时间,单位为分钟
	
	unsigned long nLinkSnapshot;			//抓拍联动
	unsigned long nLinkSnapshotInterval;	//联动抓拍间隔, 单位为秒
	unsigned long nLinkSnapshotUploadFlag;	//联动抓拍上传启用
	unsigned long nLinkSnapshotUploadMode;	//联动抓拍上传方式：smtp、ftp、tftp、客户端接收
	
	TIME_SEGMENTS day[8];					//时间段参数
	
	unsigned long reserve;					//保留
	
}VIDEO_LOST_ALARM_PARAM;

// 视频移动报警参数
typedef struct
{		
	unsigned long nLinkProbe;				//探头报警联动
	unsigned long nLinkProbeTime;			//探头报警联动输出的时间,单位为秒
	
	unsigned long nLinkRecord;				//录像联动,按位操作
	unsigned long nLinkRecordTime;			//联动录像的时间,单位为分钟
	
	unsigned long nLinkSnapshot;			//抓拍联动
	unsigned long nLinkSnapshotInterval;	//联动抓拍间隔, 单位为秒
	unsigned long nLinkSnapshotUploadFlag;	//联动抓拍上传启用
	unsigned long nLinkSnapshotUploadMode;	//联动抓拍上传方式：smtp、ftp、tftp、客户端接收
	
	TIME_SEGMENTS day[8];					//时间段参数
	
	unsigned long reserve;					//保留
	
}VIDEO_MOTION_ALARM_PARAM;

// 探头报警参数
typedef struct
{	
	//unsigned long nOnFlag;					//启用标志
 	unsigned long nLinkProbe;				//探头报警联动
	unsigned long nLinkProbeTime;			//探头报警联动输出的时间,单位为秒

	
	unsigned long nLinkRecord;				//录像联动,按位操作
	unsigned long nLinkRecordTime;			//联动录像的时间,单位为分钟
	
	unsigned long nLinkSnapshot;			//抓拍联动
	unsigned long nLinkSnapshotInterval;	//联动抓拍间隔, 单位为秒
	unsigned long nLinkSnapshotUploadFlag;	//联动抓拍上传启用
	unsigned long nLinkSnapshotUploadMode;	//联动抓拍上传方式：smtp、ftp、tftp、客户端接收
	
	unsigned char bPresetNo[32];			//对应通道的预置点调用
	
	TIME_SEGMENTS day[8];
	
	char probeName[32];
	
	unsigned long reserve;
	
}PROBE_IN_ALARM_PARAM;

 // 定时重启参数
 typedef struct
 {	
 	unsigned long bEnable;         //使能
     unsigned long nRebootInterval; //间隔（小时）
     unsigned long nRebootHour;  //小时
     unsigned long nRebootMinute; //分钟
     unsigned long dwReserved;    //保留
 }TIME_REBOOT_PARAM;


//当前报警信息
typedef struct 
{
	unsigned long nMotionStatus;//移动报警状态
	unsigned long nAlarmInStatus;//总控制
	unsigned long nIrAlarmStatus[12];//探头报警状态
	unsigned long nReserve[12];
}ALARM_STATUS_PARAM;



// 探头输出参数
typedef struct
{
	char probeName[32];
	TIME_SEGMENTS day[8];
	 
}PROBE_OUT_ALARM_PARAM;

//探头的状态, 用来获取、设置探头的状态，当要设置探头的状态，要先获取再设置，其中status每个位代表一个探头
typedef struct
{
	unsigned long status;
	
}PROBE_STATUS;

// 网络参数: 100 bytes
typedef struct
{ 
	unsigned long nDhcpOnFlag;
	char byServerIp[MAX_IP_LEN];	//服务器IP地址
	char byServerMask[MAX_IP_LEN];	//子网掩码
	char byGateway[MAX_IP_LEN];		//网关
	char byDnsAddr[MAX_IP_LEN];		//DNS地址
	char byMultiAddr[MAX_IP_LEN];	//组播地址
	unsigned short wServerPort;		//数据端口
	unsigned short wMultiPort;		//多播端口
	unsigned short wWebPort;		//Web端口
	unsigned char strPhyAddr[6];	//MAC地址
	unsigned long dwReserved;
	
}NET_PARAM;

//PPPOE
typedef struct
{
	unsigned long nOnFlag;	//启用标志
	char userName[32];
	char userPsw[32];
	
	unsigned long reserve;
	        
}PPPOE_PARAM;

  //P2P服务器参数
typedef struct tagDVSNET_P2P_PARAM
{
	unsigned long	bEnabled;									//是否启用P2P注册
	char			sServerUrl[64];								//注册的P2P服务器URL
	char			strUser[32];
	char			strPasswd[32];
	unsigned long	nPort;										//注册的P2P服务器端口
	unsigned long	nInterval;									//注册的时间间隔(秒)
	unsigned long	dwReserved;									//保留
}DVSNET_P2P_PARAM;

//DDNS
typedef struct
{
	unsigned long nOnFlag;	//启用标志
	char domain[128];
	char userName[32];
	char userPsw[32];	
	char ipAddr[32];
	unsigned long port;
	unsigned long authType;

	unsigned long reserve;	//
	        
}DDNS_PARAM;


//DDNS
typedef struct
{
	unsigned long nOnFlag;	//报警功能启用标志
	char sIpAddr[32];//报警服务器的地址
	unsigned long nPort;//报警服务器的端口
	unsigned long nTime;//报警时间间隔
	unsigned long nReserve;	//预留位
	        
}YIYUAN_ALARM_DDNS_PARAM;


typedef struct
{
	int nOnFlag;
	int nWebPort;
	int nCmdPort;
	int nWebPortStatus;
	int nCmdPortStatus;
	int nReserve;

}UPNP_PARAM;

/*
// 串口参数
typedef struct
{
	unsigned long nBaudRate;	//波特率
	unsigned long nDataBit;		//数据位
	unsigned long nParity;		//校验位
	unsigned long nStopBit;		//停止位
	
	unsigned long nReserve;

}COM_SETUP_PARAM;
*/

// 远程对讲IP
typedef struct
{
	char strTalkIP[16];				//服务器对讲IP
	unsigned long nPort;			//端口
	
	unsigned long nReserve;
	
}TALK_REQUEST_PARAM;

// 服务主动连接参数
typedef struct
{
	unsigned long nOnFlag;		//启用标志
	char strConnectURL[128];	//反向连接的服务器名称
	unsigned long nPort;		//反向连接的服务器端口
	unsigned long nInterval;	//反向连接的间隔时间
	
	unsigned long nReserve;
	
}REMOTE_CONNECT_PARAM;

// 服务主动连接参数
typedef struct
{
	unsigned long nOnFlag;		//启用标志
	char strConnectURL[64];		//反向连接的服务器名称
	char strUserName[32];
	char strPsw[32];
	unsigned long nPort;		//反向连接的服务器端口
	unsigned long nInterval;	//反向连接的间隔时间
	
	unsigned long nReserve;
}P2P_PARAM;

//P2P反向连接参数 
/*IPC<-----P2P Server 对应命令字:NETCMD_P2P_REQCONBACK*/
typedef struct tagDVSNET_REQCONBACK_INFO
{
	unsigned long nOnFlag;		//启用标志
	char strConnectURL[128];			// 反向连接的(PC端或转发服务器)的公网IP
	unsigned long	nPort;							// 反向连接的(PC端或转发服务器)的公网端口
	unsigned long	dwUserID;						// 反向连接的(PC端为用户ID，转发服务器为0xFFFFFFFF)用户ID
	unsigned long nInterval;	//反向连接的间隔时间

	unsigned long	nReserve;
}DVSNET_REQCONBACK_INFO;


/*IPC<---->P2P Server PC<---->P2P Server对应命令字：NETCMD_P2P_REGDATAPORT*/
typedef struct tagDVSNET_REGISTER_INFO
{
	char			strDeviceName[32];	//服务器名字	
	char			strSerialNo[48];	//IPC序列号
	char			strLocalIp[MAX_IP_LEN];			//本地IP地址 ,IPC或PC的本地IP
	char			strRemoteIp[MAX_IP_LEN];		//远程IP地址 ,注册时为P2P Server的IP，P2P Server返回时为IPC或PC的公网IP
	unsigned short	wLocalPort;						//本地端口 ,IPC或PC的本地端口	
	unsigned short  wRemotePort;					//远程端口,注册时为P2P Server的端口,P2P Server返回时为IPC或PC的公网端口
	unsigned long	dwDeviceType;					//设备类型	//0xFFFFFFFF为客户端
	unsigned long   dwUserID;						//第一次注册时，由P2P Server返回，做为判断的依据
	unsigned long   dwCount;						//标识为第几次连接 0-第一次，1-第二次
	unsigned long	dwNatType;						//NAT 类型 0-公网 1-克隆(全克隆或IP限制克隆可打洞) 2-端口限制克隆(可能可以打洞) 3-对称型(只能反向连接或转发)
	unsigned long	dwUpnp;							//是否已完成UPNP
	unsigned long	dwUpnpWebPort;					//Web
	unsigned long	dwUpnpDataPort;					//数据端口
	unsigned long	dwReserved;						//保留
	char			strUser[32];			//用户名
	char			strPasswd[32];
}DVSNET_REGISTER_INFO;


/*IPC----->P2P Server 对应命令字:NETCMD_P2P_ENDCONBACK */
typedef struct tagDVSNET_ENDCONBACK_INFO
{
	unsigned long	dwUserID;						// 反向连接的(PC端或转发服务器)的ID号
	unsigned long	dwReserved;
}DVSNET_ENDCONBACK_INFO;


/*IPC<-----P2P Server 对应命令字:NETCMD_P2P_REQHOLE*/
typedef struct tagDVSNET_REQHOLE_INFO
{
	char			strIp[MAX_IP_LEN];				// 请求打洞的PC端的公网IP
	unsigned long	dwPort;							// 请求打洞的PC端的公网端口
	unsigned long	dwUserID;						// 请求打洞的PC端的客户ID号
	unsigned long	dwReserved;						// 保留
}DVSNET_REQHOLE_INFO;


/*IPC----->P2P Server 对应命令字:NETCMD_P2P_ENDHOLE*/
typedef struct tagDVSNET_ENDHOLE_INFO
{
	unsigned long	dwUserID;						// 打洞请求的PC端的ID号
	unsigned long	dwReserved;
}DVSNET_ENDHOLE_INFO;

//p2p 收到数据确定命令
typedef struct tagDVSNET_CMDRECEIVED_INFO
{
	unsigned long	dwUserID;
	unsigned long	dwCmdReceived;
	unsigned long	dwReserved;
}DVSNET_CMDRECEIVED_INFO;

typedef struct tagRECORD_PARAM
{
	unsigned long nCoverMode;
	unsigned long nAudioFlag;

	unsigned long nReserve;
	
}RECORD_PARAM;


typedef struct tagUPNP_UPNP_PORT_INFO
{
	char			strLocalIp[16];			//本地IP地址 ,IPC或PC的本地IP
	char			strRemoteIp[16];		//远程IP地址 ,注册时为P2P Server的IP，P2P Server返回时为IPC或PC的公网IP
	char 			wLocalPort[8];						//本地端口 ,IPC或PC的本地端口	
	char    		wRemotePort[8];					//远程端口,注册时为P2P Server的端口,P2P Server返回时为IPC或PC的公网端口
}UPNP_PORT_INFO;

typedef struct tagUPNP_WIFI_IP_PORT_INFO
{
	char			strLocalIp[16];			//本地IP地址 ,IPC或PC的本地IP
	char			strWIFIIp[16];		// wifi IP地址
	unsigned long	bEnabled;			//是否启用
	unsigned long	upnpEthNo;			//网卡 0-有线 1-无线
	
	unsigned short  nDataPort;				
	unsigned short 	nWebPort;					
	unsigned short 	nRtspPort;	
	
	unsigned short	upnpWebStatus;		//upnp	WEB状态
	unsigned short 	upnpDataStatus;		//upnp	DATA状态
	unsigned short  upnpRtspStatus;		//upnp	rtsp状态

	unsigned long	nReserved;			//保留
	unsigned long	nOnFlag;			
}UPNP_WIFI_IP_PORT_INFO;


typedef struct tagUPNP_PORTMAPPING_INFO
{
	char			strRemoteIp[16];	
	int				nOnFlag;
	int 			nWebPortMappingStatus;
	int    			wRemoteWebPort;		
	int    			wRemoteMediaPort;	
	int    			wRemoteDataPort;		
	int    			wRemotePamerSetPort;	
	int    			wRemoteRecodFilePort;
	
}UPNP_PORTMAPPING_INFO;


   //UPNP参数
typedef struct tagDVSNET_UPNP_PARAM
{
	unsigned long	bEnabled;			//是否启用
	unsigned long	upnpEthNo;			//网卡 0-有线 1-无线
	unsigned short	upnpWebPort;		//upnp WEB端口
	unsigned short	upnpDataPort;		//upnp DATA端口
	unsigned short	upnpRtspPort;		//upnp rtsp端口
	
	unsigned long	upnpWebStatus;		//upnp	WEB状态
	unsigned long	upnpDataStatus;		//upnp	DATA状态
	unsigned long	upnpRtspStatus;		//upnp	rtsp状态
	
	unsigned long	nReserved;			//保留
}DVSNET_UPNP_PARAM;



typedef struct
{
	unsigned long nFlip;	// 0: 正常, 1：倒立
	
}VIDEO_FLIP_PARAM;

typedef struct
{
	unsigned long nMirror;	// 0: 正常, 1：镜像
	
}VIDEO_MIRROR_PARAM;

typedef struct
{
	unsigned long nHz;		// 0: 50HZ, 1: 60HZ
	
}VIDEO_HZ_PARAM;

typedef struct
{
	unsigned long nPath;	// 0: LINEIN, 1:MIC
	
}AUDIO_PATH_PARAM;

typedef struct
{
	TIME_SEGMENTS	day[8];							//报警时间 0-每天 1-星期天 ...
	unsigned long	nSnapshotInterval;							//抓拍间隔(秒)
	unsigned long	nSnapshotUploadMode;						//按位表示的上传方式
	unsigned long	dwReserved;									//保留
}SCHEDULE_SNAPSHOT_PARAM;

// Add the code by lvjh, 2009-08-24
typedef struct 
{
	unsigned char URL1[64];
	unsigned char URL2[64];
}P2PSERVER;

typedef struct
{
	unsigned char ID[48];
	P2PSERVER P2P;
	unsigned char MAC[6];
	unsigned char nReserve[130];
}DEVICE_CONFIGURE;

//多画面处理
#if 1
typedef struct tagMULTIDEV_INFO
{
	char			sDeviceAlias[32];	//别名(设备名称)
	char			sDeviceUrl[32];		//设备IP地址或域名
	unsigned short	wCmdPort;			//命令端口
	unsigned short  wDataPort;			//数据端口(目前命令端口和数据端口是一样的)
	char			sDeviceUser[32];	//登录设备用户名
	char			sDevicePwd[32];		//登录设备密码
	unsigned long	dwReserved;			//保留
}MULTIDEV_INFO;

typedef struct tagDVSNET_MULTIDEVALL_INFO
{
	MULTIDEV_INFO	nDeviceInfo[16];
	unsigned long	dwReserved;
}DVSNET_MULTIDEVALL_INFO;
#endif


//手动抓拍结果
typedef struct tagDVSNET_SNAPSHOT_RESULT
{
	unsigned long	nChannel;				//通道号
	unsigned long	nResult;				//0-成功，其它为错误码
	DATE_PARAM Time;			//时间
	unsigned long	nReserve;				//保留
}DVSNET_SNAPSHOT_RESULT;

//手动录像结果
typedef struct tagDVSNET_RECORD_RESULT
{
	unsigned long	nChannel;				//通道号
	unsigned long	nResult;				//0-成功，其它为错误码
	DATE_PARAM		Time;			//时间
	unsigned long	nReserve;				//保留
}DVSNET_RECORD_RESULT;



//系统参数，保存在文件里
typedef struct
{
	unsigned int paramFlag[32];
	
	// 系统信息
	SYS_INFO sysInfo;
	
	// 用户信息
	USER_INFO_PARAM userInfo;
	
	// 网络参数
	NET_PARAM network; 
	
	// Wlan
	WIFI_PARAM wlan;
	
	G3_PARAM g3;
	
	NTP_PARAM ntp;
		
	// 视频制式
	VIDEO_STANDARD_PARAM videoStandard[MAX_CHANNEL];
	
	// 视频属性
	VIDEO_IN_ATTR videoInAttr[MAX_CHANNEL];
	
	// 视频倒立
	VIDEO_FLIP_PARAM videoFlip[MAX_CHANNEL];
	
	// 视频镜像
	VIDEO_MIRROR_PARAM videoMirror[MAX_CHANNEL];
	
	// 视频HZ
	VIDEO_HZ_PARAM videoHz[MAX_CHANNEL];
	
	// 编码参数
	VENC_PARAM videoEnc[MAX_CHANNEL][2];	// 双码流
	
	// 水印参数
	VIDEO_WM_PARAM videoWM[MAX_CHANNEL];

	VIDEO_CHANNEL_NAME videoChnName[MAX_CHANNEL];
	
	// OSD参数
	OSD_PARAM osd[2];
		
	// LOGO参数
	LOGO_PARAM logo[MAX_CHANNEL];
	unsigned char logoData[MAX_LOGO_DATA];
		
	// MASK参数
	MASK_PARAM mask[MAX_CHANNEL];

	// MASK参数
	VIDEO_MASK mosaic[MAX_CHANNEL];
	
	// 视频移动帧测参数
	VIDEO_MOTION_PARAM motionDetect[MAX_CHANNEL];
	
	// 音频属性
	AUDIO_PARAM audioInAttr[MAX_CHANNEL];
	
	// 音频路径
	AUDIO_PATH_PARAM audioInPath[MAX_CHANNEL];
	
	AUDIO_PARAM audioOutAttr[MAX_CHANNEL];
	
	// 音频编码参数
	AENC_PARAM audioEnc[MAX_CHANNEL];
	
	// 报警参数
	VIDEO_LOST_ALARM_PARAM videoLostAlarm[MAX_CHANNEL];
	VIDEO_MOTION_ALARM_PARAM videoMotionAlarm[MAX_CHANNEL];
	PROBE_IN_ALARM_PARAM probeInAlarm[MAX_PROBE_IN];
	PROBE_OUT_ALARM_PARAM probeOutAlarm[MAX_PROBE_OUT];
	
	// FTP参数 
	FTP_PARAM ftp;
	
	//rtsp参数  
	DVSNET_RTSP_PARAM rtsp;

	
	// TFTP参数
	TFTP_PARAM tftp;
	
	// SMTP参数
	EMAIL_PARAM email;

	// JPEG存储IP
	char JpegStoreIP[MAX_IP_LEN]; 
	
	// PPPOE
	PPPOE_PARAM pppoe;
	
	// DDNS
	DDNS_PARAM ddns;

	// REMOTE CONNECT
	REMOTE_CONNECT_PARAM remoteConnectParam;
	
	
	// PTZ参数
	PTZ_PARAM ptz[MAX_CHANNEL];
	
	// 串口参数
	//COM_PARAM com[MAX_COM_NUM];
	COM_PARAM rs485[MAX_CHANNEL];	// changed by zhb, 2007-09-17
	COM_PARAM rs232;
		
	// 对讲IP
	char remoteTalkIP[MAX_IP_LEN];

	
	SCHEDULE_SNAPSHOT_PARAM snapshot[MAX_CHANNEL];
	
	PTZ_AUTO_CTRL ptzAutoCtrl[MAX_CHANNEL];

	//p2p end con back
	UPNP_PORT_INFO  upnpInfo;
	DVSNET_P2P_PARAM upnp;
	DVSNET_P2P_PARAM  p2p;
	DVSNET_REGISTER_INFO p2p_register;
	DVSNET_ENDCONBACK_INFO P2P_endconback;
	DVSNET_REQCONBACK_INFO  remoteP2PConnectParam;
	DVSNET_UPNP_PARAM newUpnpInfo;

#ifdef RECORD
	// RECORD PARAM
	RECORD_PARAM recordParam[MAX_CHANNEL];
	TIMER_RECORD_CHANNEL_PARAM timerRecordParam[MAX_CHANNEL];
	DVSNET_MULTIDEVALL_INFO multiDevAll;
	int irProbeAlarmFlags;
#endif
	//报警服务器
	YIYUAN_ALARM_DDNS_PARAM yiyuanAlarmDdnsParam;
	ALARM_STATUS_PARAM alarm_status;
	TIME_REBOOT_PARAM timerRebootParam;
	time_t 			  lastRebootTime;
}SYS_PARAM;

int generateMultiAddr(char *pMultiAddr);

int initSystemParam(int flag);
int deInitSystemParam();

int getDeviceInfo(SYS_INFO *param);
int setDeviceInfo(SYS_INFO *param);

int getSysInfoParam(SYS_INFO *param);
int setSysInfoParam(SYS_INFO *param);
int getNtpParam(NTP_PARAM *param);
int setNtpParam(NTP_PARAM *param);
int getUserInfoParam(USER_INFO_PARAM *param);
int setUserInfoParam(USER_INFO_PARAM *param);
int getVideoInStandardParam(int nChannel, VIDEO_STANDARD_PARAM *param);
int setVideoInStandardParam(int nChannel, VIDEO_STANDARD_PARAM *param);
int getVideoInAttrParam(int nChannel, VIDEO_IN_ATTR *param);
int setVideoInAttrParam(int nChannel, VIDEO_IN_ATTR *param);
int getVideoEncParam(int nChannel, int type, VENC_PARAM *param);
int setVideoEncParam(int nChannel, int type, VENC_PARAM *param);
int getVideoWMParam(int nChannel, VIDEO_WM_PARAM *param);
int setVideoWMParam(int nChannel, VIDEO_WM_PARAM *param);
int getVideoChnNameParam(int nChannel, VIDEO_CHANNEL_NAME *param);
int setVideoChnNameParam(int nChannel, VIDEO_CHANNEL_NAME *param);
int getOsdParam(int nChannel, OSD_PARAM *param);
int setOsdParam(int nChannel, OSD_PARAM *param);
int getLogoParam(int nChannel, LOGO_PARAM *param, char *data);
int setLogoParam(int nChannel, LOGO_PARAM *param, char *data);
int getMaskParam(int nChannel, MASK_PARAM *param);
int setMaskParam(int nChannel, MASK_PARAM *param);
int getMosaicParam(int nChannel, VIDEO_MASK *param);
int setMosaicParam(int nChannel, VIDEO_MASK *param);
int getVideoMotionParam(int nChannel, VIDEO_MOTION_PARAM *param);
int setVideoMotionParam(int nChannel, VIDEO_MOTION_PARAM *param);
int getAudioInAttrParam(int nChannel, AUDIO_PARAM *param);
int setAudioInAttrParam(int nChannel, AUDIO_PARAM *param);
int getAudioOutAttrParam(int nChannel, AUDIO_PARAM *param);
int setAudioOutAttrParam(int nChannel, AUDIO_PARAM *param);
int getAudioEncParam(int nChannel, AENC_PARAM *param);
int setAudioEncParam(int nChannel, AENC_PARAM *param);
int getVideoLostAlarmParam(int nChannel, VIDEO_LOST_ALARM_PARAM *param);
int setVideoLostAlarmParam(int nChannel, VIDEO_LOST_ALARM_PARAM *param);
int getVideoMotionAlarmParam(int nChannel, VIDEO_MOTION_ALARM_PARAM *param);
int setVideoMotionAlarmParam(int nChannel, VIDEO_MOTION_ALARM_PARAM *param);
int getProbeInAlarmParam(int nChannel, PROBE_IN_ALARM_PARAM *param);
int setProbeInAlarmParam(int nChannel, PROBE_IN_ALARM_PARAM *param);
int getProbeOutAlarmParam(int nChannel, PROBE_OUT_ALARM_PARAM *param);
int setProbeOutAlarmParam(int nChannel, PROBE_OUT_ALARM_PARAM *param);
int getFtpParam(FTP_PARAM *param);
int setFtpParam(FTP_PARAM *param);
//add code by lvjh
int getRtspParam(DVSNET_RTSP_PARAM *param);
int setRtspParam(DVSNET_RTSP_PARAM *param);


int setIrProbeAlarmStartParam(void);
int setIrProbeAlarmStopParam(void);
int getIrProberAlarmParam(void);

int getTftpParam(TFTP_PARAM *param);
int setTftpParam(TFTP_PARAM *param);
int getEmailParam(EMAIL_PARAM *param);
int setEmailParam(EMAIL_PARAM *param);
int getJpegIPParam(char *param);
int setJpegIPParam(char *param);
int getNetParam(NET_PARAM *param);
int setNetParam(NET_PARAM *param);
int getPPPOEParam(PPPOE_PARAM *param);
int setPPPOEParam(PPPOE_PARAM *param);
int getDDNSParam(DDNS_PARAM *param);
int setDDNSParam(DDNS_PARAM *param);
int getYIYUAN_DDNSParam(YIYUAN_ALARM_DDNS_PARAM *param);
int setYIYUAN_DDNSParam(YIYUAN_ALARM_DDNS_PARAM *param);
int getAlarmStatusParam(ALARM_STATUS_PARAM *param);
int setAlarmStatusParam(ALARM_STATUS_PARAM *param);
int getTimeRebootParam(TIME_REBOOT_PARAM *param);
int setTimeRebootParam(TIME_REBOOT_PARAM *param);


int getRemoteConnectParam(REMOTE_CONNECT_PARAM *param);
int setRemoteConnectParam(REMOTE_CONNECT_PARAM *param);
int setRemoteP2PConnectParam(DVSNET_REQCONBACK_INFO *param);
int getRemoteP2PConnectParam(DVSNET_REQCONBACK_INFO *param);
int setP2PBACKParam(DVSNET_ENDCONBACK_INFO *param);

int getPtzParam(int nChannel, PTZ_PARAM *param);
int setPtzParam(int nChannel, PTZ_PARAM *param);
int getRs485Param(int nChannel, COM_PARAM *param);
int setRs485Param(int nChannel, COM_PARAM *param);
int getRs232Param(int nChannel, COM_PARAM *param);
int setRs232Param(int nChannel, COM_PARAM *param);
int getRemoteTalkIPParam(char *param);
int setRemoteTalkIPParam(char *param);
int setTimeParam(DATE_PARAM param);
int getUpnpParam(UPNP_PORTMAPPING_INFO *param);
int setUpnpParam(UPNP_PORTMAPPING_INFO *param);
int getNewUpnpParam(DVSNET_UPNP_PARAM *param);
int setNewUpnpParam(DVSNET_UPNP_PARAM *param);
int getP2PParam(DVSNET_P2P_PARAM *param);
int setP2PParam(DVSNET_P2P_PARAM *param);
int setP2PRegisterParam(DVSNET_REGISTER_INFO* param);
int getWifiParam(WIFI_PARAM *param);
int setWifiParam(WIFI_PARAM *param);
int getVideoFlipParam(int nChannel, VIDEO_FLIP_PARAM *param);
int setVideoFlipParam(int nChannel, VIDEO_FLIP_PARAM *param);
int getVideoMirrorParam(int nChannel, VIDEO_MIRROR_PARAM *param);
int setVideoMirrorParam(int nChannel, VIDEO_MIRROR_PARAM *param);
int getVideoHzParam(int nChannel, VIDEO_HZ_PARAM *param);
int setVideoHzParam(int nChannel, VIDEO_HZ_PARAM *param);
int getAudioInPathParam(int nChannel, AUDIO_PATH_PARAM *param);
int setAudioInPathParam(int nChannel, AUDIO_PATH_PARAM *param);
int get3gParam(G3_PARAM *param);
int set3gParam(G3_PARAM *param);
int getScheduleSnapshotParam(int nChannel, SCHEDULE_SNAPSHOT_PARAM *param);
int setScheduleSnapshotParam(int nChannel, SCHEDULE_SNAPSHOT_PARAM *param);
int getPtzAutoCtrlParam(int nChannel, PTZ_AUTO_CTRL *param);
int setPtzAutoCtrlParam(int nChannel, PTZ_AUTO_CTRL *param);

int getParamFromFlash();

int saveParamToFlash();
int getParamStatusFromFlash();
int setParamStatusToFlash(int status);
int getBootStausFromFlash();
int setBootStatusToFlash(int status);

int saveParamToFile();
int getParamFromFile();
int getParamStatusFromFile();
int setParamStatusToFile(int status);
int getBootStausFromFile();
int setBootStatusToFile(int status);

// Add the code by lvjh, 2008-04-07
int setNetworkConfigure(int param);
int getNetworkConfigure();
int setDebugConfigure(int param);
int getDebugConfigure();
int setIDConfigure(char *param);
int getIDConfigure(char *param);
int setMACConfigure(char *param);
int getMACConfigure(char *param);
//add code by lvjh
int setRtspPortConfigure(int  param);
void writeConfigFile(char *filePath, int value, int defaultValue);
int setUSERConfigure(USER_INFO_PARAM *param);
int getUSERP2PPORTConfigure(UPNP_PORT_INFO *param);
int setP2PConfigure(P2PSERVER *P2P);
int getP2PConfigure(P2PSERVER *P2P);


#ifdef RECORD
int getRecordSDKParam(RECORD_SETUP *setup);
int getRecordParam(int nChannel, RECORD_PARAM *param);
int setRecordParam(int nChannel, RECORD_PARAM *param);
int getTimerRecordParam(int nChannel, TIMER_RECORD_CHANNEL_PARAM *param);
int setTimerRecordParam(int nChannel, TIMER_RECORD_CHANNEL_PARAM *param);
int getVideoMotionRecordParam(int nChannel, VIDEOMOTION_RECORD_CHANNEL_PARAM *param);
int getProbeRecordParam(int nChannel, DETECTOR_RECORD_CHANNEL_PARAM *param);
#define debugPrintf() {printf("FILE(%s) FUNC(%s):%d .\n",\
								__FILE__, __func__ , __LINE__); return;}
#endif

#endif

