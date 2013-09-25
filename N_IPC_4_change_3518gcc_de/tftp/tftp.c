#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "tftp.h"

//#define CONFIG_FEATURE_TFTP_DEBUG
#define CONFIG_FEATURE_CLEAN_UP
//#define CONFIG_FEATURE_TFTP_BLOCKSIZE

#define TFTP_BLOCKSIZE_DEFAULT 512 	// according to RFC 1350, don't change
#define TFTP_TIMEOUT 5             	// seconds

#define TFTP_RRQ   1
#define TFTP_WRQ   2
#define TFTP_DATA  3
#define TFTP_ACK   4
#define TFTP_ERROR 5
#define TFTP_OACK  6

static const char *tftp_error_msg[] = 
{
	"Undefined error",
	"File not found",
	"Access violation",
	"Disk full or allocation error",
	"Illegal TFTP operation",
	"Unknown transfer ID",
	"File already exists",
	"No such user"
};

const int tftp_cmd_get = 1;
const int tftp_cmd_put = 2;

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE

static int tftp_blocksize_check(int blocksize, int bufsize)
{
	if ((bufsize && (blocksize > bufsize)) ||
	    (blocksize < 8) || (blocksize > 65464)) 
	{
		return 0;
	}

	return blocksize;
}

static char *tftp_option_get(char *buf, int len, char *option)
{
	int opt_val = 0;
	int opt_found = 0;
	int k = 0;

	while (len > 0) 
	{
		for (k=0; k<len; k++) 
		{
			if (buf[k] == '\0') 
			{
				break;
			}
		}

		if (k >= len) 
		{
			break;
		}

		if (opt_val == 0) 
		{
			if (strcasecmp(buf, option) == 0) 
			{
				opt_found = 1;
			}
		}
		else 
		{
			if (opt_found) 
			{
				return buf;
			}
		}

		k++;

		buf += k;
		len -= k;

		opt_val ^= 1;
	}

	return NULL;
}

#endif

static inline int tftp(int cmd, struct hostent *host, char *remotefile, int localfd, int port, int tftp_bufsize)
{
	const int cmd_get = cmd & tftp_cmd_get;
	const int cmd_put = cmd & tftp_cmd_put;
	const int bb_tftp_num_retries = 5;

	struct sockaddr_in sa;
	struct sockaddr_in from;
	struct timeval tv;
	socklen_t fromlen;
	fd_set rfds;
	char *cp;
	unsigned short tmp;
	int socketfd;
	int len;
	int opcode = 0;
	int finished = 0;
	int timeout = bb_tftp_num_retries;
	unsigned short block_nr = 1;

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE
	int want_option_ack = 0;
#endif

	char *buf=malloc(tftp_bufsize + 4);

	tftp_bufsize += 4;

	if ((socketfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		return -1;
	}

	len = sizeof(sa);

	memset(&sa, 0, len);
	bind(socketfd, (struct sockaddr *)&sa, len);

	sa.sin_family = host->h_addrtype;
	sa.sin_port = port;
	memcpy(&sa.sin_addr, (struct in_addr *) host->h_addr, sizeof(sa.sin_addr));	
	
	if (cmd_get) 
	{
		opcode = TFTP_RRQ;
	}

	if (cmd_put) 
	{
		opcode = TFTP_WRQ;
	}

	while (1) 
	{
		cp = buf;

		*((unsigned short *) cp) = htons(opcode);

		cp += 2;

		if ((cmd_get && (opcode == TFTP_RRQ)) ||
			(cmd_put && (opcode == TFTP_WRQ))) 
		{
			int too_long = 0;

			len = strlen(remotefile) + 1;

			if ((cp + len) >= &buf[tftp_bufsize - 1]) 
			{
				too_long = 1;
			}
			else 
			{
				strncpy(cp, remotefile, len);
				cp += len;
			}

			if (too_long || ((&buf[tftp_bufsize - 1] - cp) < 6)) 
			{
				break;
			}


			memcpy(cp, "octet", 6);
			cp += 6;

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE

			len = tftp_bufsize - 4; /* data block size */

			if (len != TFTP_BLOCKSIZE_DEFAULT) 
			{
				if ((&buf[tftp_bufsize - 1] - cp) < 15) 
				{
					break;
				}

				memcpy(cp, "blksize", 8);
				cp += 8;
				cp += snprintf(cp, 6, "%d", len) + 1;
				want_option_ack = 1;
			}
#endif
		}

		/* add ack and data */
		
		if ((cmd_get && (opcode == TFTP_ACK)) ||
			(cmd_put && (opcode == TFTP_DATA))) 
		{
			*((unsigned short *) cp) = htons(block_nr);
			cp += 2;
			block_nr++;

			if (cmd_put && (opcode == TFTP_DATA)) 
			{
				//len = bb_full_read(localfd, cp, tftp_bufsize - 4);
				len = read(localfd, cp, tftp_bufsize - 4);
				//printf("read(%d)\n", len);
				if (len < 0) 
				{
					break;
				}

				if (len != (tftp_bufsize - 4)) 
				{
					finished++;
				}

				cp += len;
			}
		}


		/* send packet */
		timeout = bb_tftp_num_retries;  /* re-initialize */
		do 
		{
			len = cp - buf;

#ifdef CONFIG_FEATURE_TFTP_DEBUG
			printf("sending(%d) %u bytes\n", __LINE__, len);
			for (cp = buf; cp < &buf[len]; cp++)
				fprintf(stderr, "%02x ", (unsigned char)*cp);
			fprintf(stderr, "\n");
#endif
			if (sendto(socketfd, buf, len, 0, (struct sockaddr *) &sa, sizeof(sa)) < 0) 
			{				
				len = -1;
				break;
			}

			if (finished && (opcode == TFTP_ACK)) 
			{
				break;
			}

			/* receive packet */

			memset(&from, 0, sizeof(from));
			fromlen = sizeof(from);

			tv.tv_sec = TFTP_TIMEOUT;
			tv.tv_usec = 0;

			FD_ZERO(&rfds);
			FD_SET(socketfd, &rfds);

			switch (select(FD_SETSIZE, &rfds, NULL, NULL, &tv)) 
			{
			case 1:
				len = recvfrom(socketfd, buf, tftp_bufsize, 0, (struct sockaddr *) &from, &fromlen);

				if (len < 0) 
				{
					break;
				}

				timeout = 0;

				if (sa.sin_port == port) 
				{
					sa.sin_port = from.sin_port;
				}
				if (sa.sin_port == from.sin_port) 
				{
					break;
				}

				/* fall-through for bad packets! */
				/* discard the packet - treat as timeout */
				timeout = bb_tftp_num_retries;

			case 0:
				timeout--;
				if (timeout == 0) 
				{
					len = -1;
				}
				break;

			default:
				len = -1;
			}

		} while (timeout && (len >= 0));

		if ((finished) || (len < 0)) 
		{
			break;
		}

		/* process received packet */
		opcode = ntohs(*((unsigned short *) buf));
		tmp = ntohs(*((unsigned short *) &buf[2]));

#ifdef CONFIG_FEATURE_TFTP_DEBUG
		printf("received(%d) %d bytes: %04x %04x\n", __LINE__, len, opcode, tmp);
#endif

		if (opcode == TFTP_ERROR) 
		{
			char *msg = NULL;

			if (buf[4] != '\0') 
			{
				msg = &buf[4];
				buf[tftp_bufsize - 1] = '\0';
			} 
			else if (tmp < (sizeof(tftp_error_msg) / sizeof(char *))) 
			{
				msg = (char *) tftp_error_msg[tmp];
			}

#ifdef CONFIG_FEATURE_TFTP_DEBUG
			if (msg) 
			{
				printf("TFTP ERROR(%d): %s\n", __LINE__, msg);
			}
#endif

			break;
		}

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE
		if (want_option_ack) 
		{
			 want_option_ack = 0;

			if (opcode == TFTP_OACK) 
			{
				/* server seems to support options */
				char *res;

				res = tftp_option_get(&buf[2], len-2, "blksize");
				if (res) 
				{
					int blksize = atoi(res);
			
					if (tftp_blocksize_check(blksize, tftp_bufsize - 4)) 
					{
						if (cmd_put) 
						{
							opcode = TFTP_DATA;
						}
						else 
						{
							opcode = TFTP_ACK;
						}
#ifdef CONFIG_FEATURE_TFTP_DEBUG
						printf("using blksize(%d) %u\n", __LINE__, blksize);
#endif
						tftp_bufsize = blksize + 4;
						block_nr = 0;
						continue;
					}
				}
				/* FIXME:
				* we should send ERROR 8 */
				break;
			}

			tftp_bufsize = TFTP_BLOCKSIZE_DEFAULT + 4;
		}
#endif

		if (cmd_get && (opcode == TFTP_DATA)) 
		{
			if (tmp == block_nr) 
			{			
				//len = bb_full_write(localfd, &buf[4], len - 4);
				len = write(localfd, &buf[4], len - 4);

				if (len < 0) 
				{
					break;
				}

				if (len != (tftp_bufsize - 4)) 
				{
					finished++;
				}

				opcode = TFTP_ACK;
				continue;
			}
		}

		if (cmd_put && (opcode == TFTP_ACK)) 
		{
			if (tmp == (unsigned short)(block_nr - 1)) 
			{
				if (finished) 
				{
					break;
				}

				opcode = TFTP_DATA;
				continue;
			}
		}
	}

#ifdef CONFIG_FEATURE_CLEAN_UP
	close(socketfd);

	free(buf);
#endif

	return finished ? 0 : -1;
}

static inline int tftp_ext(int cmd, struct hostent *host, char *remotefile, char *filedata, int *datasize, int port, int tftp_bufsize)
{
	const int cmd_get = cmd & tftp_cmd_get;
	const int cmd_put = cmd & tftp_cmd_put;
	const int bb_tftp_num_retries = 5;

	struct sockaddr_in sa;
	struct sockaddr_in from;
	struct timeval tv;
	socklen_t fromlen;
	fd_set rfds;
	char *cp;
	unsigned short tmp;
	int socketfd;
	int len;
	int opcode = 0;
	int finished = 0;
	int timeout = bb_tftp_num_retries;
	unsigned short block_nr = 1;
	unsigned long totalsize = 0;

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE
	int want_option_ack = 0;
#endif

	char *buf=malloc(tftp_bufsize + 4);

	tftp_bufsize += 4;

	if ((socketfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		return -1;
	}

	len = sizeof(sa);

	memset(&sa, 0, len);
	bind(socketfd, (struct sockaddr *)&sa, len);

	sa.sin_family = host->h_addrtype;
	sa.sin_port = port;
	memcpy(&sa.sin_addr, (struct in_addr *) host->h_addr, sizeof(sa.sin_addr));	
	
	if (cmd_get) 
	{
		opcode = TFTP_RRQ;
	}

	if (cmd_put) 
	{
		opcode = TFTP_WRQ;
	}
	
	while (1) 
	{
		cp = buf;

		*((unsigned short *) cp) = htons(opcode);

		cp += 2;

		if ((cmd_get && (opcode == TFTP_RRQ)) ||
			(cmd_put && (opcode == TFTP_WRQ))) 
		{
			int too_long = 0;

			len = strlen(remotefile) + 1;

			if ((cp + len) >= &buf[tftp_bufsize - 1]) 
			{
				too_long = 1;
			}
			else 
			{
				strncpy(cp, remotefile, len);
				cp += len;
			}

			if (too_long || ((&buf[tftp_bufsize - 1] - cp) < 6)) 
			{
				break;
			}


			memcpy(cp, "octet", 6);
			cp += 6;

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE

			len = tftp_bufsize - 4; /* data block size */

			if (len != TFTP_BLOCKSIZE_DEFAULT) 
			{
				if ((&buf[tftp_bufsize - 1] - cp) < 15) 
				{
					break;
				}

				memcpy(cp, "blksize", 8);
				cp += 8;
				cp += snprintf(cp, 6, "%d", len) + 1;
				want_option_ack = 1;
			}
#endif
		}

		/* add ack and data */
		
		if ((cmd_get && (opcode == TFTP_ACK)) ||
			(cmd_put && (opcode == TFTP_DATA))) 
		{
			*((unsigned short *) cp) = htons(block_nr);
			cp += 2;
			block_nr++;

			if (cmd_put && (opcode == TFTP_DATA)) 
			{
				//len = bb_full_read(localfd, cp, tftp_bufsize - 4);
				//len = read(localfd, cp, tftp_bufsize - 4);
				
				if ((*datasize)-totalsize >= (tftp_bufsize - 4))
				{
					len = tftp_bufsize - 4;
					memcpy(cp, filedata+totalsize, len);
					totalsize += len;
				}
				else
				{
					len = (*datasize)-totalsize;
					memcpy(cp, filedata+totalsize, len);
					totalsize += len;
				}

				//printf("read data(%d)\n", len);
				
				cp += len;

				if (totalsize >= *datasize) 
				{
					finished++;
					//break;
				}
				/*
				if (len != (tftp_bufsize - 4)) 
				{
					finished++;
				}
				*/
			}
		}


		/* send packet */
		timeout = bb_tftp_num_retries;  /* re-initialize */
		do 
		{
			len = cp - buf;

#ifdef CONFIG_FEATURE_TFTP_DEBUG
			printf("sending(%d) %u bytes\n",  __LINE__, len);
			for (cp = buf; cp < &buf[len]; cp++)
				printf("%02x ", (unsigned char)*cp);
			printf("\n");
#endif
			if (sendto(socketfd, buf, len, 0, (struct sockaddr *) &sa, sizeof(sa)) < 0) 
			{				
				len = -1;
				break;
			}

			if (finished && (opcode == TFTP_ACK)) 
			{
				break;
			}

			/* receive packet */

			memset(&from, 0, sizeof(from));
			fromlen = sizeof(from);

			tv.tv_sec = TFTP_TIMEOUT;
			tv.tv_usec = 0;

			FD_ZERO(&rfds);
			FD_SET(socketfd, &rfds);

			switch (select(FD_SETSIZE, &rfds, NULL, NULL, &tv)) 
			{
			case 1:
				len = recvfrom(socketfd, buf, tftp_bufsize, 0, (struct sockaddr *) &from, &fromlen);

				if (len < 0) 
				{
					break;
				}

				timeout = 0;

				if (sa.sin_port == port) 
				{
					sa.sin_port = from.sin_port;
				}
				if (sa.sin_port == from.sin_port) 
				{
					break;
				}

				/* fall-through for bad packets! */
				/* discard the packet - treat as timeout */
				timeout = bb_tftp_num_retries;

			case 0:
				timeout--;
				if (timeout == 0) 
				{
					len = -1;
				}
				break;

			default:
				len = -1;
			}

		} while (timeout && (len >= 0));

		if ((finished) || (len < 0)) 
		{
			break;
		}

		/* process received packet */
		opcode = ntohs(*((unsigned short *) buf));
		tmp = ntohs(*((unsigned short *) &buf[2]));

#ifdef CONFIG_FEATURE_TFTP_DEBUG
		printf("received(%d) %d bytes: %04x %04x\n", __LINE__, len, opcode, tmp);
#endif

		if (opcode == TFTP_ERROR) 
		{
			char *msg = NULL;

			if (buf[4] != '\0') 
			{
				msg = &buf[4];
				buf[tftp_bufsize - 1] = '\0';
			} 
			else if (tmp < (sizeof(tftp_error_msg) / sizeof(char *))) 
			{
				msg = (char *) tftp_error_msg[tmp];
			}

#ifdef CONFIG_FEATURE_TFTP_DEBUG
			if (msg) 
			{
				printf("TFTP_ERROR(%d): %s\n", __LINE__, msg);
			}
#endif
			
			break;
		}

#ifdef CONFIG_FEATURE_TFTP_BLOCKSIZE
		if (want_option_ack) 
		{
			 want_option_ack = 0;

			if (opcode == TFTP_OACK) 
			{
				/* server seems to support options */
				char *res;

				res = tftp_option_get(&buf[2], len-2, "blksize");
				if (res) 
				{
					int blksize = atoi(res);
			
					if (tftp_blocksize_check(blksize, tftp_bufsize - 4)) 
					{
						if (cmd_put) 
						{
							opcode = TFTP_DATA;
						}
						else 
						{
							opcode = TFTP_ACK;
						}
#ifdef CONFIG_FEATURE_TFTP_DEBUG
						printf("using blksize(%d) %u\n", __LINE__, blksize);
#endif
						tftp_bufsize = blksize + 4;
						block_nr = 0;
						continue;
					}
				}
				/* FIXME:
				* we should send ERROR 8 */
				break;
			}

			tftp_bufsize = TFTP_BLOCKSIZE_DEFAULT + 4;
		}
#endif

		if (cmd_get && (opcode == TFTP_DATA)) 
		{
			if (tmp == block_nr) 
			{			
				//len = bb_full_write(localfd, &buf[4], len - 4);
				//len = write(localfd, &buf[4], len - 4);
				
				memcpy(filedata, &buf[4], len-4);
				totalsize += (len-4);

				if (len-4 < 0) 
				{
					break;
				}

				/*
				if (len != (tftp_bufsize - 4)) 
				{
					finished++;
				}
				*/
				
				opcode = TFTP_ACK;
				continue;
			}
		}

		if (cmd_put && (opcode == TFTP_ACK)) 
		{
			if (tmp == (unsigned short)(block_nr - 1)) 
			{
				if (finished) 
				{
					break;
				}

				opcode = TFTP_DATA;
				continue;
			}
		}
	}

#ifdef CONFIG_FEATURE_CLEAN_UP
	close(socketfd);

	free(buf);
#endif

	return finished ? 0 : -1;
}

int TFTP_GetFile(char *tftpserver, int port, char *remotefile)
{
	int fd = -1;
	int ret = -1;
	struct hostent *host = 	NULL;
	unsigned short port1 = 0;
	
	if (tftpserver==NULL || remotefile==NULL)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 69;
	}
	
	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	port1 = htons(port);
	
	fd = open(remotefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
	{
		return -1;
	}	
	
	ret = tftp(tftp_cmd_get, host, remotefile, fd, port1, TFTP_BLOCKSIZE_DEFAULT);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

int TFTP_PutFile(char *tftpserver, int port, char *remotefile)
{
	int fd = -1;
	int ret = -1;
	struct hostent *host = 	NULL;
	unsigned short port1 = 0;
	
	if (tftpserver==NULL || remotefile==NULL)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 69;
	}
	
	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	port1 = htons(port);
	
	fd = open(remotefile, O_RDONLY, 0644);
	if (fd < 0)
	{
		printf("Can not open file: %s\n", remotefile);
		return -1;
	}	
	
	ret = tftp(tftp_cmd_put, host, remotefile, fd, port1, TFTP_BLOCKSIZE_DEFAULT);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

int TFTP_PutFile_Ext(char *tftpserver, int port, char *remotefile, char *filedata, int *datasize)
{
	int ret = -1;
	struct hostent *host = 	NULL;
	unsigned short port1 = 0;
	
	if (tftpserver==NULL || remotefile==NULL)
	{
		return -1;
	}
	if (port<=0 || port>65535)
	{
		port = 69;
	}
	// Add the code by lvjh, 2010-06-02
	if (strlen(tftpserver) == 0)
	{
		return -1;
	}
	
	host = gethostbyname(tftpserver);
	if (host == NULL)
	{
		return -1;
	}
	port1 = htons(port);
	
	ret = tftp_ext(tftp_cmd_put, host, remotefile, filedata, datasize, port1, TFTP_BLOCKSIZE_DEFAULT);
	if (ret < 0)
	{
		return -1;
	}
	
	return 0;
}

