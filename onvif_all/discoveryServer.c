#include "onvif.nsmap"
#include "soapH.h"
#include "soapStub.h"



//////////////////////////////////////////////////////////////////////////////////////////////////////
//主函数
int main(int argc,char ** argv)
{
	struct soap ServerSoap;
	struct ip_mreq mcast;
	int count = 0;

	//初始话运行时环境
	//soap_init2(&ServerSoap, SOAP_IO_UDP | SOAP_IO_FLUSH | SOAP_IO_LENGTH, SOAP_IO_UDP | SOAP_IO_FLUSH | SOAP_IO_LENGTH);
	soap_init1(&ServerSoap, SOAP_IO_UDP | SOAP_XML_IGNORENS);
//	ServerSoap.version = 2;
//	soap_init(&ServerSoap);
//	ServerSoap.accept_timeout = 10;
//	ServerSoap.recv_timeout = 10;

	soap_set_namespaces(&ServerSoap, namespaces);

	if(!soap_valid_socket(soap_bind(&ServerSoap, NULL, 3702, 10)))
	{
		soap_print_fault(&ServerSoap, stderr);
		exit(1);
	}

	mcast.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
	mcast.imr_interface.s_addr = htonl(INADDR_ANY);

	if(setsockopt(ServerSoap.master, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mcast, sizeof(mcast)) < 0)
	{
		printf("setsockopt error! error code = %d,err string = %s\n",errno,strerror(errno));
		return 0;
	}

	for(;;)
	{
		if(soap_serve(&ServerSoap))
		{
			soap_print_fault(&ServerSoap, stderr);
		}

		soap_destroy(&ServerSoap);
		soap_end(&ServerSoap);

		//客户端的IP地址
		printf("Accepted count %d, connection from IP = %lu.%lu.%lu.%lu socket = %d \r\n",
				count, ((ServerSoap.ip)>>24)&0xFF, ((ServerSoap.ip)>>16)&0xFF, ((ServerSoap.ip)>>8)&0xFF, (ServerSoap.ip)&0xFF, (ServerSoap.socket));
		count++;
	}

	//分离运行时的环境
	soap_done(&ServerSoap);

	return 0;
}

