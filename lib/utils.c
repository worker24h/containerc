#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.h"

static int get_prefix(uint32_t mask)
{
    int i = 0, cnt = 0;
    for (i = 0; i < 32; i++)
    {
        if ((mask<<i) & 0x80000000)
            cnt++;
        else
            break;
    }
    return cnt;
}

struct bridge_ipinfo get_brip(const char *br)
{
    struct bridge_ipinfo brinfo;
    int fd, interfaces, retn = 0;
    struct ifreq buf[INET_ADDRSTRLEN];
    struct ifconf ifc;
    char ip[32], mask[32];
    uint32_t  uip, umask;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        // caddr_t,linux内核源码里定义的：typedef void *caddr_t；
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))
        {
            interfaces = ifc.ifc_len/sizeof(struct ifreq);
            while (interfaces-- > 0)
            {                
                if (strcmp(buf[interfaces].ifr_name, br)) 
                {
                    continue;
                }
                
                if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[interfaces])))
                {
                    uip = ((struct sockaddr_in*)&(buf[interfaces].ifr_addr))->sin_addr.s_addr;
                    brinfo.ip = ntohl(uip);                    
                    if (ioctl(fd, SIOCGIFNETMASK, &buf[interfaces]) < 0){  
                        fprintf(stderr, "Error: %d, cannot get mask of %s\n", errno, br);
                        exit(1);
                    }  
                    else{  
                        umask = ((struct sockaddr_in*)&(buf[interfaces].ifr_netmask))->sin_addr.s_addr;
                        brinfo.prefix = get_prefix(ntohl(umask));
                    }
                    break;
                }
            }
        }
        close(fd);        
    }
    return brinfo;
}

void new_containerip(char *ipaddr, int len)
{
    int  ip;//网络序
    struct bridge_ipinfo docker0 = get_brip("docker0");
    ip = htonl(docker0.ip + 99);
    inet_ntop(AF_INET, &ip, ipaddr, len);
    sprintf(ipaddr+strlen(ipaddr), "/%d", docker0.prefix);    
    return;
}

