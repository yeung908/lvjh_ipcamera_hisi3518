#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "simpleftp.h"
#include "../param.h"

typedef struct ftp_host_info_s 
{
	char user[32];
	char password[32];
	struct sockaddr_in s_in;
} ftp_host_info_t;

static char verbose_flag = 0;
static char do_continue = 0;

// Add the code by lvjh, 2009-04-02
static int g_ftp_socket = 0;

static int ftpcmd(const char *s1, const char *s2, FILE *stream, char *buf)
{
	if (s1) 
	{
		if (s2) 
		{
			fprintf(stream, "%s%s\r\n", s1, s2);
		} 
		else 
		{
			fprintf(stream, "%s\r\n", s1);
		}
	}
	
	do 
	{
		char *buf_ptr;

		if (fgets(buf, 510, stream) == NULL) 
		{	
			return -1;		
		}
		buf_ptr = strstr(buf, "\r\n");
		if (buf_ptr) 
		{
			*buf_ptr = '\0';
		}
	} while (! isdigit(buf[0]) || buf[3] != ' ');

	return atoi(buf);
}

static int connect_ftpdata(ftp_host_info_t *server, const char *buf)
{
	int ret = -1;
	char *buf_ptr = NULL;
	int socket_fd = -1;
	unsigned short port_num;

	buf_ptr = strrchr(buf, ',');
	*buf_ptr = '\0';
	port_num = atoi(buf_ptr + 1);

	buf_ptr = strrchr(buf, ',');
	*buf_ptr = '\0';
	port_num += atoi(buf_ptr + 1) * 256;

	server->s_in.sin_port=htons(port_num);
	
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		printf("socket() Failed!\n");
		return -1;
	}
	
	ret = connect(socket_fd, (struct sockaddr *)&server->s_in, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		printf("connect() Failed!\n");
		return -1;
	}	
	
	return socket_fd;
}

static FILE *ftp_login(ftp_host_info_t *server)
{
	int ret = -1;
	int socket_fd = -1;
	FILE *control_stream = NULL;
	char buf[512];

	/* Connect to the command socket */
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		return NULL;
	}	
	ret = connect(socket_fd, (struct sockaddr *)&server->s_in, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		// Add the code by lvjh, 2009-04-02
		close(socket_fd);
		
		return NULL;
	}	
	control_stream = fdopen(socket_fd, "r+");
	if (control_stream == NULL) 
	{
		// Add the code by lvjh, 2009-04-02
		close(socket_fd);
		
		return NULL;
	}

	if (ftpcmd(NULL, NULL, control_stream, buf) != 220) 
	{
		// Add the code by lvjh, 2009-04-02
		fclose(control_stream);
		close(socket_fd);
		
		return NULL;
	}

	/*  Login to the server */
	switch (ftpcmd("USER ", server->user, control_stream, buf)) 
	{
	case 230:
		// Add the code by lvjh, 2009-04-02
		fclose(control_stream);
		close(socket_fd);
		
		return NULL;		
		
	case 331:
		if (ftpcmd("PASS ", server->password, control_stream, buf) != 230) 
		{
			// Add the code by lvjh, 2009-04-02
			fclose(control_stream);
			close(socket_fd);
		
			return NULL;
		}
		break;
		
	default:
		// Add the code by lvjh, 2009-04-02
		fclose(control_stream);
		close(socket_fd);
		
		return NULL;		
	}

	ftpcmd("TYPE I", NULL, control_stream, buf);

	// Add the code by lvjh, 2009-04-02
	g_ftp_socket = socket_fd;
		
	return(control_stream);
}

static size_t copyfd_size(int src_fd, int dst_fd, const size_t size)
{
	size_t read_total = 0;
	char buffer[4096];

	while ((size == 0) || (read_total < size)) 
	{
		size_t read_try;
		ssize_t read_actual;

 		if ((size == 0) || (size - read_total > 4096)) 
 		{
			read_try = 4096;
		} 
		else 
		{
			read_try = size - read_total;
		}

		read_actual = read(src_fd, buffer, read_try);
		//printf("read(%d)\n", read_actual);
		if (read_actual > 0) 
		{
			if ((dst_fd >= 0) && (write(dst_fd, buffer, (size_t) read_actual) != read_actual)) 
			{			
				break;
			}
		}
		else if (read_actual == 0) 
		{
			if (size) 
			{				
			}
			break;
		} 
		else 
		{
			break;
		}

		read_total += read_actual;
	}
	
	return(read_total);
}

int safe_strtoul(char *arg, unsigned long* value)
{
	char *endptr;
	int errno_save = errno;

	errno = 0;
	*value = strtoul(arg, &endptr, 0);
	if (errno != 0 || *endptr!='\0' || endptr==arg) 
	{
		return 1;
	}
	errno = errno_save;
	return 0;
}

static int ftp_recieve(ftp_host_info_t *server, FILE *control_stream, char *local_path, char *server_path)
{
	int ret = -1;
	char buf[512];
	off_t filesize = 0;
	int fd_data = -1;
	int fd_local = -1;
	off_t beg_range = 0;

	/* Connect to the data socket */
	if (ftpcmd("PASV", NULL, control_stream, buf) != 227) 
	{
		printf("ftpcmd(PASV) Failed!\n");
		return -1;
	}
	
	fd_data = connect_ftpdata(server, buf);
	if (fd_data == -1)
	{
		printf("connect_ftpdata(%s) Failed!\n", buf);
		return -1;
	}

	if (ftpcmd("SIZE ", server_path, control_stream, buf) == 213) 
	{
		unsigned long value=filesize;
		//if (strtoul(buf + 4, &value, 10))
		if (safe_strtoul(buf + 4, &value))
		{			
			close(fd_data);
			return -1;
		}
		filesize = value;
	}

	if ((local_path[0] == '-') && (local_path[1] == '\0')) 
	{
		fd_local = STDOUT_FILENO;
		do_continue = 0;
	}

	if (do_continue) 
	{
		struct stat sbuf;
		if (lstat(local_path, &sbuf) < 0) 
		{
			close(fd_data);
			return -1;
		}
		if (sbuf.st_size > 0) 
		{
			beg_range = sbuf.st_size;
		} 
		else 
		{
			do_continue = 0;
		}
	}

	if (do_continue) 
	{
		sprintf(buf, "REST %ld", (long)beg_range);
		if (ftpcmd(buf, NULL, control_stream, buf) != 350) 
		{
			do_continue = 0;
		} 
		else 
		{
			filesize -= beg_range;
		}
	}

	if (ftpcmd("RETR ", server_path, control_stream, buf) > 150) 
	{
		close(fd_data);
		return -1;
	}
	
	/* only make a local file if we know that one exists on the remote server */
	if (fd_local == -1) 
	{
		if (do_continue) 
		{
			fd_local = open(local_path, O_APPEND | O_WRONLY, 0777);
		} 
		else 
		{
			fd_local = open(local_path, O_CREAT | O_TRUNC | O_WRONLY, 0777);
		}
	}

	if (fd_local < 0)
	{
		close(fd_data);
		return -1;
	}
	
	// Copy the file
	if (copyfd_size(fd_data, fd_local, filesize) == -1) 
	{		
		close(fd_data);
		close(fd_local);
		return -1;
	}	

	// close it all down
	close(fd_data);
	close(fd_local);
	
	if (ftpcmd(NULL, NULL, control_stream, buf) != 226) 
	{
		return -1;
	}
	
	ftpcmd("QUIT", NULL, control_stream, buf);

	return 0;
}

static int ftp_send(ftp_host_info_t *server, FILE *control_stream, char *local_path, char *server_path)
{
	int ret = -1;
	off_t filesize = 0;
	struct stat sbuf;
	char buf[512];
	int fd_data = -1;
	int fd_local = -1;
	int response = 0;
	FTP_PARAM ftp_param;

	// Connect to the data socket
	if (ftpcmd("PASV", NULL, control_stream, buf) != 227) 
	{
		return -1;
	}

	fd_data = connect_ftpdata(server, buf);
	if (fd_data < 0)
	{
		return -1;
	}
	
	//if (ftpcmd("CWD ", server_path, control_stream, buf) != 250) // [zhb][delete][2006-09-18]
	//改变ftp 工作目录
	ret = getFtpParam(&ftp_param);
	if (ret < 0)
	{
		return -1;
	}

	if (ftpcmd("CWD ", ftp_param.strPath, control_stream, buf) != 250) 
	{
		printf("%s\n", buf);
		close(fd_data);
		return -1;
	}
	
	// get the local file
	if ((local_path[0] == '-') && (local_path[1] == '\0')) 
	{
		fd_local = STDIN_FILENO;
	} 

	else 
	{		
		fd_local = open(local_path, O_RDONLY);
		fstat(fd_local, &sbuf);

		sprintf(buf, "ALLO %lu", (unsigned long)sbuf.st_size);
		filesize = sbuf.st_size;
		response = ftpcmd(buf, NULL, control_stream, buf);
		
		switch (response) 
		{
		case 200:
		case 202:
			break;
		default:
			//close(fd_local); // [zhb][delete][2006-09-18]
			printf("ALLO error(%s)!\n", buf);
			break;
		}
	}
	response = ftpcmd("STOR ", local_path, control_stream, buf);
	switch (response) 
	{
	case 125:
	case 150:
		break;
	default:
		printf("STOR error!\n");
		close(fd_local);		
	}

	// transfer the file
	if ((ret = copyfd_size(fd_local, fd_data, filesize)) == -1) 
	{
		//printf("copyfd_size() Failed!\n");
		return -1;
	}	

	// close it all down
	close(fd_data);
	if (ftpcmd(NULL, NULL, control_stream, buf) != 226) 
	{
		printf("ftpcmd(%s) Failed!\n", buf);
		return -1;
	}
	ftpcmd("QUIT", NULL, control_stream, buf);

	return 0;
}

static size_t copyfd_size_ext(char *src, int dst_fd, const size_t size)
{
	int ret = -1;
	int read_total = 0;

	if (src == NULL)
	{
		return -1;
	}

	while ((size == 0) || (read_total < size)) 
	{
		int read_try;

 		if ((size == 0) || (size - read_total > 4096)) 
 		{
			read_try = 4096;
		} 
		else 
		{
			read_try = size - read_total;
		}

		if ((ret=write(dst_fd, src+read_total, (size_t) read_try)) != read_try)
		{
			printf("write: %d(%d)\n", ret, read_total);
			break;
		}

		read_total += read_try;
	}
	
	//printf("copyfd_size_ext: %d(%d)\n", read_total, size);

	return(read_total);
}

static int ftp_send_ext(ftp_host_info_t *server, FILE *control_stream, char *local_path, char *local_data, unsigned long data_size, char *server_path)
{
	int ret = -1;
	char buf[512];
	int fd_data = -1;
	int response = 0;
	FTP_PARAM ftp_param;

	// Connect to the data socket
	if (ftpcmd("PASV", NULL, control_stream, buf) != 227) 
	{
		return -1;
	}

	fd_data = connect_ftpdata(server, buf);
	if (fd_data < 0)
	{
		return -1;
	}

	//printf("Entry ftp_send_ext function ftp_param.strPath = %s\n", ftp_param.strPath);
	//if (ftpcmd("CWD ", server_path, control_stream, buf) != 250) // [zhb][delete][2006-09-18]
	#if 1
	//改变ftp 工作目录
	ret = getFtpParam(&ftp_param);
	if (ret < 0)
	{
		return -1;
	}

#if 1
	if (ftpcmd("MKD ", ftp_param.strPath, control_stream, buf) != 250) 
	{
		printf("%s\n", buf);
		//close(fd_data);
		//return -1;
	}
#endif

	if (ftpcmd("CWD ", ftp_param.strPath, control_stream, buf) != 250) 
	{
		printf("%s\n", buf);
		close(fd_data);
		return -1;
	}
	#endif

	#if 0
	if (ftpcmd("CWD ", "./", control_stream, buf) != 250) 
	{
		printf("%s\n", buf);
		close(fd_data);
		return -1;
	}
	#endif
	
	
	
	// get the local file
	sprintf(buf, "ALLO %lu", (unsigned long)data_size);
	response = ftpcmd(buf, NULL, control_stream, buf);	
	switch (response) 
	{
	case 200:
	case 202:
		break;
	default:
		printf("ALLO error(%s)!\n", buf);
		break;
	}
	
	response = ftpcmd("STOR ", local_path, control_stream, buf);
	switch (response) 
	{
	case 125:
	case 150:
		break;
	default:
		printf("STOR error!\n");
	}

	// transfer the file
	if ((ret = copyfd_size_ext(local_data, fd_data, data_size)) == -1) 
	{
		//printf("copyfd_size_ext() Failed!\n");
		// Add the code by lvjh, 2009-04-02
		close(fd_data);
		
		return -1;
	}	

	// close it all down
	close(fd_data);
	if (ftpcmd(NULL, NULL, control_stream, buf) != 226) 
	{
		printf("ftpcmd(%s) Failed!\n", buf);
		return -1;
	}
	ftpcmd("QUIT", NULL, control_stream, buf);

	return 0;
}

#define FTPGETPUT_OPT_CONTINUE	1
#define FTPGETPUT_OPT_VERBOSE	2
#define FTPGETPUT_OPT_USER	4
#define FTPGETPUT_OPT_PASSWORD	8
#define FTPGETPUT_OPT_PORT	16

static const struct option ftpgetput_long_options[] = 
{
	{"continue", 1, NULL, 'c'},
	{"verbose", 0, NULL, 'v'},
	{"username", 1, NULL, 'u'},
	{"password", 1, NULL, 'p'},
	{"port", 1, NULL, 'P'},
	{0, 0, 0, 0}
};

int FTP_GetFile(char *tftpserver, int port, char *username, char *password, char *localfile, char *remotefile)
{
	int fd = -1;
	int ret = -1;
	struct hostent *host = 	NULL;
	ftp_host_info_t server;
	unsigned short port1 = 0;
	
	FILE *control_stream = NULL;
	
	if (tftpserver==NULL || username==NULL || password==NULL || remotefile==NULL || localfile==NULL)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 21;
	}
	port1 = htons(port);

	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	
	strncpy(server.user, username, 32);
	strncpy(server.password, password, 32);
	server.s_in.sin_family = host->h_addrtype;
	server.s_in.sin_port = port1;
	memcpy(&server.s_in.sin_addr, (struct in_addr *) host->h_addr, sizeof(server.s_in.sin_addr));
	
	control_stream = ftp_login(&server);
	if (control_stream == NULL)
	{
		printf("ftp_login() Failed!\n");
		return -1;	
	}

	ret = ftp_recieve(&server, control_stream, localfile, remotefile);
	
	return ret;
}

int FTP_PutFile(char *tftpserver, int port, char *username, char *password, char *localfile, char *remotefile)
{
	int fd = -1;
	int ret = -1;
	struct hostent *host = 	NULL;
	ftp_host_info_t server;
	unsigned short port1 = 0;
	
	FILE *control_stream = NULL;
	
	if (tftpserver==NULL || username==NULL || password==NULL || remotefile==NULL || localfile==NULL)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 21;
	}

	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	port1 = htons(port);
	
	strncpy(server.user, username, 32);
	strncpy(server.password, password, 32);
	server.s_in.sin_family = host->h_addrtype;
	server.s_in.sin_port = port1;
	memcpy(&server.s_in.sin_addr, (struct in_addr *) host->h_addr, sizeof(server.s_in.sin_addr));
	
	control_stream = ftp_login(&server);
	if (control_stream == NULL)
	{
		printf("ftp_login() Failed!\n");
		return -1;	
	}

	ret = ftp_send(&server, control_stream, localfile, remotefile);
	
	return ret;
}

int FTP_PutFile_Ext(char *tftpserver, int port, char *username, char *password, char *localfile, char *localfiledata, unsigned long datasize, char *remotefile)
{
	int fd = -1;
	int ret = -1;
	struct hostent *host = 	NULL;
	ftp_host_info_t server;
	unsigned short port1 = 0;
	
	FILE *control_stream = NULL;
	
	// Add the code by lvjh, 2009-04-02
	g_ftp_socket = -1;
	
	if (tftpserver==NULL || username==NULL || password==NULL || remotefile==NULL || localfile==NULL)
	{
		return -1;
	}
	// Add the code by lvjh, 2010-06-02
	if (strlen(tftpserver)==0 || strlen(username)==0)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 21;
	}

	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	port1 = htons(port);
	
	strncpy(server.user, username, 32);
	strncpy(server.password, password, 32);
	server.s_in.sin_family = host->h_addrtype;
	server.s_in.sin_port = port1;
	memcpy(&server.s_in.sin_addr, (struct in_addr *) host->h_addr, sizeof(server.s_in.sin_addr));
	
	control_stream = ftp_login(&server);
	if (control_stream == NULL)
	{
		printf("ftp_login() Failed!\n");
		return -1;	
	}

	printf("Data Size: %d\n", datasize);

	ret = ftp_send_ext(&server, control_stream, localfile, localfiledata, datasize, remotefile);

	printf("FTP_PutFile_Ext ret = %d\n",ret);
	fclose(control_stream);
	close(g_ftp_socket);
	g_ftp_socket = -1;
	
	return ret;
}
