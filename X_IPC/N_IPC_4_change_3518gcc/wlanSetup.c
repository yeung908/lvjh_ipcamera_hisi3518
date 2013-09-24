#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "wlanSetup.h"

#define WIFI_TOOLS 		"/mnt/mtd/dvs/wlan/iwpriv"
#define WIFI_CONF 		"/mnt/mtd/dvs/wlan/iwconfig"
#define WPA_TOOLS 		"/mnt/mtd/dvs/wlan/wpa_supplicant"
#define WPA_CONF_FILE 	        "/mnt/mtd/dvs/wlan/wpa_supplicant.conf"

#ifdef WIFI_RALINK
#define	ETH_DEV_NAME 	       "ra0"
#define	ETH_DEV_IP 	       "192.168.1.52"
#endif
#ifdef WIFI_ATHEROS
#define	ETH_DEV_NAME 	       "eth2"
#endif

int wpa_setup(WIFI_PARAM param)
{
	FILE *fp = NULL;
	char buf[1024];
	int len = 0;
	char encryption[32];
	
	
	system("busybox rm -rf /mnt/mtd/dvs/wlan/wpa_supplicant.conf");
	
	fp = fopen(WPA_CONF_FILE, "w+b");
	if (fp == NULL)
	{
		printf("Can not open the file: %s.\n", WPA_CONF_FILE);

		return -1;
	}
	
	if (param.Encryption == 2)
	{
		strcpy(encryption, "TKIP");
	}
	else if (param.Encryption == 3)
	{
		strcpy(encryption, "AES");
	}
	else
	{
		memset(encryption, 0, 32);
	}
	
	len = sprintf(buf, "ctrl_interface=/var/run/wpa_supplicant\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "network={\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tssid=\"%s\"\n", param.SSID);
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tkey_mgmt=WPA-PSK\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tproto=WPA\n");
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tpairwise=%s\n", encryption);
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tgroup=%s\n", encryption);
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "\tpsk=\"%s\"\n", param.WPAPSK);
	fwrite(buf, 1, len, fp);
	len = sprintf(buf, "}\n");
	fwrite(buf, 1, len, fp);
		
	fclose(fp);
	
	sprintf(buf, "%s -B -irausb0 -c %s -Dralink", WPA_TOOLS, WPA_CONF_FILE);
	
	system(buf);
	
	return 0;
}

int wifi_setup_old(WIFI_PARAM param)
{
#ifdef WIFI_RALINK
	char buffer[256];
	char temp[40];
	char key64[12];
	char key128[28];
	
	printf("wifi_setup(RALINK): ...\n");

	// wake up wireless interface
	sprintf(buffer, "ifconfig %s %s up", ETH_DEV_NAME, ETH_DEV_IP);
	printf("ifconfig = %s\n..............\n", buffer);
	system(buffer);
	
	// setup network type
	if (param.Mode == 0)
	{
		sprintf(buffer, "%s %s mode managed", WIFI_CONF, ETH_DEV_NAME);
	}
	else if (param.Mode == 1)
	{
		sprintf(buffer, "%s %s mode ad-hoc", WIFI_CONF, ETH_DEV_NAME);
	}
	else
	{
		sprintf(buffer, "%s %s mode managed", WIFI_CONF, ETH_DEV_NAME);
	}
	system(buffer);
	printf("wifi_setup: %s\n", buffer);
	
	// setup essid
	sprintf(buffer, "%s %s essid \"%s\"", WIFI_CONF, ETH_DEV_NAME, param.SSID);
	system(buffer);
	printf("wifi_setup: %s\n", buffer);	
	
	// setup rate
	sprintf(buffer, "%s %s rate auto", WIFI_CONF, ETH_DEV_NAME);
	system(buffer);
	printf("wifi_setup: %s\n", buffer);	
	
	// WEP Setup
	if (param.Encryption == 1)
	{
		// Add the code by lvjh, 2009-05-31
		switch (param.WEPFormat)
		{
		case 0:
			memcpy(key64, param.WEPKey641, 10);
			sprintf(buffer, "%s %s key s:%s [%d]", WIFI_CONF, ETH_DEV_NAME, key64, param.WEPKeyChoose);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 1:
			memcpy(key128, param.WEPKey1281, 26);
			sprintf(buffer, "%s %s key s:%s [%d]", WIFI_CONF, ETH_DEV_NAME, key128, param.WEPKeyChoose);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 2:
			memcpy(key64, param.WEPKey641, 10);
			key64[10] = '\0';
			sprintf(buffer, "%s %s key %s [%d]", WIFI_CONF, ETH_DEV_NAME, key64, param.WEPKeyChoose);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 3:
			memcpy(key128, param.WEPKey1281, 26);
			key64[26] = '\0';
			sprintf(buffer, "%s %s key %s [%d]", WIFI_CONF, ETH_DEV_NAME, key128, param.WEPKeyChoose);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
		}
	}
	
	// WPA Setup
	if (param.Authen == 3 || param.Authen == 4)
	{
		wpa_setup(param);
	}
	
	return 0;
#endif
}

int wifi_setup(WIFI_PARAM param)
{
#ifdef WIFI_RALINK
	char buffer[256];
	char temp[40];
	char key[32];
	// wake up wireless interface
	memset(buffer, 0, 256);
	sprintf(buffer, "ifconfig %s up", ETH_DEV_NAME);
	system(buffer);
	
	// setup network type
	memset(buffer, 0, 256);
	if (param.Mode == 0)
	{
		sprintf(buffer, "%s %s mode managed", WIFI_CONF, ETH_DEV_NAME);
	}
	else if (param.Mode == 1)
	{
		sprintf(buffer, "%s %s mode ad-hoc", WIFI_CONF, ETH_DEV_NAME);
	}
	else
	{
		sprintf(buffer, "%s %s mode managed", WIFI_CONF, ETH_DEV_NAME);
	}
	system(buffer);
	
	// setup essid
	memset(buffer, 0, 256);
	sprintf(buffer, "%s %s essid \"%s\"", WIFI_CONF, ETH_DEV_NAME, param.SSID);
	system(buffer);
	
	// setup rate
	memset(buffer, 0, 256);
	sprintf(buffer, "%s %s rate auto", WIFI_CONF, ETH_DEV_NAME);
	system(buffer);
	
	switch (param.Encryption)
	{
	case 0:		// OFF 
		// Set AuthMode
		memset(buffer, 0, 256);
		switch (param.Authen)
		{
		case 0:	// Open
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		case 1:	// share
			sprintf(buffer, "%s %s set AuthMode=SHARED", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		case 2:	// BOTH
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		default:
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
		}
		system(buffer);
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s set EncrypType=NONE", WIFI_TOOLS, ETH_DEV_NAME);
		system(buffer);
		break;
		
	case 1:		// WEP
		// Set AuthMode
		memset(buffer, 0, 256);
		switch (param.Authen)
		{
		case 0:	// Open
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		case 1:	// share
			sprintf(buffer, "%s %s set AuthMode=SHARED", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		case 2:	// BOTH
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
			
		default:
			sprintf(buffer, "%s %s set AuthMode=OPEN", WIFI_TOOLS, ETH_DEV_NAME);
			break;
		}
		system(buffer);

		// Set EncrypType
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s set EncrypType=WEP", WIFI_TOOLS, ETH_DEV_NAME);
		system(buffer);

		// Set DefaultKeyID
		memset(buffer, 0, 256);
		memset(key, 0, 32);
		switch (param.WEPKeyChoose)
		{
		case 0:	
			sprintf(buffer, "%s %s set DefaultKeyID=1", WIFI_TOOLS, ETH_DEV_NAME);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			
			// Set Key
			memset(buffer, 0, 256);
			switch (param.WEPFormat)
			{
			case 0:
			case 2:
				memcpy(key, param.WEPKey641, 10);
				key[10] = '\0';
				break;
			
			case 1:
			case 3:
				memcpy(key, param.WEPKey1281, 26);
				key[26] = '\0';
				break;
			}
			sprintf(buffer, "%s %s set Key1=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 1:	
			sprintf(buffer, "%s %s set DefaultKeyID=2", WIFI_TOOLS, ETH_DEV_NAME);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			
			// Set Key
			memset(buffer, 0, 256);
			switch (param.WEPFormat)
			{
			case 0:
			case 2:
				memcpy(key, param.WEPKey642, 10);
				key[10] = '\0';
				break;
			
			case 1:
			case 3:
				memcpy(key, param.WEPKey1282, 26);
				key[26] = '\0';
				break;
			}
			sprintf(buffer, "%s %s set Key2=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 2:	
			sprintf(buffer, "%s %s set DefaultKeyID=3", WIFI_TOOLS, ETH_DEV_NAME);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			
			// Set Key
			memset(buffer, 0, 256);
			switch (param.WEPFormat)
			{
			case 0:
			case 2:
				memcpy(key, param.WEPKey643, 10);
				key[10] = '\0';
				break;
			
			case 1:
			case 3:
				memcpy(key, param.WEPKey1283, 26);
				key[26] = '\0';
				break;
			}
			sprintf(buffer, "%s %s set Key3=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			break;
			
		case 3:	
			sprintf(buffer, "%s %s set DefaultKeyID=4", WIFI_TOOLS, ETH_DEV_NAME);
			system(buffer);
			printf("wifi_setup: %s\n", buffer);
			
			// Set Key
			memset(buffer, 0, 256);
			switch (param.WEPFormat)
			{
			case 0:
			case 2:
				memcpy(key, param.WEPKey644, 10);
				key[10] = '\0';
				break;
			
			case 1:
			case 3:
				memcpy(key, param.WEPKey1284, 26);
				key[26] = '\0';
				break;
			}
			sprintf(buffer, "%s %s set Key4=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
			system(buffer);
			break;
			
		default:
			sprintf(buffer, "%s %s set DefaultKeyID=1", WIFI_TOOLS, ETH_DEV_NAME);
			system(buffer);
			
			// Set Key
			memset(buffer, 0, 256);
			switch (param.WEPFormat)
			{
			case 0:
			case 2:
				memcpy(key, param.WEPKey641, 10);
				key[10] = '\0';
				break;
			
			case 1:
			case 3:
				memcpy(key, param.WEPKey1281, 26);
				key[26] = '\0';
				break;
			}
			sprintf(buffer, "%s %s set Key1=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
			system(buffer);
			break;
		}
		break;
		
	case 2:		// WPAPSK
		// Set AuthMode
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s set AuthMode=WPAPSK", WIFI_TOOLS, ETH_DEV_NAME);
		system(buffer);
		
		// Set EncrypType
		memset(buffer, 0, 256);
		if (param.Authen == 4)	// AES
		{
			sprintf(buffer, "%s %s set EncrypType=AES", WIFI_TOOLS, ETH_DEV_NAME);
		}
		else
		{
			sprintf(buffer, "%s %s set EncrypType=TKIP", WIFI_TOOLS, ETH_DEV_NAME);
		}
		system(buffer);
		sleep(1);
		
		// Set Key
		memset(buffer, 0, 256);
		memset(key, 0, 32);
		memcpy(key, param.WPAPSK, 20);
		key[20] = '\0';
		sprintf(buffer, "%s %s set WPAPSK=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
		system(buffer);
		break;
		
	case 3:		// WPA2PSK
		// Set AuthMode
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s set AuthMode=WPA2PSK", WIFI_TOOLS, ETH_DEV_NAME);
		system(buffer);
		
		// Set EncrypType
		memset(buffer, 0, 256);
		if (param.Authen == 4)	// AES
		{
			sprintf(buffer, "%s %s set EncrypType=AES", WIFI_TOOLS, ETH_DEV_NAME);
		}
		else
		{
			sprintf(buffer, "%s %s set EncrypType=TKIP", WIFI_TOOLS, ETH_DEV_NAME);
		}
		system(buffer);
		printf("wifi_setup: %s\n", buffer);
		
		sleep(1);
		
		// Set Key
		memset(buffer, 0, 256);
		memset(key, 0, 32);
		memcpy(key, param.WPAPSK, 20);
		key[20] = '\0';
		sprintf(buffer, "%s %s set WPAPSK=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
		system(buffer);
		break;
		
	case 4:		// WPANONE - Ad-hoc Only
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s mode ad-hoc", WIFI_CONF, ETH_DEV_NAME);
		system(buffer);

		// Set AuthMode
		memset(buffer, 0, 256);
		sprintf(buffer, "%s %s set AuthMode=WPANONE", WIFI_TOOLS, ETH_DEV_NAME);
		system(buffer);

		// Set EncrypType
		memset(buffer, 0, 256);
		if (param.Authen == 4)	// AES
		{
			sprintf(buffer, "%s %s set EncrypType=AES", WIFI_TOOLS, ETH_DEV_NAME);
		}
		else
		{
			sprintf(buffer, "%s %s set EncrypType=TKIP", WIFI_TOOLS, ETH_DEV_NAME);
		}
		system(buffer);
		printf("wifi_setup: %s\n", buffer);
		
		sleep(1);
		memset(buffer, 0, 256);
		memset(key, 0, 32);
		memcpy(key, param.WPAPSK, 20);
		key[20] = '\0';
		sprintf(buffer, "%s %s set WPAPSK=%s", WIFI_TOOLS, ETH_DEV_NAME, key);
		system(buffer);
		break;
	}
	return 0;
#endif
}	

