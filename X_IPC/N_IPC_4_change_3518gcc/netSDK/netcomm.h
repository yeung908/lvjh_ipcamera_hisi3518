//网络协议格式:
//	NET_HEADER+DATA;
//	1.登陆
//		C: NET_HEAD+USER_INFO;
//		S: NET_HEAD+SERVER_INFO/NULL(失败)
//	2.打开数据通道
//		C: NET_HEAD+OPEN_CHANNEL
//		S: NET_HEAD(成功、失败均会返回)
//	3.对讲请求
//		C: NET_HEAD;
//		S: NET_HEAD(成功、失败均会返回)
//	4.不支持的命令
//		C: NET_HEAD;
//		S: NET_HEAD(失败会返回)
//	5.网络心跳(每3秒一次)
//		C: NET_HEAD;
//		S: NET_HEAD;
//	6.数据多播请求
//		C: NET_HEAD;
//		S: NET_HEAD;
//	7.关闭数据通道
//		C: NET_HEAD;
//		S: NET_HEAD;
//	8.升级
//		C: NET_HEAD;
//		S: NET_HEAD;
//	9.TCP反向连接
//		C: NET_HEAD;
//		S: NET_HEAD;
//	10.TCP对讲反向请求
//		C: NET_HEAD;
//		S: NET_HEAD+TALK_PARAM;
//	11.发送数据包
//		S: NET_HEAD+DATA_PACKET;

#ifndef __NET_COMM_H_
#define __NET_COMM_H_

#define HDVS_FLAG						0x53564448
#define HSS_HDVS_FLAG					0X123AB678
#define HSS_HDVS_FLAG2					0X876CD321

#define NET_CMD_LINK					0x01
#define NET_CMD_PTZCTRL					0x06
#define NET_ERR_USEROVER				-103



 
#define MIN_SOCK_PORT					1024
#define MAX_SOCK_PORT					65535

#define USER_NAME_LEN					32	
#define USER_PSW_LEN					32

#define NETCMD_BUF_SIZE					2048

#define PACK_SIZE						1300
//#define NETCMD_MAX_SIZE					102400
#define NETCMD_MAX_SIZE					512000	// Change the code by lvjh, 2009-12-07
#define NETCMD_SNDFILE_SIZE				51200
#define NETCMD_TALKBACK_SIZE			51200

//网络命令
#define NETCMD_BASE						0x8000
#define NETCMD_LOGON					NETCMD_BASE+1
#define NETCMD_LOGOFF					NETCMD_BASE+2
#define NETCMD_GET_AV_INFO				NETCMD_BASE+3
#define NETCMD_OPEN_CHANNEL				NETCMD_BASE+4
#define NETCMD_CLOSE_CHANNEL			NETCMD_BASE+5
#define NETCMD_OPEN_MULTI_CHANNEL		NETCMD_BASE+6
#define NETCMD_CLOSE_MULTI_CHANNEL		NETCMD_BASE+7
#define NETCMD_OPEN_TALK				NETCMD_BASE+8
#define NETCMD_CLOSE_TALK				NETCMD_BASE+9
#define NETCMD_NAT_CONNECT				NETCMD_BASE+10
#define NETCMD_OPEN_CHANNEL_R			NETCMD_BASE+11
#define NETCMD_CLOSE_CHANNEL_R			NETCMD_BASE+12
#define NETCMD_OPEN_TALK_R				NETCMD_BASE+13
#define NETCMD_CLOSE_TALK_R				NETCMD_BASE+14
#define NETCMD_KEEP_ALIVE				NETCMD_BASE+15
#define NETCMD_UPDATE					NETCMD_BASE+16
#define NETCMD_SERVER_MSG				NETCMD_BASE+17		//服务器主动发送的消息
#define NETCMD_DEV_DATA					NETCMD_BASE+18
#define NETCMD_AV_DATA					NETCMD_BASE+19
#define NETCMD_TALK_DATA				NETCMD_BASE+20
#define NETCMD_REC_FILE					NETCMD_BASE+21
#define NETCMD_UPDATE_PROCESS			NETCMD_BASE+22

#define NETCMD_USER_BASE				NETCMD_BASE+0x500
#define NETCMD_USER_CMD					NETCMD_USER_BASE+0x01
#define NETCMD_P2P_CMD					(NETCMD_USER_BASE+2)
#define NETCMD_P2P_BASE					0x60000
#define NETCMD_P2P_REGDATAPORT			(NETCMD_P2P_BASE+1)
#define NETCMD_P2P_REQHOLE				(NETCMD_P2P_BASE+3)	//PC端请求IPC打洞 PC---->P2P Server----->IPC
#define NETCMD_P2P_ENDHOLE				(NETCMD_P2P_BASE+4) //打洞完成 IPC----->P2P Server----->PC
#define NETCMD_P2P_CMDRECEIVED			(NETCMD_P2P_BASE+301)	//确认收到命令


#define NETCMD_P2P_REQCONBACK			(NETCMD_P2P_BASE+5)
#define NETCMD_P2P_ENDCONBACK			(NETCMD_P2P_BASE+6)
#define GET_REMOTE_P2P_CONNECT_PARAM	(NETCMD_P2P_BASE+7)
#define NETERR_TALK_OPENED				 NETERR_BASE+9	//对讲已被其他用户开启


//用户消息

//网络错误代码
#define NETERR_BASE					0x9000
#define NETERR_NOT_USER				NETERR_BASE+1	//用户不存在
#define NETERR_USER_PSW				NETERR_BASE+2	//用户密码错误
#define NETERR_LOGON_FULL				NETERR_BASE+3	//用户登陆已满
#define NETERR_CHANNEL_FULL			NETERR_BASE+4	//通道数已满
#define NETERR_NOT_HDVS_DATA			NETERR_BASE+5	//不是HIDVS的数据包
#define NETERR_NOT_SUPPORT			NETERR_BASE+6	//不支持的命令
#define NETERR_ILLEGAL_PARAM			NETERR_BASE+7	//非法的命令参数
#define NETERR_NOT_LOGON				NETERR_BASE+8	//用户没有登陆
#define NETERR_TALK					NETERR_BASE+9	//用户没有登陆
#define NETERR_NOT_SUPPORT_PROTOCAL	NETERR_BASE+10	//不支持的网络协议
#define NETERR_NOT_RIGHT			NETERR_BASE+11	

//网络帧头
typedef struct
{
	unsigned long nFlag;			//标志
	unsigned long nCommand;		//命令
	unsigned long nErrorCode;		//错误码,成功只返回0
	unsigned long nBufSize;		//数据的大小
	unsigned long nReserve;		//保留
	
}NET_HEAD, *PNET_HEAD;


//和邦视讯 消息SOCKET数据头
typedef struct
{
	int   pMask1;		//MSGHEAD_MASK1
	char  sername[24];	    //服务器名称
	short nTypeMain;      //消息ID，根据不同ID号,后面的数据不同
	short nTypeSub;       //附加变量，返回时用于判断返回值.
	short nTypeThird;      //附加变量
	short nTypeFour;       //附加变量
	int   nChannel;       //通道
	int	  dwDataSize;    //数据长度，不包含数据头长度
	int   pMask2;		//MSGHEAD_MASK2
}NET_MSG_HEAD;

typedef struct
{
	char lpUserName[20];             //用户名
	char lpPassword[20];              //密码
}NETSDK_USER;

 typedef struct
{
	int server_sock;			           //设备返回的索引值
	int rights;					           //用户权限
}NETSDK_USERINFO;				   //用户连接后返回的结构信息




//命令头
typedef struct tagCOMMAND_HEAD
{
	unsigned long	nCmdLen;
	unsigned long	nCmdID;
	unsigned long	nChannel;
	unsigned long	nReserve;
}COMMAND_HEAD;



// 用户信息
typedef struct _USER_INFO
{
	char szUserName[USER_NAME_LEN];
	char szUserPsw[USER_PSW_LEN];
	char szServerName[USER_PSW_LEN];	// add the code by zhb, 2007-08-18
}NET_USER_INFO, *PNET_USER_INFO;

//网络数据
typedef struct
{
	NET_HEAD netHead;
	char pBuf[NETCMD_BUF_SIZE];
}NET_DATA, *PNET_DATA;			//内部使用

//网络协议的类型
typedef enum
{
	PROTOCOL_TCP = 0,
	PROTOCOL_UDP = 1,
	PROTOCOL_MULTI = 2
}PROTOCOL_TYPE;

//打开通道的参数
typedef struct
{
	unsigned long nID;
	PROTOCOL_TYPE nProtocolType;	//打开服务器通道的网络协议
	unsigned long nStreamType;		//用在双码流上，0：正常码，1：小码流
	unsigned long nSerChn;			//服务器通道
	unsigned long nClientID;		// add the code by zhb, 2007-08-18
	
}OPEN_CHANNEL, *POPEN_CHANNEL;

//AV信息
typedef struct
{
	unsigned long	nVideoEncType;		//视频编码类型
	unsigned long	nImageWidth;		//视频宽度
	unsigned long	nImageHeight;		//视频高度
	unsigned long	nVideoBitRate;		//视频比特率
	unsigned long	nFrameRate;			//视频帧率
	unsigned long	nAudioEncType;		//音频编码类型 0x55-MP3 0x25-G711 0x65-722 0x64-G726
	unsigned long	nAudioChannels;		//音频通道数
	unsigned long	nAudioSamples;		//音频采样率
	unsigned long	nAudioBitRate;		//音频比特率
	unsigned long	nReserve;			//保留，设置为0
    
}AV_INFO, *PAV_INFO;

//音视频
typedef struct
{
	unsigned long nTimeTick;		//时间戳
	unsigned long nVideoSize;		//视频数据大小
	unsigned short nAudioSize;		//音频数据大小
	unsigned short nImageWidth;		//视频长度
	unsigned short nImageHeight;	//视频宽度
	//unsigned short bKeyFrame;		//视频I(关键)帧标志
	// Add the code by lvjh, 2009-04-24
	unsigned char bKeyFrame;		//视频I(关键)帧标志
	unsigned char nReserve;			// unsigned char bKeyFrame;
    
}AV_FRAME_HEAD, *PAV_FRAME_HEAD;

//数据包
typedef struct
{
	unsigned char bIsDataHead;			//数据的头
	unsigned char nChannel;				//AV数据通道
	unsigned short nBufSize;			//数据的大小
	unsigned long byPacketID;			//AV包的序列号
	AV_FRAME_HEAD FrameHeader;			//AV帧头
	unsigned char PackData[PACK_SIZE];	//AV数据
}DATA_PACKET, *PDATA_PACKET;			//发送、接收时，仅发/收BufSize的PackData

//网络数据包头
typedef struct _NET_DATA_HEAD
{
	unsigned long nFlag;
	unsigned long nSize;
}NET_DATA_HEAD, *PNET_DATA_HEAD;

//网络数据包
typedef struct _NET_DATA_PACKET
{
	NET_DATA_HEAD packHead;
	DATA_PACKET packData;
}NET_DATA_PACKET, *PNET_DATA_PACKET;

//升级文件的数据包头
typedef struct _NET_UPDATE_FILE_HEAD
{
	char strFileName[32];
	char strFilePath[128];
	unsigned long nFileLen;
	unsigned long nFileOffset;
	unsigned long nBlockNum;
	unsigned long nBlockNo;
	unsigned long nBlockSize;
	unsigned long nFileCRC;
	unsigned long nReserve;
}NET_UPDATE_FILE_HEAD;

// 对讲参数
typedef struct _TALK_PARAM
{
	unsigned long nEncType;		//
	unsigned long nAinChnNum;	//
	unsigned long nAinBits;		//
	unsigned long nAinSamples;	//
	unsigned long nDecType;		//
	unsigned long nAoutChnNum;	//
	unsigned long nAoutBits;	//
	unsigned long nAoutSamples;	//
}TALK_PARAM;

#endif
