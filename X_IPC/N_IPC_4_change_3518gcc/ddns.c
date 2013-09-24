/***************************************************************************************
****************************************************************************************
* FILE      : ddns.c
* Description   : ddns ������Ҫ����ƽ̨�Խ�(�����Ͱ���Ѷ����Դ��51ddns��3322.org)
*
* Copyright (c) 2013 by www.yiview.com. All Rights Reserved.
*
* History:
* Version               Name                        Date                        Description
   0.1              www.yiview.com              2013/07/19              Initial Version

****************************************************************************************
****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <semaphore.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>



#include "param.h"
#include "snapshot.h"

char g_ddns_username[32];
char g_ddns_password[32];
char g_ddns_server[128];
int g_ddns_port = 0;
char g_ddns_auth[32];
char g_ddns_domain[128];
char g_ddns_ip[16];
int g_ddns_type = 0;
int g_register_ddns_flag = 0;

char g_ddns_server_buffer[1024] = {0};
char g_ddns_update_extern_ip[512] = {0};
char g_ddns_check_extern_ip[512] = {0};
char g_device_ddns_extern_ip[512] = {0};
char g_device_upnp_strLocalIp[16] = {0};
char g_ddns_server_return_buffer[512] = {0};

char *g_ddns_snapshot_buffer = NULL;
int   g_ddns_snapshot_buffer_size = 0;




// Add the code by lvjh, 2009-05-13
int g_web_port = 80;

int  g_ddns_flag = 0;

// å¨å±åé
static int g_ddns_run = 0;
static int g_ddns_pause = 0;


static int g_yiyuan_ddns_run = 0;
static int g_yiyuan_ddns_pause = 0;
static int g_yiyuan_ddns_alarm_type = 0;// 2:motion alarm 3:probeIn alarm


#if  1
#define net_debug()\
    {             \
        printf(" ddns error \n");\
    }
#endif


// dyndns
//./ez-ipupdate -i eth0 -S dyndns -h szdemo001.dyndns.org -u jerryzhuang001:12345678
//./ez-ipupdate -i eth0 -S dyndns -h jerryzhuang.dyndns.org -u jerryzhuang001:12345678
//./ez-ipupdate -i eth0 -S qdns -h szdemo001.3322.org -u jerryzhuang001:12345678
//./ez-ipupdate -i eth0 -S qdns -h jerryzhuang.3322.org -u jerryzhuang001:12345678
//./ez-ipupdate -i eth0 -S qdns -s user.ipcam.hk:80 -h guest2.ipcam.hk -u guest2:123
//./ez-ipupdate -i eth0 -S qdns -s user.ipcam.hk:80 -h guest2.ipcam.hk -x 82 -u guest2:123
//./ez-ipupdate.old -i eth0 -S qdns -s user.ipcam.hk:80 -h guest2.ipcam.hk -u guest2:123
//./ez-ipupdate -i eth0 -S qdns -h jerryzhuang.3322.org -x 80 -u jerryzhuang001:12345678
//http://username:password@members.dyndns.org/nic/update?hostname=yourhostname&myip=ipaddress&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG
// busybox wget -O- "http://jerryzhuang001:12345678@members.dyndns.org/nic/update?hostname=jerryzhuang.dyndns.org&myip=183.16.85.48&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG"
// busybox wget -O- "http://tositech:12345678@members.dyndns.org/nic/update?hostname=tositest.dyndns.org"
// busybox wget -O- "http://tositech001:12345678@members.dyndns.org/nic/update?hostname=tosidemo.dyndns.org"

// busybox wget -O- "http://xha1:123456@members.88safe.com/dyndns/update?system=dyndns&hostname=xha1.88safe.com&port=80"
// busybox wget -O- "http://www.88safe.com/myddns/update.asp?username=xha1&userpwd=123456"
// busybox wget -O- "http://www.88safe.com:8009/vipddns/upgengxin.asp?username=xha1&userpwd=123456&userdomain=88safe.com&userport=80"
// busybox wget -O- "http://www.7988.org/myddns/ipcam20100524.asp?username=shenzhen16&userpwd=123456"
// busybox wget -O- "http://guest2:123@members.ipcam.hk/dyndns/update?system=dyndns&hostname=guest2.ipcam.hk&port=80"
// busybox wget -O- "http://guest2:123@members.ns02.towada.net/dyndns/update?system=dyndns&hostname=guest2.towada.net&port=80"
// busybox wget -O- "http://guest2:123@ns02.towada.net/dyndns/update?system=dyndns&hostname=guest2.towada.net&port=80"


char *base64_encode(const char *cptr,char **rptr)

{

    char *res;

    int i,clen,len;

    len=strlen(cptr);

    clen=len/3;

    if(cptr==NULL||(res=malloc(clen+3*2+len))==NULL)

        return NULL;

    for(*rptr=res; clen--;)

    {

        *res++=*cptr>>2&0x3f;          /*È¡ptr¸ß6Î»·ÅÈëresµÍ6Î»*/

        *res=*cptr++<<4&0x30;          /*ÒÆ¶¯ptr×îµÍ2Î»µ½¸ß6Î»È»ºóÇå0Æä ËüÎ»*/

        *res++|=*cptr>>4;                  /*È¡ptr¸ß4Î»¸øresµÍ4Î»*/

        *res=(*cptr++&0x0f)<<2;        /*È¡ptrµÍ4Î»ÒÆ¶¯µ½¸ß6Î»*/

        *res++|=*cptr>>6;                  /*È¡ptr¸ß2Î»¸øresµÍ2Î»*/

        *res++=*cptr++&0x3f;

    }

    if(i=len%3)                                       /*´¦Àí¶àÓà×Ö·ûÖ»ÓÐÁ½ÖÖÇé¿ö¶àÒ»¸ö»òÕßÁ½¸ö×Ö·û*/

    {

        if(i==1)                                    /*¸ù¾Ýbase64±àÂë²¹=ºÅ*/

        {

            *res++=*cptr>>2&0x3f;

            *res++=*cptr<<4&0x30;

            *res++='=';

            *res++='=';

        }

        else

        {

            *res++=*cptr>>2&0x3f;

            *res=*cptr++<<4&0x30;

            *res++|=*cptr>>4;

            *res++=(*cptr&0x0f)<<2;

            *res++='=';

        }

    }

    *res='=';                                               /*±£Ö¤×îºó½áÎ»Îª=½áÊøÔ­ÒòÊÇÒòÎªbase64ÀïÓÐÎª0µÄ±àÂë*/

    for(res=*rptr; *res!='='; res++)

        *res="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="[*res];

    rptr[0][strlen(*rptr)-1]='\0';                 /*È¥µô×îºóÒ»¸ö=ºÅ*/

    return *rptr;

}


int ddns_TcpSockListen(const char* ip, int port)
{
    int ret = -1;
    int hSock = -1;
    int opt = -1;
    socklen_t len = 0;
    struct sockaddr_in addr;
    struct hostent *host = NULL;

    // ÅÐ¶ÏÊäÈë²ÎÊýµÄºÏ·¨ÐÔ
    if (ip<0 || port<0) {
        return -1;
    }
    //ulServer = inet_addr(pSerIP);
    if ((host=gethostbyname(ip)) == NULL) {
        return -1;
    }

    // ´´½¨SOCKET
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(ip);
    addr.sin_port = htons(port);
    hSock = socket(AF_INET, SOCK_STREAM, 0);
    if (hSock < 0) {
        net_debug();
        return -1;
    }

    // ÉèÖÃSOCKETµÄÊôÐÔ
    do {
        opt = 1;
        ret = setsockopt(hSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (ret < 0) {
            net_debug();
            break;
        }
        opt = 1;
        ret = setsockopt(hSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        if (ret < 0) {
            net_debug();
            break;
        }
        // ÉèÖÃ·¢ËÍBUFFERµÄ´óÐ¡
        opt = 1024*512;
        ret = setsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));
        if (ret < 0) {
            net_debug();
            break;
        }
        // ÉèÖÃ½ÓÊÕBUFFERµÄ´óÐ¡
        opt = 1024*512;
        ret = setsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
        if (ret < 0) {
            net_debug();
            break;
        }
        // »ñÈ¡·¢ËÍBUFFERµÄ´óÐ¡
        opt = sizeof(len);
        ret = getsockopt(hSock, SOL_SOCKET, SO_SNDBUF, &len, (socklen_t *)&opt);
        if (ret < 0) {
            net_debug();
            break;
        }
        // »ñÈ¡½ÓÊÕBUFFERµÄ´óÐ¡
        opt = sizeof(len);
        ret = getsockopt(hSock, SOL_SOCKET, SO_RCVBUF, &len, (socklen_t *)&opt);
        if (ret < 0) {
            net_debug();
            break;
        }
        // Add the code by lvjh, 2008-04-17
        opt = 0;
        ret = setsockopt(hSock, IPPROTO_IP, IP_MULTICAST_LOOP, &opt, sizeof(opt));

        // °ó¶¨SOCKET
        ret = bind(hSock, (struct sockaddr *)&addr, sizeof(addr));
        if (ret < 0) {
            net_debug();
            break;
        }
        // ¼àÌý
        ret = listen(hSock, 10);
        if (ret < 0) {
            net_debug();
            break;
        }
    } while(FALSE);

    // Èç¹ûSOCKETÉèÖÃÊ§°Ü£¬Ôò¹Ø±ÕSOCKET
    if (ret < 0) {
        shutdown(hSock, 2);
        close(hSock);
        return -1;
    }

    return hSock;
}



int DdnsNatConnect(const char*pSerIP, int inPort, int *sock)
{
    int nRet = 0;
    int hSocket = 0;
    int nOpt = 1;
    int error;
    struct hostent *host = NULL;
    struct sockaddr_in addr;
    unsigned long ulServer;

    unsigned long non_blocking = 1;
    unsigned long blocking = 0;



    //ulServer = inet_addr(pSerIP);
    if ((host=gethostbyname(pSerIP)) == NULL) {
        return -1;
    }


    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket <= 0) {
        return -1;
    }


    memset(&addr, 0 ,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)inPort);
    //addr.sin_addr.s_addr = ulServer;
    addr.sin_addr = *((struct in_addr *)host->h_addr);

    //non_blocking = 1;
    //ioctl(hSocket, FIONBIO, &non_blocking);
    //printf("connecting ...........\n");
    nRet = connect(hSocket, (struct sockaddr*)&addr, sizeof(addr));
    if (nRet == -1) {
        struct timeval tv;
        fd_set writefds;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        FD_ZERO(&writefds);
        FD_SET(hSocket, &writefds);

        if (select(hSocket+1, NULL, &writefds, NULL, &tv) != 0) {
            if (FD_ISSET(hSocket,&writefds)) {
                int len=sizeof(error);
                if (getsockopt(hSocket, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0) {
                    goto error_ret;
                }

                if (error != 0) {
                    goto error_ret;
                }
            } else {
                goto error_ret;
            }
        } else {
            goto error_ret;
        }

        blocking = 0;
        ioctl(hSocket, FIONBIO, &blocking);
        nRet = 0;
    }

    if (nRet != 0) {
        goto error_ret;
    }
    blocking = 0;
    ioctl(hSocket, FIONBIO, &blocking);

    *sock = hSocket;

    return 0;

error_ret:
    printf("NAT: Close Socket %d\n", hSocket);
    net_debug();
    shutdown(hSocket, 2);
    close(hSocket);

    return -1;
}


int DdnsRemoteRequestProc(int sockfd)
{
    int hConnSock = -1;
    int ret = -1;
    int len = -1;
    int level;
    int nCurNum = 0;

    fd_set fset;
    struct timeval to;
    struct sockaddr_in addr;
    char buffer[1024];

    if (sockfd <= 0 || sockfd > 65535) {
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    bzero(&to, sizeof(to));

    len = sizeof(addr);
    hConnSock = sockfd;

    //g_tcp_nat_keepalive_count++;

    // Setup attribute of the connected socket
    SetConnSockAttr(hConnSock, 200);

    //
    FD_ZERO(&fset);
    FD_SET(hConnSock, &fset);
    to.tv_sec = 200;
    to.tv_usec = 0;

    while(1) {
        ret = select(hConnSock+1, &fset, NULL, NULL, &to);
        if ( ret == 0) {
            net_debug();
            return 0;
        }
        if (ret<0) {
            net_debug();
            return 0;
        }
        if (ret < 0) {
            printf("NAT(%d): Select Error!\n", hConnSock);
            net_debug();
            return -1;
        }
        if (!FD_ISSET(hConnSock, &fset)) {
            printf("NAT(%d): FD_ISSET Error!\n", hConnSock);
            net_debug();
            return -1;
        }

        // Receive network protocal header
        ret = recv(hConnSock, &buffer, 1024 , 0);
        if (ret <= 0) {
            printf("NAT(%d): Recv Error!\n", hConnSock);
            net_debug();
            return -1;
        }
        //mcpy(&netHead, &buffer, sizeof(NET_HEAD));
        printf("buffer = %s\n", buffer);

    }//while(1);

    return ret;

Error:
    net_debug();
    shutdown(hConnSock, 2);
    close(hConnSock);
    return -1;
}

static int set_dns_addr(char *ip)
{
    int ret = -1;
    FILE *fp = NULL;

    if (NULL == ip) {
        return -1;
    }

    fp = fopen("/etc/resolv.conf", "w");
    if (NULL == fp) {
        return -1;
    }

    fprintf(fp, "nameserver %s\n", ip);
    fclose(fp);

    fp = fopen("/etc/ppp/resolv.conf", "w");
    if (NULL == fp) {
        return -1;
    }

    fprintf(fp, "nameserver %s", ip);
    fclose(fp);

    return 0;
}

int SendMsgToYiYuanDdnsServer(int ddns_type)
{
    int ret = 0;
    int sock = 0;
    struct sockaddr_in addr;

    NET_PARAM param;
    SYS_INFO  sys_param;
    DDNS_PARAM ddns_param;
    OSD_PARAM osd_param;
    YIYUAN_ALARM_DDNS_PARAM yiyuan_alarm_ddns_param;
    PROBE_IN_ALARM_PARAM probe_in_alarm_param;

    char buffer[100*1024] = {0};
    char ddns_send_buffer[100*1024] = {0};
    char temp[100*1024] = {0};
    char user_base64_buffer[100] = {0};
    char passwd_base64_buffer[100] = {0};

    getSysInfoParam(&sys_param);
    getDDNSParam(&ddns_param);
    getOsdParam(0, &osd_param);
    getProbeInAlarmParam(0, &probe_in_alarm_param);
    getYiyuanAlarmDDNSParam(&yiyuan_alarm_ddns_param);
    printf("yiyuan_alarm_ddns_param :: %d, %d, %d, %s\n", yiyuan_alarm_ddns_param.nOnFlag, yiyuan_alarm_ddns_param.nPort, yiyuan_alarm_ddns_param.nTime, yiyuan_alarm_ddns_param.sIpAddr);
    if(yiyuan_alarm_ddns_param.nOnFlag == 0)
        return 0;

    if((ddns_type == 2)|| (ddns_type == 3)) {
        videoJpegSnapShot(0, 0);
        get_snapshot_buffer_size(g_ddns_snapshot_buffer, &g_ddns_snapshot_buffer_size);

        encode_base64(g_ddns_snapshot_buffer, temp, g_ddns_snapshot_buffer_size);
        encode_base64(ddns_param.userName, user_base64_buffer, strlen(ddns_param.userName));
        encode_base64(ddns_param.userPsw, passwd_base64_buffer, strlen(ddns_param.userPsw));
        printf("g_ddns_snapshot_buffer_size::%d\n\n", g_ddns_snapshot_buffer_size);
    }

    switch(ddns_type) {
        case 1:
            memset(buffer, 0, 1024);
            sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n%s\r\n",
                    "GET /index.php? HTTP/1.1",
                    "Accept: */*",
                    "Accept-Language: zh-cn",
                    "User-Agent: Mozilla/4.0 HOSS MEGA IPCAMERA",
                    "Content-Type: application/x-www-form-urlencoded",
                    "Host: ip.camanywhere.net",
                    "Content-Length: 5",
                    "Connection: keep-alive",
                    "Cache-Control: no-cache","t=879");
            strcpy(g_ddns_server, "dns.camanywhere.com");
            g_ddns_port = 80;
            break;
        case 2:
            memset(buffer, 0, 1024*100);
            if(osd_param.TitleOSD[0].bShow == 0) {
                sprintf(ddns_send_buffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "username=", user_base64_buffer, "&password=", passwd_base64_buffer, "&ip=", yiyuan_alarm_ddns_param.sIpAddr, "&Hostname=", ddns_param.domain, "&MyIp=", g_ddns_check_extern_ip, "&message=ͨ��1|�ƶ����||||||||&mtype=YW", "&image=", temp);
                sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s%d\r\n%s\r\n%s\n\n%s",
                        "POST /action.do?action=send HTTP/1.1",
                        "Accept: */*",
                        "Accept-Language: zh-cn",
                        "User-Agent: Mozilla/5.0 (compatible; MSIE9.0; Windows NT 6.1; Trident/5.0)",
                        "Content-Type: application/x-www-form-urlencoded",
                        "Host: 218.87.254.137:7000","Content-Length:",strlen(ddns_send_buffer),"Connection: Keep-Alive","Cache-Control: no-cache",ddns_send_buffer);
            } else {
                sprintf(ddns_send_buffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "username=", user_base64_buffer, "&password=", passwd_base64_buffer, "&ip=", yiyuan_alarm_ddns_param.sIpAddr, "&Hostname=", ddns_param.domain, "&MyIp=", g_ddns_check_extern_ip,  "&message=", osd_param.TitleOSD[0].sTitle,"|�ƶ����||||||||&mtype=YW","&image=", temp);
                sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s%d\r\n%s\r\n%s\n\n%s",
                        "POST /action.do?action=send HTTP/1.1",
                        "Accept: */*",
                        "Accept-Language: zh-cn",
                        "User-Agent: Mozilla/5.0 (compatible; MSIE9.0; Windows NT 6.1; Trident/5.0)",
                        "Content-Type: application/x-www-form-urlencoded",
                        "Host: 218.87.254.137:7000","Content-Length:",strlen(ddns_send_buffer),"Connection: Keep-Alive","Cache-Control: no-cache",ddns_send_buffer);
            }
            strcpy(g_ddns_server, yiyuan_alarm_ddns_param.sIpAddr);
            g_ddns_port = yiyuan_alarm_ddns_param.nPort;
            printf("%1000.800s\n", buffer);
            printf("####g_ddns_server = %s g_ddns_port	= %d\n", g_ddns_server, g_ddns_port);
            break;

        case 3:
            memset(buffer, 0, 1024*100);
            if(strlen(probe_in_alarm_param.probeName) == 0) {
                sprintf(ddns_send_buffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s", "username=", user_base64_buffer, "&password=", passwd_base64_buffer, "&ip=", yiyuan_alarm_ddns_param.sIpAddr, "&Hostname=", ddns_param.domain, "&MyIp=", g_ddns_check_extern_ip, "&message=ͨ��1|̽ͷ����||||||||&mtype=YW", "&image=", temp);
                sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s%d\r\n%s\r\n%s\n\n%s",
                        "POST /action.do?action=send HTTP/1.1",
                        "Accept: */*",
                        "Accept-Language: zh-cn",
                        "User-Agent: Mozilla/5.0 (compatible; MSIE9.0; Windows NT 6.1; Trident/5.0)",
                        "Content-Type: application/x-www-form-urlencoded",
                        "Host: 218.87.254.137:7000","Content-Length:",strlen(ddns_send_buffer),"Connection: Keep-Alive","Cache-Control: no-cache",ddns_send_buffer);
            } else {
                sprintf(ddns_send_buffer, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "username=", user_base64_buffer, "&password=", passwd_base64_buffer, "&ip=", yiyuan_alarm_ddns_param.sIpAddr, "&Hostname=", ddns_param.domain, "&MyIp=", g_ddns_check_extern_ip,  "&message=", probe_in_alarm_param.probeName, "|̽ͷ����||||||||&mtype=YW","&image=", temp);
                sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s%d\r\n%s\r\n%s\n\n%s",
                        "POST /action.do?action=send HTTP/1.1",
                        "Accept: */*",
                        "Accept-Language: zh-cn",
                        "User-Agent: Mozilla/5.0 (compatible; MSIE9.0; Windows NT 6.1; Trident/5.0)",
                        "Content-Type: application/x-www-form-urlencoded",
                        "Host: 218.87.254.137:7000","Content-Length:",strlen(ddns_send_buffer),"Connection: Keep-Alive","Cache-Control: no-cache",ddns_send_buffer);
            }
            strcpy(g_ddns_server, yiyuan_alarm_ddns_param.sIpAddr);
            g_ddns_port = yiyuan_alarm_ddns_param.nPort;
            printf("%1000.500s\n", buffer);
            printf("####g_ddns_server = %s g_ddns_port	= %d\n", g_ddns_server, g_ddns_port);
            break;
        defaule:
            break;
    }

#if 1
    //printf(" %s send buffer = %s\n", __func__, buffer);
    ret = DdnsNatConnect(g_ddns_server, g_ddns_port, &sock);
    if (ret < 0) {
        printf("DDNS tcpSendMsg()(%d): connect Error!\n\n\n", sock);

        //����ddns ״̬
        ret = getDDNSParam(&ddns_param);
        if (ret < 0) {
            return -1;
        }

        ddns_param.reserve = 0;
        g_ddns_pause = 0;

        ret = setDDNSParam(&ddns_param);
        return -1;
    }
    ret = tcpSendMsg(sock, buffer, strlen(buffer));
    if (ret <= 0) {
        printf("DDNS tcpSendMsg()(%d): Send Error!\n\n\n", sock);
        goto Error;
    }
    switch(ddns_type) {
        case 1:
            memset(g_ddns_update_extern_ip, 0, 512);
            ret = recv(sock, g_ddns_update_extern_ip, 1024, 0);
            if(ret < 0) {

                printf("device recv ddns server error %d\n", ret);

                goto Error;
            }
            //printf("g_ddns_update_extern_ip = %s\n\n", g_ddns_update_extern_ip);

            char *g_extern_ip = strtok(g_ddns_update_extern_ip, "\n\n");
            int i;
            for(i = 0; i < 14; i++) {
                g_extern_ip = strtok(NULL, "\n");
                memset(g_ddns_check_extern_ip, 0, 512);
                strcpy(g_ddns_check_extern_ip, g_extern_ip);
            }
            printf("g_extern_ip  = %s\n", g_ddns_check_extern_ip);
            break;
        case 2:
        case 3:
            printf("sock = %d g_ddns_check_extern_ip = %s\n", sock, g_ddns_check_extern_ip);
            memset(g_ddns_server_return_buffer, 0, 512);
            ret = recv(sock, g_ddns_server_return_buffer, 512, 0);
            if(ret < 0) {

                printf("device recv ddns server error\n");

                goto Error;
            }
            if(strstr(g_ddns_server_return_buffer,"good")||strstr(g_ddns_server_return_buffer,"nochg"))
                printf("good or nochg\n");
            else {
                printf("badauth \n");
                goto Error;
            }
            printf("g_ddns_server_return_buffer = %s\n", g_ddns_server_return_buffer);
            break;
        default :
            break;
    }
    close(sock);
#endif
    //����ddns ״̬
    ret = getDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }

    ddns_param.reserve = 1;
    ret = setDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }

    return 0;

Error:
    //����ddns ״̬
    close(sock);
    ret = getDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }
    ddns_param.reserve = 0;
    ret = setDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }
    return -1;
}


int SendMsgToDdnsServer(int ddns_type)
{
    int ret = 0;
    int sock = 0;
    char buffer[1024] = {0};
    char tmp_buffer[100] = {0};
    char base64_buffer[100] = {0};
    char ddns_buffer[1024] = {0};
    struct sockaddr_in addr;
    NET_PARAM param;
    SYS_INFO  sys_param;
    DDNS_PARAM ddns_param;
    DVSNET_UPNP_PARAM upnp_param;
    DVSNET_UPNP_PARAM ddns_upnp_port_param;
    DVSNET_RTSP_PARAM rtsp_param;
    WIFI_PARAM wifiParam;
    USER_INFO_PARAM userInfo;

    getRtspParam(&rtsp_param);
    getNetParam(&param);
    getSysInfoParam(&sys_param);
    getUserInfoParam(&userInfo);
    getDDNSParam(&ddns_param);
    getWifiParam(&wifiParam);


    //ÅÐ¶ÏÓ³Éäupnp ¶Ë¿Ú
    ddns_upnp_port_param.upnpDataPort = param.wServerPort;
    ddns_upnp_port_param.upnpWebPort = param.wWebPort;
    ddns_upnp_port_param.upnpRtspPort = rtsp_param.nRtspPort;
    switch(ddns_type) {
        case 0:
            //51ddns ×¢²áÐÅÏ¢
            sprintf(buffer, "%s%s%s%s%s%s%s%d%s%d%s%s%s%s%s", "GET /action.php?action=3&username=",g_ddns_username, "&password=", g_ddns_password,"&name=",sys_param.strDeviceName,"&port=",param.wWebPort,"&dport=", param.wServerPort,"&macaddr=80-20-00-80-00-04&snno=8000E11070243700&hostname=",g_ddns_domain, "&myip=", param.byServerIp,"\r\n\r\n");
            break;

        case 1:
            //ºÍ°îÊÓÑ¶DDNS
            memset(ddns_buffer, 0, 1024);
            memset(buffer, 0, 1024);
            if(wifiParam.nOnFlag&&(wifiParam.Reserve == 0))strcpy(param.byServerIp, wifiParam.byServerIp);
            printf("ddns register ip %s::%s\n", param.byServerIp, wifiParam.byServerIp);
            sprintf(ddns_buffer, "%s%s%s%s%s%s%s%d%s%d%s%d%s%s%s", "resultok=OK&dev_type=YW&user_name=",userInfo.Admin.strName, "&user_pwd=",userInfo.Admin.strPsw,"&dev_name=Megaipc&dev_ip=",param.byServerIp, "&web_port=",ddns_upnp_port_param.upnpWebPort,"&rtsp_port=",ddns_upnp_port_param.upnpRtspPort ,"&data_port=", ddns_upnp_port_param.upnpDataPort, "&mac_addr=00-E6-01-01-DD-E2&host_name=www.hobcms.com&dev_id=",sys_param.strDeviceID,"&resultok=OK&action=3");
            sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s%d\r\n%s\r\n%s\n\n%s",
                    "POST /action.php HTTP/1.1",
                    "Accept: */*",
                    "Accept-Language: zh-cn",
                    "User-Agent: Mozilla/4.0 HOSS MEGA IPCAMERA",
                    "Content-Type: application/x-www-form-urlencoded",
                    "Host: ip.camanywhere.net","Content-Length:",strlen(ddns_buffer),"Connection: Keep-Alive","Cache-Control: no-cache",ddns_buffer);
            printf("strlen(buffer) = %d\n",strlen(buffer));
            strcpy(g_ddns_server, ddns_param.ipAddr);
            g_ddns_port = ddns_param.port;
            printf("g_ddns_server = %s g_ddns_port  = %d\n", g_ddns_server, g_ddns_port);

            break;

        case 2:
            //ta = rand()%10000;
            memset(buffer, 0, 1024);
            sprintf(buffer, "%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n%s\r\n\r\n%s\r\n",
                    "GET /index.php? HTTP/1.1",
                    "Accept: */*",
                    "Accept-Language: zh-cn",
                    "User-Agent: Mozilla/4.0 HOSS MEGA IPCAMERA",
                    "Content-Type: application/x-www-form-urlencoded",
                    "Host: ip.camanywhere.net",
                    "Content-Length: 5",
                    "Connection: keep-alive",
                    "Cache-Control: no-cache","t=879");
            strcpy(g_ddns_server, "dns.camanywhere.com");
            g_ddns_port = 80;
            printf("g_ddns_server = %s g_ddns_port  = %d\n", g_ddns_server, g_ddns_port);

            break;

        case 3:
            memset(buffer, 0, 1024);
            memset(ddns_buffer, 0, 512);
            memset(base64_buffer, 0, sizeof(char)*100);
            sprintf(tmp_buffer, "%s:%s", ddns_param.userName, ddns_param.userPsw);
            encode_base64(tmp_buffer, base64_buffer, strlen(tmp_buffer));
            sprintf(buffer, "%s%s%s%s%s%d%s%d%s%d%s%s%s%s%s\r\n%s\r\n%s%s\r\n%s\r\n%s\r\n%s\r\n\r\n%s\r\n",
                    "GET /dyndns/update?hostname=", ddns_param.domain,"&myip=", g_ddns_check_extern_ip,"&tcp=",ddns_upnp_port_param.upnpDataPort, "&rtsp=",ddns_upnp_port_param.upnpRtspPort, "&web=",ddns_upnp_port_param.upnpWebPort, "&adm=", userInfo.Admin.strName, "&pwd=", userInfo.Admin.strPsw,"&Cameronline=1||| HTTP/1.1",
                    "Accept: text/*",
                    "Authorization: Basic ", base64_buffer,
                    "Host: 218.87.154.55",
                    "Cache-Control: no-cache",
                    "Content-Length: 4",
                    "4443");
            strcpy(g_ddns_server, ddns_param.ipAddr);
            g_ddns_port = ddns_param.port;
            printf("g_ddns_server = %s g_ddns_port  = %d\n", g_ddns_server, g_ddns_port);
            break;
        defaule:
            break;
    }

    // printf("send buffer = %s\n", buffer);
    ret = DdnsNatConnect(g_ddns_server, g_ddns_port, &sock);
    if (ret < 0) {
        printf("DDNS tcpSendMsg()(%d): connect Error!\n\n\n", sock);

        //ÉèÖÃddns ×´Ì¬
        ret = getDDNSParam(&ddns_param);
        if (ret < 0) {
            return -1;
        }

        ddns_param.reserve = 0;
        g_ddns_pause = 0;

        ret = setDDNSParam(&ddns_param);
        return -1;
    }

    ret = tcpSendMsg(sock, buffer, strlen(buffer));
    if (ret <= 0) {
        printf("DDNS tcpSendMsg()(%d): Send Error!\n\n\n", sock);

        //ÉèÖÃddns ×´Ì¬
        ret = getDDNSParam(&ddns_param);
        if (ret < 0) {
            return -1;
        }

        ddns_param.reserve = 0;
        g_ddns_pause = 0;

        ret = setDDNSParam(&ddns_param);
        close(sock);
        return -1;
    }
    switch(ddns_type) {
        case 0:
            //ret = TcpReceive(sock, g_ddns_server_buffer, 1024);
            ret = recv(sock, g_ddns_server_buffer, 1024, 0);
            if(ret < 0) {
                printf("device recv ddns server error\n");

                goto Error;
            }
            printf("g_ddns_server_buffer = %s\n", g_ddns_server_buffer);
            if(strstr(g_ddns_server_buffer,"0")||strstr(g_ddns_server_return_buffer,"nochg"))
                printf("51ddns success!!\n");
            else {
                printf("51ddns error!!\n");
                goto Error;
            }
            break;

        case 1:
            //ºÍ°îÊÓÑ¶DDNS
            memset(g_ddns_server_buffer, 1024, 0);
            //  ret = TcpReceive(sock, g_ddns_server_buffer, 400);
            ret = recv(sock, g_ddns_server_buffer, 1024, 0);
            if(ret < 0) {

                printf("device recv ddns server error\n");

                goto Error;
            }
            printf("g_ddns_server_buffer = %s:: ret = %d\n\n", g_ddns_server_buffer, ret);
            if(strstr(g_ddns_server_buffer,"success!! "))g_ddns_pause  = 1;
            else
                g_ddns_pause  = 0;

            break;

        case 2:
            memset(g_ddns_update_extern_ip, 0, 512);
            ret = recv(sock, g_ddns_update_extern_ip, 1024, 0);
            if(ret < 0) {

                printf("device recv ddns server error %d\n", ret);

                goto Error;
            }
            char *g_extern_ip = strtok(g_ddns_update_extern_ip, "\n\n");
            int i;
            for(i = 0; i < 14; i++) {
                g_extern_ip = strtok(NULL, "\n");
                memset(g_ddns_check_extern_ip, 0, 512);
                strcpy(g_ddns_check_extern_ip, g_extern_ip);
            }
            break;
        case 3:
            memset(g_ddns_server_return_buffer, 0, 512);
            ret = recv(sock, g_ddns_server_return_buffer, 512, 0);
            if(ret < 0) {

                printf("device recv ddns server error\n");

                goto Error;
            }
            if(strstr(g_ddns_server_return_buffer,"good")||strstr(g_ddns_server_return_buffer,"nochg"))
                printf("good or nochg\n");

            else {
                printf("badauth \n");
                goto Error;
            }
            // printf("g_ddns_server_return_buffer = %s\n", g_ddns_server_return_buffer);
            break;
        default :
            break;
    }

    //ÉèÖÃddns ×´Ì¬
    ret = getDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }

    // printf("g_ddns_pause   = %d\n", g_ddns_pause );
    ddns_param.reserve = 1;
    ret = setDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }
    close(sock);
    return 0;

Error:
    //ÉèÖÃddns ×´Ì¬
    ret = getDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }
    ddns_param.reserve = 0;
    ret = setDDNSParam(&ddns_param);
    if (ret < 0) {
        return -1;
    }
    close(sock);
    return -1;
}

static int ddns_command(char *server, int port, char *user, char *pass, char *domain, int type, int webport)
{
    char buf[512];
    int ret = 0;
    memset(buf, 0, 512);

    switch (type) {
        case 0:     // 3322.org
            sprintf(buf, "busybox wget -O- \"http://%s:%s@members.3322.org/dyndns/update?system=dyndns&hostname=%s\"", user, pass, domain);
            break;

        case 1:     // dyndns.org
            sprintf(buf, "busybox wget -O- \"http://%s:%s@members.dyndns.org/nic/update?hostname=%s\"", user, pass, domain);
            break;

        case 2:     // dhc
            if (strlen(g_ddns_ip) > 8) {
                sprintf(buf,"/mnt/mtd/dvs/app/ez-ipupdate -h %s -S dhs -u %s:%s -a %s ", domain, user, pass, g_ddns_ip);
            } else {
                sprintf(buf,"/mnt/mtd/dvs/app/ez-ipupdate -i ppp0 -h %s -S dhs -u %s:%s", domain, user, pass);
            }
            break;

        case 3:     // 51ddns
            SendMsgToDdnsServer(0);
            break;

        case 4:  //cam any where ddns
            SendMsgToDdnsServer(1);
            break;

        case 5:
            SendMsgToDdnsServer(1);
            break;

        case 6:     // yiyuan ddns
            g_ddns_pause = 0;
            SendMsgToDdnsServer(2);
            SendMsgToDdnsServer(3);
            break;


        case 7:     // dyndns.org
            // Add the code by lvjh, 2011-05-11
            sprintf(buf, "busybox wget -O- \"http://%s:%s@members.dyndns.org/nic/update?hostname=%s\"", user, pass, domain);
            break;

        case 11:        // yiyuan ddns
            g_ddns_pause = 0;
            SendMsgToDdnsServer(2);
            SendMsgToDdnsServer(3);
            break;

        default:
            printf("Can not support ddns type: %d\n", type);
            return -1;
    }

    return ret ;
}

int DDNS_Open()
{
    memset(g_ddns_username, 0, 32);
    memset(g_ddns_password, 0, 32);
    memset(g_ddns_server, 0, 128);
    g_ddns_port = 0;
    memset(g_ddns_auth, 0, 32);
    memset(g_ddns_domain, 0, 128);
    memset(g_ddns_ip, 0, 16);

    return 0;
}

int DDNS_Close()
{
    memset(g_ddns_username, 0, 32);
    memset(g_ddns_password, 0, 32);
    memset(g_ddns_server, 0, 128);
    g_ddns_port = 0;
    memset(g_ddns_auth, 0, 32);
    memset(g_ddns_domain, 0, 128);
    memset(g_ddns_ip, 0, 16);

    return 0;
}

int DDNS_Setup(char *user, char *pass, char *server, int port, char *auth, char *domain, int *type, int webport)
{
    int ret = -1;
    char localIP[32];
    FILE *fp = NULL;

    if (user==NULL || pass==NULL || domain==NULL) {
        return -1;
    }

    if (server != NULL) {
        strncpy(g_ddns_server, server, 128);
    }
    g_ddns_port = port;
    g_web_port = webport;

    strncpy(g_ddns_username, user, 32);
    strncpy(g_ddns_password, pass, 32);
    strncpy(g_ddns_domain, domain, 128);
    if (strlen(localIP) > 8) {
        strncpy(g_ddns_ip, localIP, 16);
    } else {
        memset(g_ddns_ip, 0, 16);
    }
    g_ddns_type = type;

    return 0;
}

int DDNS332_GetSetup(char *user, char *pass, char *server, int *port, char *auth, char *domain, int *type, int *webport)
{
    if (user==NULL || pass==NULL || domain==NULL || type==NULL) {
        return -1;
    }

    strncpy(user, g_ddns_username, 32);
    strncpy(pass, g_ddns_password, 32);
    strncpy(domain, g_ddns_domain, 128);
    *type = g_ddns_type;

    return 0;
}

int DDNS_Start()
{
    int ret = -1;

    ret = ddns_command(g_ddns_server, g_ddns_port, g_ddns_username, g_ddns_password, g_ddns_domain, g_ddns_type, g_web_port);

    return ret;
}

int DDNS_Stop()
{
    return 0;
}



int YiYuanDdnsFun()
{
    int ret = -1;
    DDNS_PARAM ddnsParam;
    char gateway[16];
    char update_gateway[16];
    YIYUAN_ALARM_DDNS_PARAM yiyuan_alarm_ddns_param;
    videoJpegSnapShot(0, 0);

    while(g_yiyuan_ddns_run) {
        getYiyuanAlarmDDNSParam(&yiyuan_alarm_ddns_param);
        //  printf("yiyuan_alarm_ddns_param.nTime = %d\n", yiyuan_alarm_ddns_param.nTime );

        if(g_yiyuan_ddns_pause) {
            usleep(1000*100);
            sleep(5);
            continue;
        }
        getDDNSParam(&ddnsParam);
        ret = DDNS_Open();
        if (ret == 0) {
            DDNS_Setup(ddnsParam.userName, ddnsParam.userPsw, ddnsParam.ipAddr, ddnsParam.port, (int)ddnsParam.authType, ddnsParam.domain, ddnsParam.authType, ddnsParam.port);
            SendMsgToYiYuanDdnsServer(1);
            SendMsgToYiYuanDdnsServer(g_yiyuan_ddns_alarm_type);
            ddnsYiYuanPause();
        }
        sleep(yiyuan_alarm_ddns_param.nTime);

    }

    pthread_exit(NULL);

    return 0;
}


int ddnsFun()
{
    int ret = -1;
    DDNS_PARAM ddnsParam;
    NET_PARAM netParam;     // Add the code by lvjh, 2009-05-13
    UPNP_PORT_INFO upnp_param;
    UPNP_PORT_INFO upnp_update_ip_param;
    char gateway[16];
    char update_gateway[16];

    NET_PARAM net_param;
    UPNP_PORT_INFO upnp_ip_param;
    WIFI_PARAM wifiParam;
    UPNP_WIFI_IP_PORT_INFO ip_wifi_param;
    DVSNET_RTSP_PARAM rtsp_param;

    getNetParam(&net_param);
    getNewUpnpParam(&upnp_param);
    getRtspParam(&rtsp_param);
    getWifiParam(&wifiParam);

    memset(&ip_wifi_param, 0, sizeof(UPNP_WIFI_IP_PORT_INFO));
    strcpy(ip_wifi_param.strLocalIp, net_param.byServerIp);
    strcpy(ip_wifi_param.strWIFIIp, wifiParam.byServerIp);
    ip_wifi_param.nWebPort = net_param.wWebPort;
    ip_wifi_param.nDataPort = net_param.wServerPort;
    ip_wifi_param.nRtspPort = rtsp_param.nRtspPort;
    ip_wifi_param.nOnFlag = wifiParam.nOnFlag;
    save_ip_wifi_configure(&ip_wifi_param);

    while (1) {
        getDDNSParam(&ddnsParam);
        if(ddnsParam.authType == 3) {
            g_ddns_pause = 0;
        }
        if(g_ddns_pause) {
            //¼ì²âupnp ÍâÍøÊÇ·ñ¸Ä±ä
            // system("upnpc -w");
            memset(&upnp_update_ip_param, 0, sizeof(UPNP_PORT_INFO));
            getUSERP2PPORTConfigure(&upnp_update_ip_param);
            printf("upnpc:::upnp_update_ip_param = %s:: %d\n", upnp_update_ip_param.strRemoteIp, strlen(upnp_update_ip_param.strRemoteIp));
            if(strcmp(upnp_update_ip_param.strRemoteIp, g_device_upnp_strLocalIp)) {
                printf("UPNP CHANGE\n");
                g_ddns_pause = 0;
                strcpy(g_device_upnp_strLocalIp, upnp_update_ip_param.strRemoteIp);
                UPNP_Resume();
                //printf("update_gateway = %s\n", upnp_update_ip_param.strRemoteIp);
                continue;
            }

            //¼ÙÈçÍâÍøip¸Ä±äÖØÐÂ×¢²áddns·þÎñÆ÷
            g_ddns_port = 80;
            SendMsgToDdnsServer(2);
            if(!strcmp(g_ddns_check_extern_ip, "0.0.0.0"))
                printf("get g_ddns_check_extern_ip error\n");
            else if(strcmp(g_ddns_check_extern_ip, g_device_ddns_extern_ip)) {
                printf("NAT CHANGE\n");
                strcpy(g_device_ddns_extern_ip, g_ddns_check_extern_ip);
                g_ddns_pause = 0;
                UPNP_Resume();
            }
            printf("get g_ddns_check_extern_ip success!!\n");

            //¼ÙÈçÓÃ»§ÉèÖÃÁËÍøÂçÊý¾Ý¾ÍÖØÐÂ×¢²áddns·þÎñÆ÷
            if(g_register_ddns_flag) {
                reset_register_ddns();
                UPNP_Resume();
                g_ddns_pause = 0;
                printf("user change net remapping ip or port\n");
            }

            sleep(40);
            continue;

        }
        sleep(40);
        get_gateway_addr_ext(ETH_WIRE_DEV,update_gateway);

        getDDNSParam(&ddnsParam);
        getNetParam(&netParam);
        ret = DDNS_Open();
        if (ret == 0 && ddnsParam.nOnFlag) {
            DDNS_Setup(ddnsParam.userName, ddnsParam.userPsw, ddnsParam.ipAddr, ddnsParam.port, (int)ddnsParam.authType, ddnsParam.domain, ddnsParam.authType, netParam.wWebPort);

            ret = DDNS_Start();
            if (ret == 0) {
                //printf("DDNS(%d): OK!\n", ret);
                ;
            } else {
                printf("DDNS(%d): Failed!\n", ret);
            }
        }

    }

    pthread_exit(NULL);

    return 0;
}


int ddnsStart()
{
    int ret = -1;
    int max_priority;
    int policy;
    pthread_attr_t init_attr;
    struct sched_param init_priority;

    pthread_t threadID;
    pthread_t yy_threadID;

    // ³õÊ¼»¯ddns Ïß³ÌµÄÊôÐÔ
    pthread_attr_init(&init_attr);    //³õÊ¼»¯ÊôÐÔ

    ret = pthread_attr_setdetachstate (&init_attr,PTHREAD_CREATE_DETACHED);    //ÉèÖÃ·ÖÀë×´Ì¬
    if (ret) {
        printf("set policy:%s\n",
               strerror(ret));
    }
    pthread_attr_getinheritsched(&init_attr, &max_priority);    //»ñÈ¡¼Ì³ÐµÄµ÷¶È²ßÂÔ
    pthread_attr_setinheritsched(&init_attr, PTHREAD_EXPLICIT_SCHED);    //ÉèÖÃ¼Ì³ÐµÄµ÷¶È²ßÂÔ
    pthread_attr_getinheritsched(&init_attr, &max_priority);    //»ñÈ¡¼Ì³ÐµÄµ÷¶È²ßÂÔ
    ret = pthread_attr_setschedpolicy(&init_attr, policy);        //ÉèÖÃµ÷¶È²ßÂÔ
    if (ret) {
        printf("set policy:%s\n",
               strerror(ret));
    }

    ret = pthread_attr_setschedparam(&init_attr, &init_priority);        //ÉèÖÃµ÷¶È²ÎÊý
    if (ret) {
        printf("set policy:%s\n",
               strerror(ret));
    }

    g_ddns_snapshot_buffer  = (char *)malloc(400*1024);
    if (g_ddns_snapshot_buffer  == NULL) {
        return -1;
    }

    g_ddns_run = 1;



    ret = pthread_create(&threadID, NULL, (void *)ddnsFun, NULL);
    if (ret) {
        g_ddns_run = 0;
        free(g_ddns_snapshot_buffer);
        g_ddns_snapshot_buffer = NULL;
        return -1;
    }
    g_yiyuan_ddns_run  = 1;
    g_yiyuan_ddns_pause = 1;

#if 1
    ret = pthread_create(&yy_threadID, NULL, (void *)YiYuanDdnsFun, NULL);
    if (ret) {
        g_yiyuan_ddns_run  = 0;

        free(g_ddns_snapshot_buffer);
        g_ddns_snapshot_buffer = NULL;
        return -1;
    }
#endif

    pthread_attr_destroy(&init_attr);

    printf("ddnsStart(): OK!\n");

    return 0;
}

int ddnsStop()
{
    g_ddns_run = 0;

    return 0;
}

int ddnsPause()
{
    g_ddns_pause = 1;

    return 0;
}

int ddnsResume()
{
    g_ddns_pause = 0;

    return 0;
}


int ddnsYiYuanPause()
{

    g_yiyuan_ddns_pause = 1;

    return 0;
}


int ddnsYiYuanResume(int alarm_type)
{
    g_yiyuan_ddns_pause = 0;
    g_yiyuan_ddns_alarm_type = alarm_type;

    return 0;
}

int set_register_ddns(void)
{
    g_register_ddns_flag  = 1;
    return 0;
}
int reset_register_ddns(void)
{
    g_register_ddns_flag  = 0;
    return 0;
}

#if 1
int set_snapshot_buffer_size(char *snapshot_buffer, int snapshot_size)
{
    if(snapshot_buffer == NULL)
        return -1;
    //sem_wait(&g_snapshot_sem);
    memcpy(g_ddns_snapshot_buffer, snapshot_buffer, snapshot_size);
    g_ddns_snapshot_buffer_size = snapshot_size;
    //printf("##################################\n g_snapshot_buffer_size = %d snapshot_size  = %d\n", g_snapshot_buffer_size, *snapshot_size);
    return 0;
}
#endif

