#ifndef _G3_H_
#define _G3_H_

typedef struct
{
	unsigned long nOnFlag;
	unsigned long nType;			// 0: WCDMA, 1: EVDO, 2: TD-CDMA
	unsigned char pAPN[128];		// 
	unsigned char pDialNumber[32];
	unsigned char pAccount[32];
	unsigned char pPassword[32];
	unsigned char pWanIP[16];
	unsigned long nReserve;
	
}G3_PARAM;

int G3_Open();
int G3_Close();
int G3_Setup(G3_PARAM param);
int G3_GetSetup(G3_PARAM *param);
int G3_Start();
int G3_Stop();
int G3_Pause();
int G3_Resume();

#endif
