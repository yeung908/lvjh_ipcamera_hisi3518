#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#ifndef CYG
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <linux/route.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>     
#include <linux/if_ether.h>
#include <stddef.h>
#endif

#include "netTools.h"

#define PACKET_SIZE			4096
#define MAX_WAIT_TIME		5

#define ICMP_ECHO			8
#define ICMP_ECHOREPLY		0

struct arg1opt 
{
	const char *name;
	unsigned short selector;
	unsigned short ifr_offset;
};

struct options 
{
	const char *name;
	const unsigned char  flags;
	const unsigned char  arg_flags;
	const unsigned short selector;
};

#define ifreq_offsetof(x)  offsetof(struct ifreq, x)

#define IFF_DYNAMIC     0x8000

#define N_CLR            0x01
#define M_CLR            0x02
#define N_SET            0x04
#define M_SET            0x08
#define N_ARG            0x10
#define M_ARG            0x20

#define M_MASK           (M_CLR | M_SET | M_ARG)
#define N_MASK           (N_CLR | N_SET | N_ARG)
#define SET_MASK         (N_SET | M_SET)
#define CLR_MASK         (N_CLR | M_CLR)
#define SET_CLR_MASK     (SET_MASK | CLR_MASK)
#define ARG_MASK         (M_ARG | N_ARG)

#define A_CAST_TYPE      0x03

#define A_MAP_TYPE       0x0C
#define A_ARG_REQ        0x10	/* Set if an arg is required. */
#define A_NETMASK        0x20	/* Set if netmask (check for multiple sets). */
#define A_SET_AFTER      0x40	/* Set a flag at the end. */
#define A_COLON_CHK      0x80	/* Is this needed?  See below. */

#define A_CAST_CHAR_PTR  0x01
#define A_CAST_RESOLVE   0x01
#define A_CAST_HOST_COPY 0x02
#define A_CAST_HOST_COPY_IN_ETHER    A_CAST_HOST_COPY
#define A_CAST_HOST_COPY_RESOLVE     (A_CAST_HOST_COPY | A_CAST_RESOLVE)


#define A_MAP_ULONG      0x04	/* memstart */
#define A_MAP_USHORT     0x08	/* io_addr */
#define A_MAP_UCHAR      0x0C	/* irq */


#define ARG_METRIC       (A_ARG_REQ /*| A_CAST_INT*/)
#define ARG_MTU          (A_ARG_REQ /*| A_CAST_INT*/)
#define ARG_TXQUEUELEN   (A_ARG_REQ /*| A_CAST_INT*/)
#define ARG_MEM_START    (A_ARG_REQ | A_MAP_ULONG)
#define ARG_IO_ADDR      (A_ARG_REQ | A_MAP_ULONG)
#define ARG_IRQ          (A_ARG_REQ | A_MAP_UCHAR)
#define ARG_DSTADDR      (A_ARG_REQ | A_CAST_HOST_COPY_RESOLVE)
#define ARG_NETMASK      (A_ARG_REQ | A_CAST_HOST_COPY_RESOLVE | A_NETMASK)
#define ARG_BROADCAST    (A_ARG_REQ | A_CAST_HOST_COPY_RESOLVE | A_SET_AFTER)
#define ARG_HW           (A_ARG_REQ | A_CAST_HOST_COPY_IN_ETHER)
#define ARG_POINTOPOINT  (A_CAST_HOST_COPY_RESOLVE | A_SET_AFTER)
#define ARG_KEEPALIVE    (A_ARG_REQ | A_CAST_CHAR_PTR)
#define ARG_OUTFILL      (A_ARG_REQ | A_CAST_CHAR_PTR)
#define ARG_HOSTNAME     (A_CAST_HOST_COPY_RESOLVE | A_SET_AFTER | A_COLON_CHK)

static const struct arg1opt Arg1Opt[] = {
	{"SIOCSIFMETRIC",  SIOCSIFMETRIC,  ifreq_offsetof(ifr_metric)},
	{"SIOCSIFMTU",     SIOCSIFMTU,     ifreq_offsetof(ifr_mtu)},
	{"SIOCSIFTXQLEN",  SIOCSIFTXQLEN,  ifreq_offsetof(ifr_qlen)},
	{"SIOCSIFDSTADDR", SIOCSIFDSTADDR, ifreq_offsetof(ifr_dstaddr)},
	{"SIOCSIFNETMASK", SIOCSIFNETMASK, ifreq_offsetof(ifr_netmask)},
	{"SIOCSIFBRDADDR", SIOCSIFBRDADDR, ifreq_offsetof(ifr_broadaddr)},
	{"SIOCSIFHWADDR",  SIOCSIFHWADDR,  ifreq_offsetof(ifr_hwaddr)},
	{"SIOCSIFDSTADDR", SIOCSIFDSTADDR, ifreq_offsetof(ifr_dstaddr)},
	{"SIOCSIFMAP",     SIOCSIFMAP,     ifreq_offsetof(ifr_map.mem_start)},
	{"SIOCSIFMAP",     SIOCSIFMAP,     ifreq_offsetof(ifr_map.base_addr)},
	{"SIOCSIFMAP",     SIOCSIFMAP,     ifreq_offsetof(ifr_map.irq)},
	/* Last entry if for unmatched (possibly hostname) arg. */
	{"SIOCSIFADDR",    SIOCSIFADDR,    ifreq_offsetof(ifr_addr)},
};


static const struct options OptArray[] = {
	{"metric",       N_ARG,         ARG_METRIC,      0},
    {"mtu",          N_ARG,         ARG_MTU,         0},
	{"txqueuelen",   N_ARG,         ARG_TXQUEUELEN,  0},
	{"dstaddr",      N_ARG,         ARG_DSTADDR,     0},
	{"netmask",      N_ARG,         ARG_NETMASK,     0},
	{"broadcast",    N_ARG | M_CLR, ARG_BROADCAST,   IFF_BROADCAST},
	{"hw",           N_ARG,         ARG_HW,          0},
	{"pointopoint",  N_ARG | M_CLR, ARG_POINTOPOINT, IFF_POINTOPOINT},
	{"mem_start",    N_ARG,         ARG_MEM_START,   0},
	{"io_addr",      N_ARG,         ARG_IO_ADDR,     0},
	{"irq",          N_ARG,         ARG_IRQ,         0},
	{"arp",          N_CLR | M_SET, 0,               IFF_NOARP},
	{"trailers",     N_CLR | M_SET, 0,               IFF_NOTRAILERS},
	{"promisc",      N_SET | M_CLR, 0,               IFF_PROMISC},
	{"multicast",    N_SET | M_CLR, 0,               IFF_MULTICAST},
	{"allmulti",     N_SET | M_CLR, 0,               IFF_ALLMULTI},
	{"dynamic",      N_SET | M_CLR, 0,               IFF_DYNAMIC},
	{"up",           N_SET        , 0,               (IFF_UP | IFF_RUNNING)},
	{"down",         N_CLR        , 0,               IFF_UP},
	{ NULL,          0,             ARG_HOSTNAME,    (IFF_UP | IFF_RUNNING)}
};

static int in_ether(char *bufp, struct sockaddr *sap)
{
	unsigned char *ptr;
	int i, j;
	unsigned char val;
	unsigned char c;

	sap->sa_family = ARPHRD_ETHER;
	ptr = (unsigned char *)(sap->sa_data);

	i = 0;
	do 
	{
		j = val = 0;

		// We might get a semicolon here - not required. 
		if (i && (*bufp == ':')) 
		{
			bufp++;
		}

		do 
		{
			c = (unsigned char)(*bufp);
			if (((unsigned char)(c - '0')) <= 9) 
			{
				c -= '0';
			}
			else if (((unsigned char)((c|0x20) - 'a')) <= 5) 
			{
				c = (c|0x20) - ('a'-10);
			} 
			else if (j && (c == ':' || c == 0)) 
			{
				break;
			} 
			else 
			{
				return -1;
			}
			++bufp;
			val <<= 4;
			val += c;
		} while (++j < 2);

		*ptr++ = val;
	} while (++i < ETH_ALEN);

	return (int) (*bufp);	// Error if we don't end at end of string. 
}


char * safe_strncpy(char *dst, const char *src, size_t size)
{
    dst[size-1] = '\0';
    return strncpy(dst, src, size-1);
}

int set_net_phyaddr(char *name, char *addr)  	// name : eth0 eth1 lo and so on 
{												// addr : 12:13:14:15:16:17
	const struct arg1opt *a1op;
	const struct options *op;
	int 	sockfd;		
	struct 	ifreq ifr;
	struct 	sockaddr sa;
	unsigned char mask;
	char 	*p = "hw";
	char 	host[128];
	
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		return -1;
	}

	/* get interface name */
	safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
	mask = N_MASK;
	
	for (op = OptArray ; op->name ; op++) 
	{		
		if (strcmp("hw",op->name) == 0) 
		{
			if ((mask &= op->flags)) 
			{ 
			    goto SET_NET_PHYADDR_FOUND_ARG;
			}
		}
	}
	return -4;
		
SET_NET_PHYADDR_FOUND_ARG:		
	
	a1op = Arg1Opt + 6;
	
	safe_strncpy(host, addr, (sizeof host));
		
	if (in_ether(host, &sa)) 
	{
		return -2;
	}
	p = (char *) &sa;
		
	memcpy((((char *) (&ifr)) + a1op->ifr_offset), p, sizeof(struct sockaddr));
		
	if (ioctl(sockfd, a1op->selector, &ifr) < 0) 
	{
		return -3;
	}

	return  0;
}

int ifconfig_up_down(char *name, char *action) // name :  eth0 eth1 lo and so on 
{											   // action: down up	
	const struct options *op;
	int 	sockfd;		
	int 	selector;
	struct 	ifreq ifr;
	unsigned char mask;
	
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		return -1;
	}

	/* get interface name */
	safe_strncpy(ifr.ifr_name, name, IFNAMSIZ);
	mask = N_MASK;
	
	for (op = OptArray ; op->name ; op++) 
	{		
			if (strcmp(action,op->name) == 0) 
			{
				if ((mask &= op->flags)) 
				{ 
				    goto IFCONFIG_UP_DOWN_FOUND_ARG;
				}
			}
	}
	return -4;
		
IFCONFIG_UP_DOWN_FOUND_ARG:		
	
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) 
	{
		return -2;
	} 
	else 
	{
		selector = op->selector;
		if (mask & SET_MASK) 
		{
			ifr.ifr_flags |= selector;
		} 
		else 
		{
			ifr.ifr_flags &= ~selector;
		}
		if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) 
		{
			return -3;
		}
	}						

	return  0;
}


int pid;
int sock_fd;
char send_packet[PACKET_SIZE];
char recv_packet[PACKET_SIZE];
struct sockaddr_in dest_addr;

unsigned short check_sum(unsigned short *buf,int len)
{
	int left = len;
	int sum = 0;
	unsigned short *addr = buf;
	unsigned short answer = 0;
	while(left > 1)
	{
		sum += *addr++;
		left -= 2;
	}
	if(left == 1)
	{
		*(unsigned char *)(&answer) = *(unsigned char *)addr;
		sum += answer;
	}
	 
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	
	return answer;
}

int send_icmp_packet(char *addr)
{
	int pack_size;
	int ret;
	struct icmp *icmp;
	struct timeval *tv;
	
	pid = getpid();
	pack_size = 64;
	
	/* create a raw socket for icmp */
	if((sock_fd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP)) < 0)
		return -1; 
	
	/* set destination host information */
	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr(addr);
	
	/* fill in icmp packet */
	icmp = (struct icmp *)send_packet;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = 0;
	icmp->icmp_id = pid;
	tv = (struct timeval *)icmp->icmp_data;
	gettimeofday(tv,NULL);
	icmp->icmp_cksum = check_sum((unsigned short *)icmp,pack_size);
	
	ret = sendto(sock_fd,send_packet,pack_size,0,(struct sockaddr *)&dest_addr,sizeof(dest_addr));

	return ret;
}

int recv_icmp_packet()
{
	int ip_head_len;
	int recv_len;
	int ret;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval tv;
	fd_set fdset;
	
	/* set timeout */
	tv.tv_sec = MAX_WAIT_TIME;
	tv.tv_usec = 0;
	
	/* set fd_set */
	FD_ZERO(&fdset);
	FD_SET(sock_fd,&fdset);
	
	if((ret = select(sock_fd+1,&fdset,NULL,NULL,&tv)) == -1)
	{
		close(sock_fd);
		printf("selec error !\n");
		return -1;
	}
	else if(ret == 0)
	{
		printf("receive timeout !\n");
		return -1;
	}
	else
	{
		ret = recvfrom(sock_fd, recv_packet,sizeof(recv_packet), 0, (struct sockaddr *)&dest_addr, (socklen_t *)&recv_len);

		ip = (struct ip *)recv_packet;
		ip_head_len = ip->ip_hl << 2;
		icmp = (struct icmp *)(recv_packet + ip_head_len);
		if(icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == pid)
		{
			printf("receive ok !\n");
			close(sock_fd);
			return 0;
		} 
		else
		{
			close(sock_fd);
			return -1;
		}
	}
}

int get_ip_addr(char *name,char *net_ip)
{
	struct ifreq ifr;
	int ret = 0;
	int fd;	
	
	strcpy(ifr.ifr_name, name);
	ifr.ifr_addr.sa_family = AF_INET;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
	{
		return -1;
	}
		
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) 
	{
		close(fd);
		return -1;
	}
	
	strcpy(net_ip,inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

	close(fd);

	return	0;
}

int get_mask_addr(char *name,char *net_mask)
{
	struct ifreq ifr;
	int ret = 0;
	int fd;	
	
	strcpy(ifr.ifr_name, name);
	ifr.ifr_addr.sa_family = AF_INET;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ret = -1;
		
	if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) 
	{
		ret = -1;
	}
	
	strcpy(net_mask,inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

	close(fd);

	return	ret;
}

int get_brdcast_addr(char *name,char *net_brdaddr)
{
	struct ifreq ifr;
	int ret = 0;
	int fd;	
	
	strcpy(ifr.ifr_name, name);
	ifr.ifr_addr.sa_family = AF_INET;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ret = -1;
		
	if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) 
	{
		ret = -1;
	}
	
	strcpy(net_brdaddr,inet_ntoa(((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr));

	close(fd);

	return	ret;
}

int set_ip_addr(char *name,char *net_ip) /* name : eth0 eth1 lo and so on */
{	
	struct sockaddr	addr;
	struct ifreq ifr;
	char gateway_addr[32];
	int ret = 0;
	int fd;	
	
	((struct sockaddr_in *)&(addr))->sin_family = PF_INET;
	((struct sockaddr_in *)&(addr))->sin_addr.s_addr = inet_addr(net_ip);

	ifr.ifr_addr = addr;
	strcpy(ifr.ifr_name,name);

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ret = -1;
		
	get_gateway_addr(gateway_addr);

	if (ioctl(fd, SIOCSIFADDR, &ifr) != 0) 
	{
		ret = -1;
	}

	close(fd);

	set_gateway_addr(gateway_addr);

	return	ret;
}

int set_mask_addr(char *name,char *mask_ip) /* name : eth0 eth1 lo and so on */
{	
	struct sockaddr	addr;
	struct ifreq ifr;
	char gateway_addr[32];
	int ret = 0;
	int fd;	
	
	((struct sockaddr_in *)&(addr))->sin_family = PF_INET;
	((struct sockaddr_in *)&(addr))->sin_addr.s_addr = inet_addr(mask_ip);
	ifr.ifr_netmask = addr;
	strcpy(ifr.ifr_name,name);

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ret = -1;
		
	get_gateway_addr(gateway_addr);

	if (ioctl(fd, SIOCSIFNETMASK, &ifr) != 0) 
	{
		ret = -1;
	}

	close(fd);

	set_gateway_addr(gateway_addr);

	return	ret;
}

int get_gateway_addr(char *gateway_addr)
{
	char buff[256];
	int  nl = 0 ;
	struct in_addr gw;
	int flgs, ref, use, metric;
	unsigned long int d,g,m;
	unsigned long addr;
	unsigned long *pgw = &addr;

	FILE	 *fp = NULL;
	
	if (pgw == NULL)
		return -1;
		
	*pgw = 0;

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
	{
		return -1;
	}
		
	nl = 0 ;
	while( fgets(buff, sizeof(buff), fp) != NULL ) 
	{
		if(nl) 
		{
			int ifl = 0;
			while(buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
				ifl++;
			buff[ifl]=0;    /* interface */
			if(sscanf(buff+ifl+1, "%lx%lx%X%d%d%d%lx",
				   &d, &g, &flgs, &ref, &use, &metric, &m)!=7) 
			{
				//continue;
				fclose(fp);
				return -2;
			}

			ifl = 0;        /* parse flags */
			if(flgs&RTF_UP) 
			{			
				gw.s_addr   = g;
					
				if(d==0)
				{
					*pgw = g;		
					strcpy(gateway_addr, inet_ntoa(gw));
					fclose(fp);
					return 0;
				}				
			}
		}
		nl++;
	}	
	
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	
	return	-1;
}

int get_gateway_addr_ext(char *device, char *gateway_addr)
{
	char buff[256];
	int  nl = 0 ;
	struct in_addr gw;
	int flgs, ref, use, metric;
	unsigned long int d,g,m;
	unsigned long addr;
	unsigned long *pgw = &addr;

	FILE *fp = NULL;
	
	if (device == NULL)
	{
		return -1;
	}

	if (pgw == NULL)
	{
		return -1;
	}
	
	*pgw = 0;

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
	{
		return -1;
	}
		
	nl = 0 ;
	while( fgets(buff, sizeof(buff), fp) != NULL ) 
	{
		if(nl) 
		{
			int ifl = 0;
			while(buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
				ifl++;
			buff[ifl]=0;    // interface

			if (strcmp(buff, device) != 0)
			{
				continue;
			}
			
			if(sscanf(buff+ifl+1, "%lx%lx%X%d%d%d%lx", &d, &g, &flgs, &ref, &use, &metric, &m)!=7) 
			{
				//continue;
				fclose(fp);
				return -2;
			}

			if (flgs == 3)
			{
				gw.s_addr = g;
				*pgw = g;		
				strcpy(gateway_addr, inet_ntoa(gw));
				fclose(fp);

				printf("get_gateway_addr_ext(%s %s)\n", device, gateway_addr);

				return 0;
			}
		}
		nl++;
	}	
	
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	
	return	0;
}

int set_gateway_addr(char *gateway_addr)
{
	char old_gateway_addr[32];
	struct rtentry rt;
	unsigned long gw;
	int fd;
	int ret = 0;
	
	get_gateway_addr(old_gateway_addr);
	del_gateway_addr(old_gateway_addr);
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		ret = -1;
	}	

	gw = inet_addr(gateway_addr);
	memset((char *) &rt, 0, sizeof(struct rtentry));

	((struct sockaddr_in *)&(rt.rt_dst))->sin_addr.s_addr = 0;
	
	rt.rt_flags = RTF_UP | RTF_GATEWAY ;
	//rt.rt_flags = 0x03;

	((struct sockaddr_in *)&(rt.rt_dst))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_addr.s_addr = gw;	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_addr.s_addr = 0;
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_family = PF_INET;
	rt.rt_dev = NULL;
		
	if (ioctl(fd, SIOCADDRT, &rt) < 0) 	
	{	
		ret = -1;
	}
	close(fd);
	
	return	ret;
}

int set_gateway_addr_ext(char *device, char *gateway_addr)
{
	char old_gateway_addr[32];
	struct rtentry rt;
	unsigned long gw;
	int fd;
	int ret = 0;
	
	get_gateway_addr_ext(device, old_gateway_addr);
	del_gateway_addr(old_gateway_addr);
	
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{
		ret = -1;
	}	

	gw = inet_addr(gateway_addr);
	memset((char *) &rt, 0, sizeof(struct rtentry));

	((struct sockaddr_in *)&(rt.rt_dst))->sin_addr.s_addr = 0;
	
	rt.rt_flags = RTF_UP | RTF_GATEWAY ;
	//rt.rt_flags = 0x03;

	((struct sockaddr_in *)&(rt.rt_dst))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_addr.s_addr = gw;	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_addr.s_addr = 0;
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_family = PF_INET;
	rt.rt_dev = NULL;
		
	if (ioctl(fd, SIOCADDRT, &rt) < 0) 	
	{	
		ret = -1;
	}
	close(fd);
	
	return	ret;
}

int del_gateway_addr(char *gateway_addr)
{
	struct rtentry rt;
	unsigned long gw;
	int ret = 0;
	int fd;
	
	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		ret = -1;
	
	gw = inet_addr(gateway_addr);
	memset((char *) &rt, 0, sizeof(struct rtentry));

	rt.rt_flags = RTF_UP | RTF_GATEWAY ;
	
	((struct sockaddr_in *)&(rt.rt_dst))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_addr.s_addr = gw;	
	((struct sockaddr_in *)&(rt.rt_gateway))->sin_family = PF_INET;
	
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_addr.s_addr = 0;
	((struct sockaddr_in *)&(rt.rt_genmask))->sin_family = PF_INET;
	
	rt.rt_dev = NULL;
		
	if (ioctl(fd, SIOCDELRT, &rt) < 0) 	
	{
		ret = -1;
	}
	
	close(fd);

	return	ret;
}

int get_specific_gateway(char *device, char *gateway_addr)
{
	char buff[256];
	int  nl = 0 ;
	struct in_addr gw;
	int flgs, ref, use, metric;
	unsigned long int d,g,m;
	unsigned long addr;
	unsigned long *pgw = &addr;

	FILE *fp = NULL;
	
	if (device == NULL)
	{
		return -1;
	}

	if (pgw == NULL)
	{
		return -1;
	}
	
	*pgw = 0;

	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
	{
		return -1;
	}
		
	nl = 0 ;
	while( fgets(buff, sizeof(buff), fp) != NULL ) 
	{
		if (nl) 
		{
			int ifl = 0;
			
			while(buff[ifl]!=' ' && buff[ifl]!='\t' && buff[ifl]!='\0')
			{
				ifl++;
			}
			buff[ifl]=0;    /* interface */

			if (strcmp(buff, device) != 0)
			{
				continue;
			}

			if(sscanf(buff+ifl+1, "%lx%lx%X%d%d%d%lx", &d, &g, &flgs, &ref, &use, &metric, &m)!=7) 
			{
				//continue;
				fclose(fp);
				return -2;
			}

			if (d != 0)
			{
				gw.s_addr = d;
				*pgw = d;		
				strcpy(gateway_addr, inet_ntoa(gw));
				fclose(fp);
				return 0;
			}
		}
		nl++;
	}	
	
	if(fp)
	{
		fclose(fp);
		fp = NULL;
	}
	
	return	-1;
}

// [ZHB][2005-11-09]
int get_net_phyaddr(char *name, char *addr)  // name : eth0 eth1 lo and so on 
{											// addr : 12:13:14:15:16:17
	int 		sockfd;		
	struct 	ifreq ifr;
	char 	buff[24];
	
	/* Create a channel to the NET kernel. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{		
		return -1;
	}

	/* get net physical address */
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
	{		
		return -1;
	}
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
	{
		return -1;	
	}
	else
	{
		memcpy(buff, ifr.ifr_hwaddr.sa_data, 8);
		snprintf(addr, sizeof(buff), "%02X:%02X:%02X:%02X:%02X:%02X",
			 (buff[0] & 0377), (buff[1] & 0377), (buff[2] & 0377),
			 (buff[3] & 0377), (buff[4] & 0377), (buff[5] & 0377));
		
		printf("HW address: %s\n", addr);
		
		return  0;
	}
}

int get_net_phyaddr_ext(char *name, char *addr)		// name : eth0 eth1 lo and so on 
{													// addr : 121314151617
	int sockfd;		
	struct ifreq ifr;
	
	// Create a channel to the NET kernel
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
	{		
		return -1;
	}

	// get net physical address
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0)
	{		
		return -1;
	}
	strcpy(ifr.ifr_name, name);
	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
	{
		return -1;	
	}
	else
	{
		memcpy(addr, ifr.ifr_hwaddr.sa_data, 6);

		printf("HW address: %x:%x:%x:%x:%x:%x\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
		
		return  0;
	}
}


int set_dns_addr(char *address)
{
	FILE *fp = NULL;
	char buffer[16];
	
	if (address != NULL)
	{
		memcpy(buffer, address, sizeof(buffer));
	}
	else
	{
		return -1;
	}

	fp = fopen("/etc/resolv.conf", "w+");
	if (fp == NULL)
	{
		return -1;
	}		

	fprintf(fp, "nameserver %s", buffer);
	fclose(fp);	
	
	return 0;		
}

int add_dns_addr(char *address1, char *address2)
{
	int len = 0;
	FILE *fp = NULL;
	char buffer[128];
	
	printf("add_dns_addr: %s %s\n", address1, address2);

	if (address1 == NULL && address2 == NULL)
	{
		return -1;
	}

	fp = fopen("/etc/resolv.conf", "w+");
	if (fp == NULL)
	{
		return -1;
	}		

	if (address1)
	{
		memset(buffer, 0, 128);
		len = sprintf(buffer, "nameserver %s\n", address1);
		fwrite(buffer, 1, len, fp);
	}
	if (address2)
	{
		memset(buffer, 0, 128);
		len = sprintf(buffer, "nameserver %s\n", address2);
		fwrite(buffer, 1, len, fp);
	}

	fclose(fp);	
	
	return 0;		
}
