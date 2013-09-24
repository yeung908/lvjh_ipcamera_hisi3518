#include "onvif.h"
#include "uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*const char *xmlns_soapenv1 = "http://www.w3.org/2003/05/soap-envelope";
const char *xmlns_soapenv2 = "http://www.w3.org/2003/05/soap-encoding";
const char *xmlns_xsi="http://www.w3.org/2001/XMLSchema-instance";
const char *xmlns_xsd="http://www.w3.org/2001/XMLSchema";
const char *xmlns_wsa="http://schemas.xmlsoap.org/ws/2004/08/addressing";
const char *xmlns_wsa5="http://www.w3.org/2005/08/addressing";
const char *xmlns_d="http://schemas.xmlsoap.org/ws/2005/04/discovery";
const char *xmlns_dn="http://www.onvif.org/ver10/network/wsdl";*/

const union soap_double_nan soap_double_nan = {{0xFFFFFFFF, 0xFFFFFFFF}};
const char soap_base64o[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char soap_base64i[81] = "\76XXX\77\64\65\66\67\70\71\72\73\74\75XXXXXXX\00\01\02\03\04\05\06\07\10\11\12\13\14\15\16\17\20\21\22\23\24\25\26\27\30\31XXXXXX\32\33\34\35\36\37\40\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60\61\62\63";

static const char soap_padding[4] = "\0\0\0";
#define SOAP_STR_PADDING (soap_padding)
#define SOAP_STR_EOS (soap_padding)
#define SOAP_NON_NULL (soap_padding)

extern unsigned short g_wOnvifPort;

#define soap_blank(c)		((c) >= 0 && (c) <= 32)

int XmlGetStringValue(const char *str, const char *sName, char *sValue, int nLen)
{
	char strNameStart[64],strNameEnd[64];
	char *strStart,*strEnd;
	int  nValueLen;
	sprintf(strNameStart,"<%s",sName);
	sprintf(strNameEnd,"</%s>",sName);
	strStart = strstr(str,strNameStart);
	if(strStart)
		strStart = strstr(strStart + 1,">");
	strEnd = strstr(str,strNameEnd);
	if(NULL == strStart || NULL == strEnd)
		return -1;
	if(strEnd <= strStart)
		return -2;
	nValueLen = strEnd - strStart -1;
	if(nValueLen >= nLen)
		return -3;
	memcpy(sValue,strStart + 1,nValueLen);
	sValue[nValueLen] = '\0';
	return 0;
}


int XmlContainString(const char *str,const char *sValue)
{
	char *strPos = strstr(str,sValue);
	if(NULL == strPos)
		return 0;
	return 1;
}

int BuildCommonHeaderString(char *sBuffer,struct Namespace *pNamepaces)
{
	char TmpBuffer[1024];
	sprintf(sBuffer,"<SOAP-ENV:Envelope");
	while(pNamepaces)
	{
		if(pNamepaces->id == NULL)
			break;
		sprintf(TmpBuffer," xmlns:%s=\"%s\"",pNamepaces->id,pNamepaces->ns);
		strcat(sBuffer,TmpBuffer);
		pNamepaces ++;
	}

	strcat(sBuffer,">");
	return strlen(sBuffer);
}


int soap_tag_cmp(const char *s, const char *t)
{ 
	for (;;)
  	{ 
  		register int c1 = *s;
    	register int c2 = *t;
    	if (!c1 || c1 == '"')
     		 break;
    	if (c2 != '-')
    	{ 
    		if (c1 != c2)
      		{ 
      			if (c1 >= 'A' && c1 <= 'Z')
          			c1 += 'a' - 'A';
        		if (c2 >= 'A' && c2 <= 'Z')
          			c2 += 'a' - 'A';
      		}
      		if (c1 != c2)
      		{
      			if (c2 != '*')
          			return 1;
        		c2 = *++t;
        		if (!c2)
          			return 0;
        		if (c2 >= 'A' && c2 <= 'Z')
          			c2 += 'a' - 'A';
        		for (;;)
        		{ 
        			c1 = *s;
          			if (!c1 || c1 == '"')
            			break;
          			if (c1 >= 'A' && c1 <= 'Z')
            			c1 += 'a' - 'A';
          			if (c1 == c2 && !soap_tag_cmp(s + 1, t + 1))
            			return 0;
          			s++;
        		}
        		break;
      		}
    	}
   		 s++;
   		 t++;
  	}
  	if (*t == '*' && !t[1])
    	return 0;
  	return *t;
}

static const char*soap_decode(char *buf, size_t len, const char *val, const char *sep)
{ 
	const char *s;
  	char *t = buf;
  	for (s = val; *s; s++)
    	if (*s != ' ' && *s != '\t' && !strchr(sep, *s))
      		break;
  	if (*s == '"')
  	{ 
  		s++;
    	while (*s && *s != '"' && --len)
      		*t++ = *s++;
  	}
  	else
  	{ 
  		while (*s && !soap_blank(*s) && !strchr(sep, *s) && --len)
    	{ 
    		if (*s == '%')
      		{
      			*t++ = ((s[1] >= 'A' ? (s[1] & 0x7) + 9 : s[1] - '0') << 4) + (s[2] >= 'A' ? (s[2] & 0x7) + 9 : s[2] - '0');
        		s += 3;
      		}
      		else
        		*t++ = *s++;
    	}
  	}
  	*t = '\0';
  	while (*s && !strchr(sep, *s))
    	s++;
  	return s;
}

const char*soap_decode_key(char *buf, size_t len, const char *val)
{ 
	return soap_decode(buf, len, val, "=,;");
}

const char*soap_decode_val(char *buf, size_t len, const char *val)
{ 
	if (*val != '=')
  	{
  		*buf = '\0';
    	return val;
  	}
  	return soap_decode(buf, len, val + 1, ",;");
}

void soap_getcookies(struct Http_Buffer *pBuf,const char *val)
{
	
}

const char*soap_get_header_attribute(struct Http_Buffer *pBuf, const char *line, const char *key)
{ 
	register const char *s = line;
  	if (s)
  	{ 
  		while (*s)
    	{ 
    		register short flag;
      		s = soap_decode_key(pBuf->tmpbuf, sizeof(pBuf->tmpbuf), s);
      		flag = soap_tag_cmp(pBuf->tmpbuf, key);
      		s = soap_decode_val(pBuf->tmpbuf, sizeof(pBuf->tmpbuf), s);
      		if (!flag)
        		return pBuf->tmpbuf;
    	}
  	}
  	return NULL;
}

const char*soap_base642s(struct Http_Buffer*pBuf, const char *s, char *t, size_t l, int *n)
{ 
	register int i, j, c;
  	register unsigned long m;
  	register const char *p;
  	if (!s || !*s)
  	{ 
  		if (n)
      		*n = 0;
    	if (pBuf->error)
      		return NULL;
    	return SOAP_NON_NULL;
  	}
 // 	if (!t)
 // 	{ 
 // 		l = (strlen(s) + 3) / 4 * 3;
 //   	t = (char*)soap_malloc(soap, l);
 // 	}
 // 	if (!t)
 //   	return NULL;
  	p = t;
  	if (n)
    	*n = 0;
  	for (;;)
  	{ 
  		for (i = 0; i < SOAP_BLKLEN; i++)
    	{
    		m = 0;
      		j = 0;
      		while (j < 4)
      		{
      			c = *s++;
        		if (c == '=' || !c)
        		{
        			i *= 3;
          			switch (j)
          			{ 
          				case 2:
              				*t++ = (char)((m >> 4) & 0xFF);
             				 i++;
              			break;
            			case 3:
              				*t++ = (char)((m >> 10) & 0xFF);
              				*t++ = (char)((m >> 2) & 0xFF);
              				i += 2;
          			}
          			if (n)
            			*n += i;
          			return p;
        		}
        		c -= '+';
        		if (c >= 0 && c <= 79)
        		{ 
        			int b = soap_base64i[c];
          			if (b >= 64)
          			{ 
          				pBuf->error = SOAP_TYPE;
            			return NULL;
          			}
          			m = (m << 6) + b;
          			j++;
        		}
        		else if (!soap_blank(c + '+'))
        		{
        			pBuf->error = SOAP_TYPE;
          			return NULL;
        		}
      		}
      		*t++ = (char)((m >> 16) & 0xFF);
      		*t++ = (char)((m >> 8) & 0xFF);
      		*t++ = (char)(m & 0xFF);
      		if (l < 3)
      		{
      			if (n)
          			*n += i;
        		return p;
      		}
      		l -= 3;
   		}
    	if (n)
      		*n += 3 * SOAP_BLKLEN;
  	}
}

static int http_get(struct Http_Buffer *soap)
{ 
	return SOAP_GET_METHOD;
}

static int http_405(struct Http_Buffer *soap)
{ 
	return 405;
}

static int http_put(struct Http_Buffer *pBuf)
{
	return HttpParse(pBuf);
}


int HttpParseHeader(struct Http_Buffer *pBuf,const char *key,const char *val)
{
	char *action;
	if(!soap_tag_cmp(key,"Host"))
	{
		strcpy(pBuf->endpoint,"http://");
		strncat(pBuf->endpoint,val,sizeof(pBuf->endpoint) - 8);
		pBuf->endpoint[sizeof(pBuf->endpoint) - 1] = '\0';
	}
	else if(!soap_tag_cmp(key,"Content-Type"))
	{
		strncpy(pBuf->http_content,val,1024);
		action = (char *)soap_get_header_attribute(pBuf,val,"action");
		if(action)
		{
			if(*action == '"')
			{
				strncpy(pBuf->action,action + 1,1024);
				pBuf->action[strlen(pBuf->action) -1] = '\0';
			}
			else
				strncpy(pBuf->action,action,1024);
			pBuf->nHasAction = 1;
		}
	}
	else if(!soap_tag_cmp(key,"Content-Length"))
	{
		pBuf->length = strtoul(val,NULL,10);
	}
	else if(!soap_tag_cmp(key,"Content-Encoding"))
	{
		if(!soap_tag_cmp(val,"deflate"))
			pBuf->zlib_in = SOAP_ZLIB_DEFLATE;
		else if(!soap_tag_cmp(val,"gzip"))
			pBuf->zlib_in = SOAP_ZLIB_GZIP;
	}
	else if(!soap_tag_cmp(key,"Accept-Encoding"))
	{
		if(strchr(val,'*') || soap_get_header_attribute(pBuf,val,"gzip"))
			pBuf->zlib_out = SOAP_ZLIB_GZIP;
		else if(strchr(val,'*') || soap_get_header_attribute(pBuf,val,"deflate"))
			pBuf->zlib_out = SOAP_ZLIB_DEFLATE;
		else
			pBuf->zlib_out = SOAP_ZLIB_NONE;
	}
	else if(!soap_tag_cmp(key,"Transfer-Encoding"))
	{
		pBuf->imode &= ~SOAP_IO;
		if(!soap_tag_cmp(val,"chunked"))
			pBuf->imode |= SOAP_IO_CHUNK;
	}
	else if(!soap_tag_cmp(key,"Connection"))
	{
		if(!soap_tag_cmp(val,"keep-alive"))
			pBuf->keep_alive = -pBuf->keep_alive;
		else if(!soap_tag_cmp(val,"close"))
			pBuf->keep_alive = 0;
			
	}
	else if (!soap_tag_cmp(key, "Authorization"))
	{ 
		if (!soap_tag_cmp(val, "Basic *"))
	  	{
	  		int n;
			char *s;
			soap_base642s(pBuf, val + 6, pBuf->tmpbuf, sizeof(pBuf->tmpbuf) - 1, &n);
			pBuf->tmpbuf[n] = '\0';
			if ((s = strchr(pBuf->tmpbuf, ':')))
			{ 
				*s = '\0';
		  		strncpy(pBuf->userid , pBuf->tmpbuf,1024);
		  		strncpy(pBuf->passwd , s + 1,1024);
			}
	  	}
	}
	else if(!soap_tag_cmp(key,"SOAPAction"))
	{
		if (*val == '"')
    	{ 
    		strncpy(pBuf->action , val + 1, 1024);
      		pBuf->action[strlen(pBuf->action) - 1] = '\0';
    	}
    	else
      		strncpy(pBuf->action, val, 1024);
		pBuf->nHasAction = 1;
	}
	else if(!soap_tag_cmp(key,"Location"))
	{
		strncpy(pBuf->endpoint,val,sizeof(pBuf->endpoint));
		pBuf->endpoint[sizeof(pBuf->endpoint) - 1] = '\0';
	}
	else if(!soap_tag_cmp(key,"X-Forwarded-For"))
	{
		strncpy(pBuf->proxy_from,val,1024);
	}
	else if (!soap_tag_cmp(key, "Cookie")
	 || !soap_tag_cmp(key, "Cookie2")
	 || !soap_tag_cmp(key, "Set-Cookie")
	 || !soap_tag_cmp(key, "Set-Cookie2"))
	{ 
		soap_getcookies(pBuf, val);
	}
	return SOAP_OK;
}

int HttpParse(struct Http_Buffer *pBuf)
{
	char header[1024],*s;
	unsigned short httpcmd = 0,status = 0;
	if(!pBuf->nHeaderOK)
	{
		if(!pBuf->nMsgOK)
		{
			if (soap_getline(pBuf, pBuf->msgbuf, sizeof(pBuf->msgbuf)))
    		{
    			if (pBuf->error == SOAP_EOF)
        			return SOAP_EOF;
      			return pBuf->error = 414;
    		}
			pBuf->nMsgOK = 1;
		}
		while(1)
		{
			if(soap_getline(pBuf,header,1024))
			{
				return pBuf->error;
			}
			if(!*header)
			{
				pBuf->nHeaderOK = 1;
				pBuf->headerlen = pBuf->nBufIndex;
				break;
			}
			s = strchr(header,':');
			if(s)
			{
				char *t;
				*s = '\0';
				do
				{
					s++;
				}while(*s && *s <= 32);
				if(*s == '"')
					s++;
				t = s + strlen(s) - 1;
				while(t > s && *t <= 32)
					t--;
				if(t >= s && *t == '"')
					t--;
				t[1] = '\0';
				if((pBuf->error = HttpParseHeader(pBuf,header,s)))
				{
					if(pBuf->error < SOAP_STOP)
						return pBuf->error;
					status = pBuf->error;
					pBuf->error = SOAP_OK;
				}
			}
		}
	}
	if(!pBuf->nHeaderOK)
		return -1;
	if(pBuf->length)
	{
		if(pBuf->nBufLen - pBuf->headerlen < pBuf->length)
			return -2;
	}
	s = strstr(pBuf->msgbuf, "HTTP/");
	if (s && s[7] != '1')
	{ if (pBuf->keep_alive == 1)
		pBuf->keep_alive = 0;
	  if (pBuf->status == 0 && (pBuf->omode & SOAP_IO) == SOAP_IO_CHUNK) /* soap->status == 0 for HTTP request */
	  { pBuf->imode |= SOAP_IO_CHUNK;
		pBuf->omode = (pBuf->omode & ~SOAP_IO) | SOAP_IO_STORE;
	  }
	}
	if (pBuf->keep_alive < 0)
	  pBuf->keep_alive = 1;
	if (pBuf->status == 0)
	{ 
		size_t l = 0;
	  	if (s)
	  	{ 
	  		if (!strncmp(pBuf->msgbuf, "POST ", l = 5))
		  		httpcmd = 1;
			else if (!strncmp(pBuf->msgbuf, "PUT ", l = 4))
		  		httpcmd = 2;
			else if (!strncmp(pBuf->msgbuf, "GET ", l = 4))
		  		httpcmd = 3;
			else if (!strncmp(pBuf->msgbuf, "DELETE ", l = 7))
		  		httpcmd = 4;
			else if (!strncmp(pBuf->msgbuf, "OPTIONS ", l = 8))
		  		httpcmd = 5;
			else if (!strncmp(pBuf->msgbuf, "HEAD ", l = 5))
		  		httpcmd = 6;
	  	}
	  	if (s && httpcmd)
	  	{
	  		size_t m = strlen(pBuf->endpoint);
			size_t n = m + (s - pBuf->msgbuf) - l - 1;
			if (m > n)
		  		m = n;
			if (n >= sizeof(pBuf->endpoint))
		  		n = sizeof(pBuf->endpoint) - 1;
			strncpy(pBuf->path, pBuf->msgbuf + l, n - m);
				pBuf->path[n - m] = '\0';
			strcat(pBuf->endpoint, pBuf->path);
//			DBGLOG(TEST,SOAP_MESSAGE(fdebug, "Target endpoint='%s'\n", soap->endpoint));
			if (httpcmd > 1)
			{ 
//				DBGLOG(TEST,SOAP_MESSAGE(fdebug, "HTTP %s handler\n", soap->msgbuf));
		 	 	switch (httpcmd)
		  		{ 
		  			case  2: pBuf->error = http_put(pBuf);
						break;
					case  3: pBuf->error = http_get(pBuf);
						break;
					case  4: pBuf->error = http_405(pBuf);
						break;
					case  5: pBuf->error = http_405(pBuf);
						break;
					case  6: pBuf->error = http_405(pBuf);
						break;
					default: pBuf->error = 405;
						break;
		  		}
//		  		DBGLOG(TEST,SOAP_MESSAGE(fdebug, "HTTP handler return = %d\n", soap->error));
		  		if (pBuf->error == SOAP_OK)
					pBuf->error = SOAP_STOP; /* prevents further processing */
		  		return pBuf->error;
			}
			if (status)
		  		return pBuf->error = status;
	  	}
	  	else if (status)
			return pBuf->error = status;
	  	else if (s)
			return pBuf->error = 405;
	}
	/* Status OK (HTTP 200) */
	printf("Http parse OK,pBuf->nBufLen:%d,pBuf->nHeaderLen:%d,pBuf->length:%d,pBuf->action:%s\n",
		pBuf->nBufLen,pBuf->headerlen,pBuf->length,pBuf->action);
	if (pBuf->status == 0 || pBuf->status == 200)
	 	return SOAP_OK;
	return pBuf->error;
}

int soap_getline(struct Http_Buffer *pBuf,char *s,size_t len)
{
	int i = len;
  	char c = 0;
	int  nIndex = pBuf->nBufIndex;
  	for (;;)
  	{ 
  		while (--i > 0)
    	{ 
    		if(nIndex >= pBuf->nBufLen)
				return pBuf->error = SOAP_EOF;
    		c = pBuf->Buffer[nIndex];
			nIndex ++;
      		if (c == '\r' || c == '\n')
      		{
      			pBuf->nBufIndex = nIndex;
        		break;
      		}
      		*s++ = (char)c;
    	}
    	if (c != '\n')
    	{
    		if(pBuf->nBufIndex >= pBuf->nBufLen)
				c = EOF;
			else
			{
      			c = pBuf->Buffer[pBuf->nBufIndex]; /* got \r or something else, now get \n */
				pBuf->nBufIndex++;
			}
		}
    	if (c == '\n')
    	{ 
    		*s = '\0';
      		if (i+1 == len) /* empty line: end of HTTP/MIME header */
        		break;
    		if(pBuf->nBufIndex >= pBuf->nBufLen)
				c = EOF;
			else
      			c = pBuf->Buffer[pBuf->nBufIndex];
      		if (c != ' ' && c != '\t') /* HTTP line continuation? */
        		break;
    	}
    	else if ((int)c == EOF)
      		return pBuf->error = SOAP_EOF;
    	if (i < 0)
      		return pBuf->error = SOAP_HDR;
  	}
  	return SOAP_OK;
}
