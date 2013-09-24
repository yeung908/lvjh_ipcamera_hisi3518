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

#include "dhcpSetup.h"

int dhcp_setup(char *ethname, char *hostname)
{
	int ret = -1;

	char buffer[512];

	sprintf(buffer,"busybox udhcpc -i %s -H %s -s /mnt/mtd/dvs/etc/udhcpc.script -n -q -t 10", ethname, hostname);

	ret = system(buffer);

	return ret;
}

