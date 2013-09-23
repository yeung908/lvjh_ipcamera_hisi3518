#include "onvif.h"
#include "onvifDiscovery.h"
#include "uuid.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#else
#include <WINSOCK2.H>
#include <WS2TCPIP.H>
#include <IO.H>
#include "test.h"
#endif
#include <errno.h>
#include "../param.h"
#include "discovery.nsmap"

const char *wsa_addr="http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
const char *wsa_action="http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
const char *scopes_string="onvif://www.onvif.org/type/video_encoder onvif://www.onvif.org/type/ptz onvif://www.onvif.org/type/audio_encoder onvif://www.onvif.org/location/city/china onvif://www.onvif.org/hardware/5201H onvif://www.onvif.org/name/IP_CAMERA";

Onvif_Server g_OnvifServer;
unsigned short g_wOnvifPort = 1239;
unsigned int g_OnvifDiscoveryRunning = 1;

int BuildProbeHeaderString(char *sBuffer,const char *suuid,struct Namespace *pNamespaces)
{
	char TmpBuffer[1024];
	char struid[100];
	uuid_t uid;
	BuildCommonHeaderString(sBuffer,pNamespaces);
	//start soap-env header
	strcat(sBuffer,"<SOAP-ENV:Header>");
	uuid_generate(uid);
	uuid_unparse(uid,struid);
	sprintf(TmpBuffer,"<wsa:MessageID>uuid:%s</wsa:MessageID>",struid);
	strcat(sBuffer,TmpBuffer);
//	uuid_generate(uid);
//	uuid_unparse(uid,struid);
	sprintf(TmpBuffer,"<wsa:RelatesTo>%s</wsa:RelatesTo>",suuid);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<wsa:ReplyTo SOAP-ENV:mustUnderstand=\"true\"><wsa:Address>%s</wsa:Address></wsa:ReplyTo>",wsa_addr);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<wsa:To SOAP-ENV:mustUnderstand=\"true\">%s</wsa:To>",wsa_addr);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<wsa:Action SOAP-ENV:mustUnderstand=\"true\">%s</wsa:Action>",wsa_action);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</SOAP-ENV:Header>");
	//end soap-env header	
	return strlen(sBuffer);	
}

int BuildNVTString(char *sBuffer,const char *suuid, struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	char struid[100];
	uuid_t uid;
	get_ip_addr(ETH_WIRE_DEV,localIP);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildProbeHeaderString(sBuffer+nLen,suuid,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<d:ProbeMatches><d:ProbeMatch>");
	uuid_generate(uid);
	uuid_unparse(uid,struid);
	sprintf(TmpBuffer,"<wsa:EndpointReference><wsa:Address>urn:uuid:%s</wsa:Address></wsa:EndpointReference>",struid);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<d:Types>dn:NetworkVideoTransmitter</d:Types>");
	sprintf(TmpBuffer,"<d:Scopes>%s</d:Scopes>",scopes_string);
	strcat(sBuffer,TmpBuffer);
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<d:XAddrs>http://%s/onvif/device_service</d:XAddrs>",localIP);
	else
		sprintf(TmpBuffer,"<d:XAddrs>http://%s:%d/onvif/device_service</d:XAddrs>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<d:MetadataVersion>%d</d:MetadataVersion>",124578);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</d:ProbeMatch></d:ProbeMatches></SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
	
}

int BuildDevString(char *sBuffer,const char *suuid, struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
//	char localIP[16];
//	get_ip_addr(ETH_WIRE_DEV,localIP);
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildProbeHeaderString(sBuffer+nLen,suuid,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<SOAP-ENV:Fault><SOAP-ENV:Code>");
	strcat(sBuffer,"<SOAP-ENV:Value>SOAP-ENV:Sender</SOAP-ENV:Value>");
	strcat(sBuffer,"<SOAP-ENV:Subcode>");
	strcat(sBuffer,"<SOAP-ENV:Value>d:MatchingRuleNotSupported</SOAP-ENV:Value>");
	strcat(sBuffer,"</SOAP-ENV:Subcode></SOAP-ENV:Code>");
	strcat(sBuffer,"<SOAP-ENV:Reason><SOAP-ENV:Text xml:lang=\"en\">the matching rule specified is not supported.</SOAP-ENV:Text></SOAP-ENV:Reason>");
	strcat(sBuffer,"</SOAP-ENV:Fault></SOAP-ENV:Body></SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int ONVIF_DiscoveryThread()
{
	int sock;
	fd_set fset;
	int ret;
	struct timeval to;
	struct ip_mreq mcast;
	struct sockaddr_in local_addr;
	struct sockaddr_in remote_addr;
	char buffer[10240];
	char strTag[1024];
	char strType[1024];
	char struuid[100];
	int sock_opt = 1;
	int nDiscovery,nType,nUUID;
	int len,nStrLen;
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock == -1)
	{
		printf("unable to create onvif discovery socket\n");
		pthread_exit(NULL);
		return -1;
	}
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void *)&sock_opt,sizeof(sock_opt)) == -1)
	{
		printf("set onvif discovery socket error\n");
		close(sock);
		pthread_exit(NULL);
		return -1;
	}
	memset(&local_addr,0,sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(3702);
	if(bind(sock,(struct sockaddr *)&local_addr,sizeof(local_addr)) == -1)
	{
		printf("bind onvif discovery socket error\n");
		close(sock);
		pthread_exit(NULL);
		return -1;
	}
	mcast.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	mcast.imr_interface.s_addr = htonl(INADDR_ANY);
	if(setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const char *)&mcast,sizeof(mcast)) == -1)
	{
		printf("add onvif discovery socket membership error\n");
		close(sock);
		pthread_exit(NULL);
		return -1;
	}
	while (g_OnvifDiscoveryRunning)
	{
		FD_ZERO(&fset);
		FD_SET(sock, &fset);
		to.tv_sec = 60;
		to.tv_usec = 0;

		ret = select(sock+1, &fset, NULL, NULL, &to);
		if ( ret == 0)
		{
			continue;
		}
		if (ret<0 && errno==EINTR)
		{
			continue;
		}
		if (ret < 0)
		{
			break;
		}

		if (!FD_ISSET(sock, &fset))
		{
			break;
		}
		if(!g_OnvifDiscoveryRunning)
			break;
		len = sizeof(remote_addr);
		ret = recvfrom(sock, buffer, 10240 - 1, 0, (struct sockaddr*)&remote_addr, &len);
		printf("Search onvif Client: %s(%d),recved:%d bytes\n", (char *)inet_ntoa(remote_addr.sin_addr),ntohs(remote_addr.sin_port),ret);
		if (ret >0)
		{
			buffer[ret] = 0;
			nDiscovery = XmlGetStringValue(buffer,"Probe",strTag,1024);
			nType = XmlGetStringValue(buffer,"d:Types",strType,1024);
			if(nType)
				nType = XmlGetStringValue(buffer,"Types",strType,1024);
			nUUID = XmlGetStringValue(buffer,"a:MessageID",struuid,100);
			if(nUUID)
				nUUID = XmlGetStringValue(buffer,"wsa:MessageID",struuid,100);
			if(nDiscovery || nType || nUUID)
			{
				printf("nDiscovery:%d,nType:%d,nUUID:%d\n",nDiscovery,nType,nUUID);
				continue;
			}
			nStrLen = 0;
			printf("::strType = %s\n ::struuid = %s\n", strType, struuid);
			if(XmlContainString(strType,"NetworkVideoTransmitter"))
			{
				printf("recved NetworkVideoTransmitter\n");
				nStrLen = BuildNVTString(buffer,struuid,discovery_namespaces);
			}
			else if(XmlContainString(strType,"Device"))
			{
				printf("recved Device\n");
				nStrLen = BuildDevString(buffer,struuid,discovery_namespaces);
			}
			printf("::strType = %s\n", strType);
			if(nStrLen > 0)
			{
				ret = sendto(sock, buffer, nStrLen, 0, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
			}
		}
	}
	close(sock);
	pthread_exit(NULL);
	return 0;
}

int ONVIF_DiscoveryStart()
{
	int ret;
	g_OnvifDiscoveryRunning = 1;
	ret = pthread_create(&g_OnvifServer.hDiscoveryThread, NULL, (void*)&ONVIF_DiscoveryThread, NULL);
	if (ret < 0)
	{
		printf("create onvif discovery thread error\n");
		return -1;
	}
	return 0;
}

void ONVIF_DiscoveryStop()
{
	g_OnvifDiscoveryRunning = 0;
	pthread_join(g_OnvifServer.hDiscoveryThread,NULL);
}
