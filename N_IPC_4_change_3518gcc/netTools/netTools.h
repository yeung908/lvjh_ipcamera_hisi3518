#ifndef __NET_TOOLS_H_
#define __NET_TOOLS_H_

int send_icmp_packet(char *addr);
int recv_icmp_packet();

int get_gateway_addr(char *gateway_addr);
int get_gateway_addr_ext(char *device, char *gateway_addr);
int set_gateway_addr(char *gateway_addr);
int set_gateway_addr_ext(char *device, char *gateway_addr);
int del_gateway_addr(char *gateway_addr);
int get_specific_gateway(char *device, char *gateway_addr);

int get_ip_addr(char *name,char *net_ip);
int get_ip_addr(char *name,char *net_mask);
int set_ip_addr(char *name,char *net_ip);
int set_ip_addr(char *name,char *net_ip);
int get_brdcast_addr(char *name,char *net_brdaddr);

int set_mask_addr(char *name,char *mask_ip);
int get_mask_addr(char *name,char *net_mask);

int ifconfig_up_down(char *name, char *action);

int set_net_phyaddr(char *name,char *addr);
int get_net_phyaddr(char *name, char *addr);

int set_dns_addr(char *address);
int add_dns_addr(char *address1, char *address2);

#endif
