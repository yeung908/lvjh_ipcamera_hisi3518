#ifndef __PTZ_H_
#define __PTZ_H_

#include "com.h"

#define MAX_PTZ_NUM			80
#define MAX_PTZ_NAME		32
#define MAX_CMD_SIZE		64
#define MAX_CMD_NUM			64


typedef enum
{
	
	PTZ_LEFT            =  0,     //nTypeThird: 速度码(1-10)
	PTZ_RIGHT           =  1,    // nTypeThird: 速度码(1-10)
	PTZ_UP              =  2,    // nTypeThird: 速度码(1-10)
	PTZ_DOWN            =  3,    // nTypeThird: 速度码(1-10)
	PTZ_IRISADD         =  4,    //nTypeThird: 速度码(1-10)
	PTZ_IRISDEC         =  5,    // nTypeThird: 速度码(1-10)
	PTZ_FOCUSADD        =  6,    // nTypeThird: 速度码(1-10)
	PTZ_FOCUSDEC        =  7,   //  nTypeThird: 速度码(1-10)
	PTZ_ZOOMADD         =  8,    // nTypeThird: 速度码(1-10)
	PTZ_ZOOMDEC         =  9,    // nTypeThird: 速度码(1-10)
	PTZ_GOTOPOINT       = 10,    // nTypeThird: 预置点号(1-255)
	PTZ_SETPOINT        = 11,    // nTypeThird: 预置点号(1-255)
	PTZ_AUTO            = 12,      // nTypeThird: 未定义
	PTZ_STOP            = 13,     // nTypeThird: 未定义
	PTZ_LEFTSTOP        = 14,     // nTypeThird: 未定义
	PTZ_RIGHTSTOP       = 15,     // nTypeThird: 未定义
	PTZ_UPSTOP	        = 16,     // nTypeThird: 未定义
	PTZ_DOWNSTOP        = 17,     // nTypeThird: 未定义
	PTZ_IRISADDSTOP     = 18,     // nTypeThird: 未定义
	PTZ_IRISDECSTOP     = 19,    //   nTypeThird: 未定义
	PTZ_FOCUSADDSTOP    = 20,     // nTypeThird: 未定义
	PTZ_FOCUSDECSTOP    = 21,     // nTypeThird: 未定义
	PTZ_ZOOMADDSTOP     = 22,     // nTypeThird: 未定义
	PTZ_ZOOMDECSTOP     = 23,     // nTypeThird: 未定义
	PTZ_DEVOPEN         = 30,    // nTypeThird: 辅助开关号(0-7)
	PTZ_DECCLOSE        = 31,    // nTypeThird: 辅助开关号(0-7)
	PTZ_AUTOSTOP        = 32,    // nTypeThird: 未定义
	PTZ_CLEARPOINT      = 33,    // nTypeThird: 未定义
}HOSS_PTZ_CTRL_CMD;




typedef enum
{
	UP_START 				= 0,
	UP_STOP 				= 1,
	DOWN_START				= 2,
	DOWN_STOP				= 3,
	LEFT_START				= 4,
	LEFT_STOP				= 5,
	RIGHT_START				= 6,
	RIGHT_STOP				= 7,
	ROTATION_START			= 8,
	ROTATION_STOP			= 9,
	
	AADD_START				= 10,	// Iris
	AADD_STOP				= 11,
	ADEC_START				= 12,	// Iris
	ADEC_STOP				= 13,
	
	FADD_START				= 14,	// Focus
	FADD_STOP				= 15,
	FDEC_START				= 16,	// Focus
	FDEC_STOP				= 17,
	
	DADD_START				= 18,	// Zoom
	DADD_STOP				= 19,
	DDEC_START				= 20,	// Zoom
	DDEC_STOP				= 21,
	
	LIGHT_START				= 22,	// Light
	LIGHT_STOP				= 23,
	
	RAIN_START				= 24,	// Rain Brush
	RAIN_STOP				= 25,
	
	TRACK_START				= 26,	// Track
	TRACK_STOP				= 27,
	
	IO_START				= 28,	// IO
	IO_STOP					= 29,
	
	SET_POINT				= 30,	// Point
	GOTO_POINT				= 31,
	CLEAR_POINT				= 32,
	
	BYPASS					= 33,	// Add the code by lvjh, 2011-01-11

   	LEFTUP_START			= 34,
	LEFTUP_STOP				= 35,
	LEFTDOWN_START			= 36,
	LEFTDOWN_STOP			= 37,
	RIGHTUP_START			= 38,
	RIGHTUP_STOP			= 39,
	RIGHTDOWN_START			= 40,
	RIGHTDOWN_STOP			= 41,
		
	VERT_ROTATION_START		= 42,
	VERT_ROTATION_STOP		= 43,
}PTZ_CTRL_CMD;

// PTZ列表
typedef struct
{
	unsigned long nNum;						//个数
	//unsigned long nChnIndex;				//当前通道的PTZ
	char strName[MAX_PTZ_NUM][MAX_PTZ_NAME];	//PTZ名字
	unsigned long nReserve;
	
}PTZ_INDEX;

//PTZ文件
typedef struct
{
	char strName[MAX_PTZ_NAME];		//PTZ名字
	unsigned long nSize;			//文件大小
	//char strData[5*1024];			//PTZ文件数据
}PTZ_DATA;	// 实际发送、接收这个结构时，数据大小应为：sizeof(PTZ_DATA)-5*1024+size

//PTZ参数
typedef struct
{
	char strName[MAX_PTZ_NAME];		//PTZ名字
	unsigned long nAddr;			//PTZ地址
	unsigned long nRate;			//PTZ速度
	
}PTZ_PARAM;

//PTZ控制命令
typedef struct
{
	unsigned long nCmd;
	unsigned long nValue;
	
}PTZ_CMD;

//透明传输用的
typedef struct ptzCtrlCmd
{
	unsigned long nCmdLen;
	char cmd[MAX_CMD_SIZE];
}PTZCTRLCMD;

typedef struct 
{
	char name[32];
	unsigned int addr;
	unsigned int rate;
	COM_PARAM param;
	PTZCTRLCMD ptzCmd[MAX_CMD_NUM];
	unsigned int checksum;
	unsigned int sum_start_bit;
	unsigned int sum_end_bit;
	unsigned int reserve;
		
}PTZCTRLINFO, *PPTZCTRLINFO;

typedef struct 
{
	COM_PARAM param;
	unsigned long nCmdLen;
	char cmd[MAX_CMD_SIZE];
}PTZCTRLCMD_THROUGH;

// Add the code by lvjh, 2009-09-16
typedef struct
{
	unsigned long bEnabled;			//是否启用巡航自动停止(PT机默认启用)
	unsigned long nAutoTimeLen;		//启动巡航后自动停止的时间(单位：秒）
	unsigned long dwReserved;		//保留，为零
}PTZ_AUTO_CTRL;

int setChnPtz(int nChannel, int index);
int getChnPtz(int nChannel, int *index);
int setChnPtzExt(int nChannel, char *ptzName, int addr, int rate);
int getChnPtzExt(int nChannel, char *ptzName, int *addr, int *rate);
int getPtzList(PTZ_INDEX *list);
int getPtzRs485Com(int nChannel, COM_PARAM *param);
int setPtzRs485Com(int nChannel, COM_PARAM param);
int getPtzRs232Com(int nChannel, COM_PARAM *param);
int setPtzRs232Com(int nChannel, COM_PARAM param);
int getPtzRate(int nChannel, int *param);
int addPtz(char *ptzName, char *fileData, int size);
int deletePtz(char *ptzName);
int ptzStart();
int ptzStop();
int ptzPause();
int ptzResume();
int ptzControl(int nChannel, PTZ_CMD cmd);

int setAutoPtz(int nChannel, PTZ_AUTO_CTRL param);
int getAutoPtz(int nChannel, PTZ_AUTO_CTRL *param);

// 透明传输
//int ptzControlThrough(int nChannel, int nComType, PTZCTRLINFO cmd);
int ptzControlThrough(int nChannel, int nComType, PTZCTRLCMD_THROUGH cmd);

#endif
 
