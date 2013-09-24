#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "soapH.h"
#include "soapStub.h"


static char szDevXAddr[1024];
static char szEndpointReference[1024];
static char tmpBuf[1024];
static struct d__ScopesType scopes;
#define METADATA_VERSION    1


#define ONVIF_SERVER_CALL()    printf("onvifs: call %s, path=%s\r\n", __FUNCTION__, soap->path)

#define ONVIF_RETURN_OK(soap, namespaces)   \
	ONVIF_SERVER_CALL();    \
	if(namespaces!=NULL) soap_set_namespaces(soap, namespaces);  \
	return SOAP_OK;



static inline int onvif_receiver_fault_subcode_oom(struct soap *soap)
{
	return soap_receiver_fault_subcode(soap, "ter:OutofMemory", "Out of Memory", "The device does not have sufficient memory to complete the action.");
}


/****************************** 具体服务方法 ******************************/
SOAP_FMAC5 int SOAP_FMAC6 __dndl__Probe(struct soap* soap, struct d__ProbeType *d__Probe, struct d__ProbeMatchesType *d__ProbeMatches)
{
	char MessageID[128] = {0};
	int MatchingRuleNotSupported = 0;

	printf("__dndl__Probe %s from %s\n", d__Probe->Types?d__Probe->Types:"Any types", inet_ntoa(soap->peer.sin_addr));

	if (soap->header)
	{
#if 0
		uuid_t uuid;

		uuid_generate(uuid);
		strcpy(MessageID, "urn:uuid:");
		uuid_unparse(uuid, MessageID+9);
#else
		unsigned char mac[6] = {0};
		netGetMac("eth0", mac);
		struct timeval tv, tv1;
		gettimeofday(&tv, NULL);
		usleep(1);
		gettimeofday(&tv1, NULL);
		sprintf(MessageID, "urn:uuid:%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			(int)tv.tv_usec, (int)(tv.tv_usec&0xFFFF), (int)(tv1.tv_usec&0xFFFF), (int)(tv.tv_usec&0xFF), (int)(tv1.tv_usec&0xFF), mac[0]&0xFF, mac[1]&0xFF, mac[2]&0xFF, mac[3]&0xFF, mac[4]&0xFF, mac[5]&0xFF);
#endif
		if(soap->header->wsa__MessageID)
		{
			printf("remote wsa__MessageID : %s\n",soap->header->wsa__MessageID);
			soap->header->wsa__RelatesTo = (struct wsa__Relationship*)soap_malloc(soap, sizeof(struct wsa__Relationship));
			if(soap->header->wsa__RelatesTo==NULL)
				return onvif_receiver_fault_subcode_oom(soap);
			soap_default__wsa__RelatesTo(soap, soap->header->wsa__RelatesTo);
			soap->header->wsa__RelatesTo->__item = soap->header->wsa__MessageID;

			soap->header->wsa__MessageID = soap_strdup(soap, MessageID);
			soap->header->wsa__To = (char*)"http://schemas.xmlsoap.org/ws/2004/08/addressing/role/anonymous";
			if(MatchingRuleNotSupported)
				soap->header->wsa__Action = (char*)"http://schemas.xmlsoap.org/ws/2005/04/discovery/fault";
			else
				soap->header->wsa__Action = (char*)"http://schemas.xmlsoap.org/ws/2005/04/discovery/ProbeMatches";
		}
	}


	d__ProbeMatches->__sizeProbeMatch = 1;
	d__ProbeMatches->ProbeMatch = (struct d__ProbeMatchType*)soap_malloc(soap, sizeof(struct d__ProbeMatchType));
	if(d__ProbeMatches->ProbeMatch)
	{
		unsigned int localIp = 0;
		netGetIp("eth3", &localIp);
		sprintf(szDevXAddr, "http://%s/onvif/device_service", inet_ntoa(*((struct in_addr *)&localIp)));

		soap_default_d__ScopesType(soap, &scopes);
		sprintf(tmpBuf, "%s", "onvif://www.onvif.org/type/video_encoder onvif://www.onvif.org/type/audio_encoder onvif://www.onvif.org/hardware/IPC-model onvif://www.onvif.org/name/IPC-model");
		scopes.__item = tmpBuf;

		soap_default_d__ProbeMatchType(soap, d__ProbeMatches->ProbeMatch);
		sprintf(szEndpointReference, MessageID);//"urn:uuid:11223344-5566-7788-99aa-000c29ebd542");
		d__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address = szEndpointReference;//"urn:uuid:464A4854-4656-5242-4530-313035394100";
		d__ProbeMatches->ProbeMatch->Types = (char*)"dn:NetworkVideoTransmitter";
		d__ProbeMatches->ProbeMatch->Scopes = &scopes;
		d__ProbeMatches->ProbeMatch->XAddrs = szDevXAddr;//"http://192.168.7.98/onvif/device_service";
		d__ProbeMatches->ProbeMatch->MetadataVersion = METADATA_VERSION;

		return SOAP_OK;
	}
	else
	{
		return onvif_receiver_fault_subcode_oom(soap);
	}
}


SOAP_FMAC5 int SOAP_FMAC6 __dnrd__Hello(struct soap* soap, struct d__HelloType *d__Hello, struct d__ResolveType *dn__HelloResponse)
{
	ONVIF_RETURN_OK(soap, NULL);
}


SOAP_FMAC5 int SOAP_FMAC6 __dnrd__Bye(struct soap* soap, struct d__ByeType *d__Bye, struct d__ResolveType *dn__ByeResponse)
{
	ONVIF_RETURN_OK(soap, NULL);
}



int netGetMac(char *pInterface, unsigned char *pMac)
{
	struct ifreq ifreq;
	int sockfd = 0;
	unsigned char mac[6] = {0}; 

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{/*创建套接口，后用于获取mac地址*/
		perror("netGetMac socket");
		return -1;
	}

	strcpy(ifreq.ifr_name, pInterface);

	if(ioctl(sockfd, SIOCGIFHWADDR, &ifreq) < 0)
	{/*获取mac地址*/
		perror("netGetMac ioctl");
		close(sockfd);
		return -2;
	}

	memcpy(mac, ifreq.ifr_hwaddr.sa_data, 6);/*复制mac地址到mac*/
	printf("MAC:%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	if(pMac != NULL)
	{
		memcpy(pMac, mac, 6);
	}

	close(sockfd);

	return 0;
}

/* 获取Ip */
int netGetIp(char *pInterface, unsigned int *ip)
{
	int sock = 0;
	struct ifreq ifr;

	if((pInterface == NULL) || (*pInterface == '\0'))
	{
		printf("get ip: pInterface == NULL\r\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, pInterface, IFNAMSIZ);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock <= 0)
	{
		printf("get ip: sock error, %s\r\n", strerror(errno));
		return -1;
	}

	((struct sockaddr_in*)&ifr.ifr_addr)->sin_family = PF_INET;
	if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
	{
		printf("get ip error: %s\r\n", strerror(errno));
		close(sock);
		return -1;
	}
	else
	{
		*ip = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr.s_addr;
		printf("get ip(%d:%d:%d:%d) success!\r\n", (*ip)&0xff, (*ip>>8)&0xff, (*ip>>16)&0xff, (*ip>>24)&0xff);
	}
	close(sock);

	return 0;
}

