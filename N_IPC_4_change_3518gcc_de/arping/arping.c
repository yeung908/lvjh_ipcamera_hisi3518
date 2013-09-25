#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>

#ifdef ARP_DEBUG
#define arp_err()
#else
#define arp_err()		\
{				\
	printf("%s(%d) - %d , %s \n", __FILE__, __LINE__, errno, strerror(errno)); \
}
#endif

typedef struct
{
	int sockfd;

	int count;
	int singleSentCount;
	int brdSentCount;

	int dadFlag;
	int unicasting;

	struct in_addr srcAddr;
	struct in_addr dstAddr;
	struct sockaddr_ll srcAddrII;
	struct sockaddr_ll dstAddrII;

}ARPHANDLE;

static int ARP_init(ARPHANDLE *handle, char *srcaddr, char *dstaddr, int count);
static int ARP_release(ARPHANDLE arpHandle);
static int send_arp_pack(ARPHANDLE arpHandle);
static int recv_arp_pack(ARPHANDLE arpHandle);

static int ARP_init(ARPHANDLE *arpHandle, char *srcaddr, char *dstaddr, int count)
{
	int ret = -1;
	struct ifreq ifr;
	int ifindex = 0;
	int i = 0;
	char srcAddr[20];
	char dstAddr[20];

	if (arpHandle==NULL || srcaddr==NULL || dstaddr==NULL)
	{
		return -2;
	}
	if (!strcmp(srcaddr, dstaddr))
	{
		return -3;
	}

	memset(arpHandle, 0, sizeof(ARPHANDLE));
	strcpy(srcAddr, srcaddr);
	strcpy(dstAddr, dstaddr);

	ret = socket(PF_PACKET, SOCK_DGRAM, 0);
	if (ret < 0)
	{
		arp_err();
		return -1;
	}
	arpHandle->sockfd = ret;
	
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
	ret = ioctl(arpHandle->sockfd, SIOCGIFINDEX, &ifr);
	if (ret < 0) 
	{
		arp_err();
		return -1;
	}
	ifindex = ifr.ifr_ifindex;
	ret = ioctl(arpHandle->sockfd, SIOCGIFFLAGS, (char *)&ifr);
	if (ret < 0) 
	{
		arp_err();
		return -1;	
	}
	if (!(ifr.ifr_flags & IFF_UP)) 
	{
		arp_err();
		return -1;
	}
	if (ifr.ifr_flags & (IFF_NOARP | IFF_LOOPBACK)) 
	{
		arp_err();
		return -1;
	}
	
	if (!inet_aton(dstAddr, &arpHandle->dstAddr)) 
	{
		struct hostent *ent = NULL;

		ent = gethostbyname2((const char *)dstAddr, AF_INET);
		if (!ent) 
		{
			arp_err();
			return -1;
		}
		//memcpy(&arpHandle->dstAddr, &ent->h_addr, 4);
	}	
	if (!inet_aton(srcAddr, &arpHandle->srcAddr)) 
	{
		arp_err();
		return -1;
	}

	arpHandle->srcAddrII.sll_family = AF_PACKET;
	arpHandle->srcAddrII.sll_ifindex = ifindex;
	arpHandle->srcAddrII.sll_protocol = htons(ETH_P_ARP);
	ret = bind(arpHandle->sockfd, (struct sockaddr *) &arpHandle->srcAddrII, sizeof(arpHandle->srcAddrII));
	if (ret == -1) 
	{
		arp_err();	
		return -1;
	}
	i = sizeof(arpHandle->srcAddrII);
	ret = getsockname(arpHandle->sockfd, (struct sockaddr *) &arpHandle->srcAddrII, &i);
	if (ret == -1) 
	{
		arp_err();
		return -1;
	}
	if (arpHandle->srcAddrII.sll_halen == 0) 
	{
		arp_err();
		return -1;
	}

	arpHandle->dstAddrII = arpHandle->srcAddrII;
	memset(arpHandle->dstAddrII.sll_addr, -1, arpHandle->dstAddrII.sll_halen);

	arpHandle->count = count;
	arpHandle->dadFlag = 0;
	arpHandle->unicasting = 0;	// single

	return 0;
}

static int ARP_release(ARPHANDLE arpHandle)
{
	if (arpHandle.sockfd)
	{
		close(arpHandle.sockfd);
	}
					
	return 0;
}

static int send_arp_pack(ARPHANDLE arpHandle)
{
	int ret = -1;
	unsigned char buf[256];
	struct arphdr *ah = (struct arphdr *) buf;
	unsigned char *p = (unsigned char *) (ah + 1);
	
	ah->ar_hrd = htons(arpHandle.srcAddrII.sll_hatype);
	ah->ar_hrd = htons(ARPHRD_ETHER);
	ah->ar_pro = htons(ETH_P_IP);
	ah->ar_hln = arpHandle.srcAddrII.sll_halen;
	ah->ar_pln = 4;
	ah->ar_op = htons(ARPOP_REQUEST);	// ARPOP_REPLY

	memcpy(p, &arpHandle.srcAddrII.sll_addr, ah->ar_hln);
	p += ah->ar_hln;
	memcpy(p, &arpHandle.srcAddr, 4);
	p += 4;
	memcpy(p, &arpHandle.dstAddrII.sll_addr, ah->ar_hln);	
	p += ah->ar_hln;
	memcpy(p, &arpHandle.dstAddr, 4);
	p += 4;

	ret = sendto(arpHandle.sockfd, buf, p-buf, 0, (struct sockaddr *)&arpHandle.dstAddrII, sizeof(struct sockaddr_ll));
	if (ret == p - buf) 
	{		
		if (arpHandle.count)
		{
			arpHandle.singleSentCount++;
			if (!arpHandle.unicasting)
			{
				arpHandle.brdSentCount++;
			}
		}
	}
	else
	{
		arp_err();
	}
	
	return 0;
}

static int recv_arp_pack(ARPHANDLE arpHandle)
{
	char packet[4096];
	struct arphdr *ah = (struct arphdr *) packet;
	unsigned char *p = (unsigned char *) (ah + 1);
	struct in_addr src_ip, dst_ip;
	
	struct sockaddr_ll from;
	int ret = -1;
	int len = 0;
	struct timeval tv;
	fd_set fdset;
	
	/* set timeout */
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	/* set fd_set */
	FD_ZERO(&fdset);
	FD_SET(arpHandle.sockfd, &fdset);
	
	if ((ret = select(arpHandle.sockfd+1, &fdset, NULL, NULL, &tv)) == -1)
	{	
		return -1;
	}
	else if(ret == 0)
	{		
		return -1;
	}
	else
	{	
		len = sizeof(from);
		ret = recvfrom(arpHandle.sockfd, packet, sizeof(packet), 0, (struct sockaddr *) &from, &len);
		if (ret < 0)
		{
			return -1;
		}
	}

	// Filter out wild packets
	if (from.sll_pkttype != PACKET_HOST &&
		from.sll_pkttype != PACKET_BROADCAST &&
		from.sll_pkttype != PACKET_MULTICAST)
	{	
		return -1;
	}

	// Only these types are recognised
	if (ah->ar_op != htons(ARPOP_REQUEST) && ah->ar_op != htons(ARPOP_REPLY))
	{
		return -1;
	}

	// ARPHRD check and this darned FDDI hack here
	if (ah->ar_hrd != htons(from.sll_hatype) &&
		(from.sll_hatype != ARPHRD_FDDI || ah->ar_hrd != htons(ARPHRD_ETHER)))
	{
		return -1;
	}

	// Protocol must be IP.
	if (ah->ar_pro != htons(ETH_P_IP))
	{	
		return -1;
	}
	if (ah->ar_pln != 4)
	{
		return -1;
	}
	if (ah->ar_hln != arpHandle.srcAddrII.sll_halen)
	{	
		return -1;
	}
	if (len < sizeof(*ah) + 2 * (4 + ah->ar_hln))
	{
		//return -1;
	}
	memcpy(&src_ip, p + ah->ar_hln, 4);
	memcpy(&dst_ip, p + ah->ar_hln + 4 + ah->ar_hln, 4);
	
	if (!arpHandle.dadFlag) 
	{
		if (src_ip.s_addr != arpHandle.dstAddr.s_addr)
		{
			return -1;
		}
		if (arpHandle.srcAddr.s_addr != dst_ip.s_addr)
		{
			return -1;
		}
		if (memcmp(p + ah->ar_hln + 4, &arpHandle.srcAddrII.sll_addr, ah->ar_hln))
		{
			return -1;
		}
	} 
	else 
	{
		if (src_ip.s_addr != arpHandle.dstAddr.s_addr)
		{
			return -1;
		}
		if (memcmp(p, &arpHandle.srcAddrII.sll_addr, arpHandle.srcAddrII.sll_halen) == 0)
		{
			return -1;
		}
		if (arpHandle.srcAddr.s_addr && arpHandle.srcAddr.s_addr != dst_ip.s_addr)
		{
			return -1;
		}
	}
	
	return 0;
}

int NVS_arp(char *srcaddr, char *dstaddr, int count)
{
	int ret = -1;
	int i = count;
	ARPHANDLE arpHandle;

	ret = ARP_init(&arpHandle, srcaddr, dstaddr, count);
	if (ret < 0)
	{
#ifdef ARP_DEBUG
		printf("ARP_init Fail(%d).\n", ret);
#endif
		ARP_release(arpHandle);
		return -1;
	}
	
	while (i > 0)
	{
		ret = send_arp_pack(arpHandle);
		if (ret == 0)
		{
#ifdef ARP_DEBUG
			printf("send_arp_pack OK.\n");
#endif		
		}
		else
		{
#ifdef ARP_DEBUG
			printf("send_arp_pack Fail.\n");
#endif
			ARP_release(arpHandle);
		
			return -1;
		}
		
		ret = recv_arp_pack(arpHandle);
		if (ret == 0)
		{
			ARP_release(arpHandle);
#ifdef ARP_DEBUG
			printf("The IP(%s) is exist.\n", dstaddr);
#endif			
			return 0;
		}
		else
		{
			i--;
		}
	}
	
	ARP_release(arpHandle);
#ifdef ARP_DEBUG
	printf("The IP(%s) is not exist.\n", dstaddr);
#endif

	return -1;
}


