#ifndef _NET_SETUP_H__
#define _NET_SETUP_H__

typedef struct
{
	unsigned long nOnFlag;
	
	///////////////////////////////////
	//  NTP Server:
	///////////////////////////////////
	//unsigned char Server[128];
	unsigned char Server[124];
	unsigned long nInterval;
	
	///////////////////////////////////
	//  TimeZone:
	//  0~25:(GMT-12)~GMT~(GMT+12)
	///////////////////////////////////
	unsigned long TimeZone;
	
	///////////////////////////////////
	//  Reserve for new parameter:
	///////////////////////////////////
	unsigned long Reserve;

}NTP_PARAM;

int ntp_setup(NTP_PARAM param);

#endif
