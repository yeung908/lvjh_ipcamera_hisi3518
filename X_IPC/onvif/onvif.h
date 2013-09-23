#ifndef _ONVIF_H_INCLUDED
#define _ONVIF_H_INCLUDED

#include "soapH.h"
#include <pthread.h>

#define SOAP_STOP		1000	/* No HTTP response */
#define SOAP_FORM		1001	/* Form request/response */
#define SOAP_HTML		1002	/* Custom HTML response */
#define SOAP_FILE		1003	/* Custom file-based response */

#define SOAP_IO				0x00000003	/* IO mask */
#define SOAP_IO_FLUSH		0x00000000	/* flush output immediately, no buffering */
#define SOAP_IO_BUFFER		0x00000001	/* buffer output in packets of size SOAP_BUFLEN */
#define SOAP_IO_STORE		0x00000002	/* store entire output to determine length for transport */
#define SOAP_IO_CHUNK		0x00000003	/* use HTTP chunked transfer AND buffer packets */

#  define SOAP_BLKLEN   (256)

#define SOAP_EOF			EOF
#define SOAP_OK				0
#define SOAP_TYPE			4
#define SOAP_GET_METHOD		15
#define SOAP_HDR			22

#define SOAP_ZLIB_NONE		0x00
#define SOAP_ZLIB_DEFLATE	0x01
#define SOAP_ZLIB_INFLATE	0x02
#define SOAP_ZLIB_GZIP		0x02

union soap_double_nan {struct {unsigned int n1, n2;} iv; double dv; float fv;};
extern const union soap_double_nan soap_double_nan;
extern const char soap_base64o[], soap_base64i[];

typedef struct tagOnvif_Server
{
	pthread_t hDiscoveryThread;
	pthread_t hServiceThread;
}Onvif_Server;

struct Http_Buffer
{
	char 	Buffer[65535];
	char	msgbuf[1024];
	char	endpoint[1024];
	char	path[1024];
	char 	tmpbuf[1024];
	char	action[1024];
	char	http_content[1024];
	char	userid[1024];
	char	passwd[1024];
	char 	proxy_from[1024];
	size_t  nBufLen;
	size_t	 nBufIndex;
	size_t	length;
	size_t	headerlen;
	int		status;
	int		error;
	int		zlib_in;
	int		zlib_out;
	int		imode;
	int		omode;
	int		keep_alive;
	int		nHasAction;
	int		nMsgOK;
	int		nHeaderOK;
};

int XmlGetStringValue(const char *str, const char *sName, char *sValue, int nLen);
int XmlContainString(const char *str,const char *sValue);
int HttpParseHeader(struct Http_Buffer *pBuf,const char *key,const char *val);
int HttpParse(struct Http_Buffer *pBuf);
int soap_getline(struct Http_Buffer *pBuf,char *value,size_t nLen);
int BuildCommonHeaderString(char *sBuffer,struct Namespace *pNamepaces);

#endif
