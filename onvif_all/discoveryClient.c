#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdint.h>
#include "onvif.nsmap"
#include "soapH.h"
#include "soapStub.h" 

int main()
{
	struct soap *soap; 
	int result = 0; 
	struct d__ProbeType req;
	struct d__ProbeMatchesType resp;
	struct d__ScopesType sScope;
	struct SOAP_ENV__Header header;
	int count = 0;
//	char guid_string[100];
//	uuid_t uuid;
	soap = soap_new(); 
	if(soap == NULL)
	{
		printf("sopa new error\r\n");
		return -1;
	}

	/* discovery test */
//	uuid_generate(uuid);
//	uuid_unparse(uuid, guid_string);
	soap_set_namespaces(soap, namespaces); 

	//超过5秒钟没有数据就退出
	soap->recv_timeout = 5;
	soap_default_SOAP_ENV__Header(soap, &header);
#if 0
	header.wsa__MessageID = guid_string;
	header.wsa__To = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
	header.wsa__Action = "http://schemas.xmllocal_soap.org/ws/2005/04/discovery/Probe";
#endif
	soap->header = &header;
	soap_default_d__ScopesType(soap, &sScope);
	sScope.__item = "";
	soap_default_d__ProbeType(soap, &req);
	req.Scopes = &sScope;
	req.Types =NULL;

	int i = 0;
	do
	{
		soap_call___dndl__Probe(soap, "soap.udp://239.255.255.250:3702/datagram", NULL, &req, &resp); 
		printf("002.1 \n");

		if (soap->error) { 
			printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap)); 
			result = soap->error; 
			break;
		} 
		else
		{

			//printf("soap_call___dndl__Probe __sizeProbeMatch=%d\r\n",resp.__sizeProbeMatch);

//			while(1)
			{
				printf("Target EP Address       : %s\r\n", resp.ProbeMatch[i].wsa__EndpointReference.Address);
				printf("Target Type             : %s\r\n", resp.ProbeMatch[i].Types);
				printf("Target Service Address  : %s\r\n", resp.ProbeMatch[i].XAddrs);
				i++;
			}
			//printf("Target Metadata Version : %d\r\n", resp.ProbeMatch[0].ns2__MetadataVersion);
			//printf("Target Scopes Address   : %s\r\n", resp.ProbeMatch[0].ns2__Scopes->__item);
		}
	}while(0);
	soap_destroy(soap); // remove deserialized class instances (C++ only)
	soap_end(soap); // clean up and remove deserialized data

	////////////////////////////////////////////////////////////////////////////
failed:
	soap_free(soap);//detach and free runtime context
	soap_done(soap); // detach context (last use and no longer in scope)

	return result; 
}

