#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>

//#define SMTP_DEBUG			1

#define READ_DELAY 			10000
#define SMTPTIMEOUT 		3

#define BASE64_MAXLINE  	76
#define EOL  				"\r\n"
#define MAXDATASIZE			1024

#define LOGIN				1
#define	PLAIN				2

char g_base64tab[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

int smtp_socket_fd = -1;

int smtp_socket_open()
{
	int fd = -1;

	if (smtp_socket_fd != -1)
	{
		shutdown(smtp_socket_fd, 2);
		close(smtp_socket_fd);
		smtp_socket_fd = -1;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
	{
		smtp_socket_fd = -1;
		return -1;
	}

	smtp_socket_fd = fd;

	return fd;
}

int smtp_socket_close()
{
	if (smtp_socket_fd != -1)
	{
		shutdown(smtp_socket_fd, 2);
		close(smtp_socket_fd);
		smtp_socket_fd = -1;

		return 0;
	}
	else
	{
		return -1;
	}
}

int smtp_socket_connect(char *pHostAddress, int port)
{
	int ret = -1;
	struct sockaddr_in addr;
	struct hostent *host = NULL;

	if (smtp_socket_fd<0 || pHostAddress==NULL || port<=0)
	{
		return -1;
	}

	if ((host=gethostbyname(pHostAddress)) == NULL)
	{
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(addr.sin_zero), 8);

	ret = connect(smtp_socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int smtp_socket_send(char *buffer, int size)
{
	int ret = -1;

	if (smtp_socket_fd < 0)
	{
		return -1;
	}
	if (buffer == NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}
#ifdef SMTP_DEBUG
	printf("<S>: %s\n", buffer);
#endif
	ret = send(smtp_socket_fd, buffer, size, 0);
	if (ret <= 0)
	{
		return -1;
	}

	return ret;
}

int smtp_socket_recv(char *buffer, int size)
{
	int ret = -1;

	if (smtp_socket_fd<0 || buffer==NULL)
	{
		return -1;
	}
	if (size <= 0)
	{
		return -1;
	}

	ret = recv(smtp_socket_fd, buffer, size, 0);
	if (ret <= 0)
	{
		return -1;
	}
#ifdef SMTP_DEBUG
	printf("<R>: %s\n", buffer);
#endif
	return ret;
}

typedef struct __SMTP_MESSAGE
{

	char name[32];
	char From[64];
	char Subject[128];
	char XMailer[64];
	char ReplyTo[64];

	char toList[64];
	char ccList[64];
	char bccList[64];
	char attaList[64];

	char *Body;
} SMTP_MESSAGE;

int encode_base64(unsigned char *in, unsigned char *out, int inlen)
{
	if (in==NULL || out==NULL || inlen<=0)
	{
		return -1;
	}

	for (; inlen >= 3; inlen -= 3)
	{
		*out++ = g_base64tab[in[0] >> 2];
		*out++ = g_base64tab[((in[0] << 4) & 0x30) | (in[1] >> 4)];
		*out++ = g_base64tab[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
		*out++ = g_base64tab[in[2] & 0x3f];
		in += 3;
	}

	if (inlen > 0)
	{
		unsigned char fragment;

		*out++ = g_base64tab[in[0] >> 2];
		fragment = (in[0] << 4) & 0x30;

		if (inlen > 1)
			fragment |= in[1] >> 4;

		*out++ = g_base64tab[fragment];
		*out++ = (inlen < 2) ? '=' : g_base64tab[(in[1] << 2) & 0x3c];
		*out++ = '=';
	}

	*out = '\0';

	return 0;
}

int smtp_connect(char *pSmtpServer)
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (pSmtpServer == NULL)
	{
		return -1;
	}

	ret = smtp_socket_open();
	if (ret < 0)
	{
		return -1;
	}

	ret = smtp_socket_connect(pSmtpServer, 25);
	if (ret < 0)
	{
		smtp_socket_close();
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		smtp_socket_close();
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 220)
	{
		smtp_socket_close();
		return -1;
	}

	return 0;
}

int smtp_ehlo(char *pSmtpServer)
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (pSmtpServer == NULL)
	{
		return -1;
	}
	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "EHLO ");
	strcat(buf, pSmtpServer);
	strcat(buf, "\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 250)
	{
		return -1;
	}

	return 0;
}

int smtp_auth(char *pUser, char *pPsw, int mode)
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];
	char pEncUser[128];
	char pEncPsw[128];

	if (pUser==NULL || pPsw==NULL)
	{
		return -1;
	}
	if (mode<LOGIN || mode>PLAIN)
	{
		return -1;
	}
	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	switch (mode)
	{
	case LOGIN:
		// User Name
		ret = encode_base64(pUser, pEncUser, strlen(pUser));
		if (ret < 0)
		{
			break;
		}

		strcpy(buf, "AUTH LOGIN ");
		strcat(buf, pEncUser);
		strcat(buf, "\r\n");

		ret = smtp_socket_send(buf, strlen(buf));
		if (ret < 0)
		{
			break;
		}

		usleep(READ_DELAY);

		memset(buf, 0, MAXDATASIZE);

		ret = smtp_socket_recv(buf, MAXDATASIZE);
		if (ret < 0)
		{
			break;
		}

		sscanf(buf, "%d", &code);

		if (code != 334)
		{
			break;
		}

		// Password
		ret = encode_base64(pPsw, pEncPsw, strlen(pPsw));
		if (ret < 0)
		{
			break;
		}

		strcpy(buf, pEncPsw);
		strcat(buf, "\r\n");

		ret = smtp_socket_send(buf, strlen(buf));
		if (ret < 0)
		{
			break;
		}

		usleep(READ_DELAY);

		memset(buf, 0, MAXDATASIZE);

		ret = smtp_socket_recv(buf, MAXDATASIZE);
		if (ret < 0)
		{
			break;
		}

		sscanf(buf, "%d", &code);

		if (code != 235)
		{
			break;
		}

		ret = 0;

		break;

	case PLAIN:
	{
		// User Name/Password
		int i = 0;
		int len = 0;
		char temp[MAXDATASIZE];
		char encode[MAXDATASIZE];

		memset(buf, 0, MAXDATASIZE);
		memset(temp, 0, MAXDATASIZE);
		memset(encode, 0, MAXDATASIZE);

		strcpy(temp, "^");
		strcat(temp, pUser);
		strcat(temp, "^");
		strcat(temp, pPsw);

		len = strlen(temp);

		for (i=len-1; i>=0; i--)
		{
			if (temp[i]=='^')
			{
				temp[i]='\0';
			}
		}

		ret = encode_base64(temp, encode, strlen(temp));
		if (ret < 0)
		{
			break;
		}

		strcpy(buf, "AUTH PLAIN ");
		strcat(buf, encode);
		strcat(buf, "\r\n");

		ret = smtp_socket_send(buf, strlen(buf));
		if (ret < 0)
		{
			break;
		}

		usleep(READ_DELAY);

		memset(buf, 0, MAXDATASIZE);

		ret = smtp_socket_recv(buf, MAXDATASIZE);
		if (ret < 0)
		{
			break;
		}

		sscanf(buf, "%d", &code);

		if (code != 235)
		{
			break;
		}

		ret = 0;
	}
	break;

	default:
		break;
	}

	return ret;
}

int smtp_mail(char *pFrom)
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (pFrom ==NULL)
	{
		return -1;
	}
	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "MAIL FROM:<");
	strcat(buf, pFrom);
	strcat(buf, ">\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}


	sscanf(buf, "%d", &code);

	if (code != 250)
	{
		return -1;
	}

	return 0;
}

int smtp_rcpt(char *pTo)
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (pTo ==NULL)
	{
		return -1;
	}
	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "RCPT TO:<");
	strcat(buf, pTo);
	strcat(buf, ">\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 250)
	{
		return -1;
	}

	return 0;
}

int smtp_data()
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "DATA\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 354)
	{
		return -1;
	}

	return 0;
}

int smtp_header(char *pFrom, char *pTo, char *pCC, char *pBCC, char *pSubject, int nAttach)
{
	int ret = -1;
	char header[2048];


	if (smtp_socket_fd < 0)
	{
		return -1;
	}
	if (pFrom==NULL || pTo==NULL || pSubject==NULL)
	{
		return -1;
	}

	ret = getheader(pFrom, pTo, pCC, pBCC, pSubject, nAttach, header);
	if (ret < 0)
	{
		return -1;
	}

	//Send the header
	ret = smtp_socket_send(header, strlen(header));
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int smtp_mime_start()
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	//Send the Mime Header for the body
	strcpy(buf, "\r\n--#BOUNDARY#\r\nContent-Type: text/plain; charset=gb2312\r\nContent-Transfer-Encoding: quoted-printable\r\n\r\n");
	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int smtp_mime_end()
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	//Send the Mime Header for the body
	strcpy(buf, "\r\n--#BOUNDARY#--");
	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int smtp_body(char *body)
{
	int ret = -1;

	if (smtp_socket_fd < 0)
	{
		return -1;
	}
	if (body == NULL)
	{
		return -1;
	}

	//Send the body
	ret = smtp_socket_send(body, strlen(body));
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int smtp_attach(char *attach, char *attach_name, int size)
{
	int ret = -1;
	int encodesize = 0;
	char buf[MAXDATASIZE];
	char *temp = NULL;

	if (smtp_socket_fd < 0)
	{
		return -1;
	}
	if (attach==NULL || attach_name==NULL || size<=0)
	{
		return -1;
	}

	encodesize = (size+2)/3*4;
	encodesize += (strlen(EOL)*encodesize/BASE64_MAXLINE + 3);
	encodesize += 10240;	// Add the code by lvjh, 2009-08-07

	temp = (char *)malloc(encodesize);
	if (temp == NULL)
	{
		return -1;
	}

	//First send the Mime header for each attachment
	sprintf(buf, "\r\n\r\n--#BOUNDARY#\r\nContent-Type: application/octet-stream; name=%s\r\nContent-Transfer-Encoding: base64\r\nContent-Disposition: attachment; filename=%s\r\n\r\n",
	        attach_name, attach_name);
	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	encode_base64(attach, temp, size);

	//Then send the encoded attachment
	ret = smtp_socket_send(temp, strlen(temp));
	if (ret < 0)
	{
		free(temp);
		temp = NULL;
		return -1;
	}

	free(temp);
	temp = NULL;

	return 0;
}

int separate_email(char *email, char *name)
{
	char *temp = NULL;

	if (email==NULL || name==NULL)
	{
		return -1;
	}

	temp = strchr(email, '@');
	if (temp == NULL)
	{
		return -1;
	}

	strncpy(name, email, temp-email);
	name[temp-email] = '\0';

	return 0;
}

int smtp_mail_address_convert(char *name, char *email, char *smtp_mail)
{
	if (name==NULL || email==NULL || smtp_mail==NULL)
	{
		return -1;
	}

	sprintf(smtp_mail, "%s <%s>", name, email);

	return 0;
}

int getheader(char *pFrom, char *pTo, char *pCC, char *pBCC, char *pSubject, int nAttach, char *header)
{
	time_t timep;
	struct tm *p;
	char time_buf[64];
	char to_name[32];
	char cc_name[32];
	char bcc_name[32];
	char to[512];
	char cc[512];
	char bcc[512];

	char pBase64_Subject[128] = {0};
	char pGB2312_Subject[256] = {0};


	if (header==NULL || pTo==NULL || pFrom==NULL)
	{
		return -1;
	}

	//Create the "Date:" part of the header
	time(&timep);
	p = gmtime(&timep);
	sprintf(time_buf, "%s, %02d %s %04d %02d:%02d:%02d ",
	        week[p->tm_wday], p->tm_mday, month[p->tm_mon], p->tm_year + 1900,
	        p->tm_hour, p->tm_min, p->tm_sec);

	//Create the "to:" part of the header
	memset(to, 0, 512);
	separate_email(pTo, to_name);
	smtp_mail_address_convert(to_name, pTo, to);
	memset(cc, 0, 512);
	if (pCC)
	{
		separate_email(pCC, cc_name);
		smtp_mail_address_convert(cc_name, pCC, cc);
	}
	memset(bcc, 0, 512);
	if (pBCC)
	{
		separate_email(pBCC, bcc_name);
		smtp_mail_address_convert(bcc_name, pBCC, bcc);
	}
	encode_base64(pSubject,pBase64_Subject , strlen(pSubject));

	sprintf(pGB2312_Subject, "%s%s%s", "=?gb2312?B?", pBase64_Subject,"?=");
	//printf("pGB2312_Subject = %s pBase64_Subject = %s pSubject = %s\n", pGB2312_Subject,pBase64_Subject, pSubject);

	//Stick everything together
	if (strlen(cc))
	{
		sprintf(header, "From: %s\r\nTo: %s\r\nCc: %s\r\nSubject: %s\r\nDate: %s\r\nX-Mailer: %s\r\n",
		        pFrom, to, cc, pGB2312_Subject, time_buf, "TDS Mailer");
	}
	else
	{
		sprintf(header, "From: %s\r\nTo: %s\r\nSubject: %s\r\nDate: %s\r\nX-Mailer: %s\r\n",
		        pFrom, to, pGB2312_Subject, time_buf, "TDS Mailer");
	}

	//Add the optional fields if attachments are included
	if (nAttach)
	{
		strcat(header, "MIME-Version: 1.0\r\nContent-type: multipart/mixed;boundary=\"#BOUNDARY#\"\r\n");
	}
	else
	{
		// avoid long textual message being automatically converted by the server:
		strcat(header, "MIME-Version: 1.0\r\nContent-type: text/plain; charset=US-ASCII\r\n");
	}

	strcat(header, "\r\n");

	//printf("header = %s %s\n",header, __func__);

	return 0;
}

int smtp_content()
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "\r\n.\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 250)
	{
		return -1;
	}

	return 0;
}

int smtp_quit()
{
	int ret = -1;
	int code = 0;
	char buf[MAXDATASIZE];

	if (smtp_socket_fd < 0)
	{
		return -1;
	}

	strcpy(buf, "QUIT\r\n");

	ret = smtp_socket_send(buf, strlen(buf));
	if (ret < 0)
	{
		return -1;
	}

	usleep(READ_DELAY);

	memset(buf, 0, MAXDATASIZE);

	ret = smtp_socket_recv(buf, MAXDATASIZE);
	if (ret < 0)
	{
		return -1;
	}

	sscanf(buf, "%d", &code);

	if (code != 221)
	{
		return -1;
	}

	return 0;
}

int SMTP_Send(char *emailServer, char *username, char *password,
              char *from, char *to, char *cc, char *bcc, char *replyto, char *subject, char *body, char *attach_filename)
{
	return 0;
}

int SMTP_Send_Ext(char *emailServer, char *username, char *password,
                  char *from, char *to, char *cc, char *bcc, char *replyto,
                  char *subject, char *body, char *filename, char *attach_buffer, int size)
{
	int ret = 0;
	int nAttach = 0;
	char *pTo = NULL;
	char *pCc = NULL;
	char *pBcc = NULL;

	if (emailServer==NULL || username==NULL || password==NULL || from==NULL || to==NULL || subject==NULL || body==NULL)
	{
		return -1;
	}
	if (strlen(to) <= 0)
	{
		return -1;
	}
	// Add the code by lvjh, 2010-06-02
	if (strlen(emailServer)==0 || strlen(username)==0)
	{
		return -1;
	}

	//printf("%s %s\n", from, to);

	ret = smtp_connect(emailServer);
	if (ret < 0)
	{
		printf("smtp_connect(%s): failed!\n", emailServer);
		return -1;
	}

	ret = smtp_ehlo(emailServer);
	if (ret < 0)
	{
		printf("smtp_ehlo(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_auth(username, password, LOGIN);
	if (ret < 0)
	{
		ret = smtp_auth(username, password, PLAIN);
		if (ret < 0)
		{
			printf("smtp_auth(): failed!\n");
			smtp_socket_close();
			return -1;
		}
	}

	ret = smtp_mail(from);
	if (ret < 0)
	{
		printf("smtp_mail(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_rcpt(to);
	if (ret < 0)
	{
		printf("smtp_rcpt(to: %s): failed!\n", to);
		smtp_socket_close();
		return -1;
	}

	// Add the code by lvjh, 2008-02-29
	if (strlen(cc)>3 && strstr(cc, "@"))
	{
		pCc = cc;
	}
	else
	{
		pCc = NULL;
	}

	//if (cc != NULL)
	if (pCc != NULL)
	{
		ret = smtp_rcpt(cc);
		if (ret < 0)
		{
			printf("smtp_rcpt(cc: %s): failed!\n", cc);
			smtp_socket_close();
			return -1;
		}
	}

	// Add the code by lvjh, 2008-02-29
	if (strlen(bcc)>3 && strstr(bcc, "@"))
	{
		pBcc = bcc;
	}
	else
	{
		pBcc = NULL;
	}

	//if (bcc != NULL)
	if (pBcc != NULL)
	{
		ret = smtp_rcpt(bcc);
		if (ret < 0)
		{
			printf("smtp_rcpt(bcc: %s): failed!\n", bcc);
			smtp_socket_close();
			return -1;
		}
	}

	ret = smtp_data();
	if (ret < 0)
	{
		printf("smtp_data(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	if (filename==NULL || attach_buffer==NULL || size<=0)
	{
		nAttach = 0;
	}
	else
	{
		nAttach = 1;
	}
	//ret = smtp_header(from, to, cc, bcc, subject, nAttach);
	ret = smtp_header(from, to, pCc, pBcc, subject, nAttach);
	if (ret < 0)
	{
		printf("smtp_header(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_mime_start();
	if (ret < 0)
	{
		printf("smtp_mime_start(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_body(body);
	if (ret < 0)
	{
		printf("smtp_body(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	if (nAttach)
	{
		ret = smtp_attach(attach_buffer, filename, size);
		if (ret < 0)
		{
			printf("smtp_attach(): failed!\n");
			smtp_socket_close();
			return -1;
		}
	}

	ret = smtp_mime_end();
	if (ret < 0)
	{
		printf("smtp_mime_end(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_content();
	if (ret < 0)
	{
		printf("smtp_content(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	ret = smtp_quit();
	if (ret < 0)
	{
		printf("smtp_quit(): failed!\n");
		smtp_socket_close();
		return -1;
	}

	return 0;
}


