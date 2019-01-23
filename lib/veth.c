#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <linux/veth.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "netlink.h"

struct veth_request {
    struct nlmsghdr     header;
    char                buf[1024]; /* 默认1024足够*/
};

static int __veth_addattr(struct rtattr *rta, int type, const void *data, int alen)
{
    int len = RTA_LENGTH(alen);//计算属性所需要的长度
    rta->rta_type = type;
    rta->rta_len = len;
    if (alen)
        memcpy(RTA_DATA(rta), data, alen);
    return 0;
}

/**
 * 创建veth pair
 */
void veth_create(const char *veth_name, const char *peer_veth_name) 
{
    int status = 0;
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;
    struct rtattr *rta = NULL;
    struct rtattr *rta_linkinfo = NULL;
    struct rtattr *rta_infodata = NULL;
    struct rtattr *rta_infopeer = NULL;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    start += RTA_ALIGN(sizeof(struct ifinfomsg));

    //* 添加属性: 增加ifname */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_IFNAME, veth_name, strlen(veth_name)+1);
    start += RTA_ALIGN(rta->rta_len);

    /* 添加属性: 增加link info */
    rta_linkinfo = (struct rtattr *)start;
    rta_linkinfo->rta_type = IFLA_LINKINFO;
    start += sizeof(struct rtattr);

    /* 添加属性: 增加info kind */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_INFO_KIND, "veth", strlen("veth"));
    start += RTA_ALIGN(rta->rta_len);

    /* 添加属性: 增加info data */
    rta_infodata = (struct rtattr *)start;
    rta_infodata->rta_type = IFLA_INFO_DATA;
    start += sizeof(struct rtattr);
    /* 添加属性: 增加peer info */
    rta_infopeer= (struct rtattr *)start;
    rta_infopeer->rta_type = VETH_INFO_PEER;
    start += sizeof(struct rtattr);

    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    start += RTA_ALIGN(sizeof(struct ifinfomsg));

    /*添加属性: 设置peer veth名称 */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_IFNAME, peer_veth_name, strlen(peer_veth_name)+1);
    start += RTA_ALIGN(rta->rta_len);

    rta_infopeer->rta_len = start - (char*)rta_infopeer;
    
    rta_infodata->rta_len = start - (char*)rta_infodata;

    rta_linkinfo->rta_len = start - (char*)rta_linkinfo;

    req.header.nlmsg_len = start - (char*)(&req);
    
    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);
    
    return;
}

void veth_up(const char *veth_name) 
{
    int status = 0;
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_index = if_nametoindex(veth_name);
    ifmsg_header->ifi_change |= IFF_UP;
    ifmsg_header->ifi_flags |= IFF_UP;
    start += RTA_ALIGN(sizeof(struct ifinfomsg));
    req.header.nlmsg_len = start - (char*)(&req);

    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);
}

static int __get_addr_ipv4(__u8 *ap, const char *cp)
{
    int i;

    for (i = 0; i < 4; i++) {
        unsigned long n;
        char *endp;

        n = strtoul(cp, &endp, 0);
        if (n > 255)
            return -1;    /* bogus network value */

        if (endp == cp) /* no digits */
            return -1;

        ap[i] = n;

        if (*endp == '\0')
            break;

        if (i == 3 || *endp != '.')
            return -1;    /* extra characters */
        cp = endp + 1;
    }

    return 1;
}

void veth_config_ipv4(const char *veth_name, char *ipv4)
{
    int status = 0;
    char *prefix = NULL;
    int mask = 0;
    char ip[4] = {0};
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;
    struct rtattr *rta = NULL;
    struct ifaddrmsg *ifaddrmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWADDR
    };
    
    prefix = strchr(ipv4, '/');
    if (prefix) {
        mask = atoi(prefix+1);
        prefix[0] = 0;
    } else {
        mask = 32;
    }
    __get_addr_ipv4(ip, ipv4);
    
    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifaddrmsg_header = (struct ifaddrmsg *)start;
    ifaddrmsg_header->ifa_family = AF_INET;
    ifaddrmsg_header->ifa_prefixlen = mask;
    ifaddrmsg_header->ifa_flags = 0;    
    ifaddrmsg_header->ifa_scope = 0;
    ifaddrmsg_header->ifa_index = if_nametoindex(veth_name);
    start += RTA_ALIGN(sizeof(struct ifaddrmsg));   
    
    //* 添加属性: 增加if local */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFA_LOCAL, ip, sizeof(ip));
    start += RTA_ALIGN(rta->rta_len);

    /* 添加属性: 增加if addr */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFA_ADDRESS, ip, sizeof(ip));
    start += RTA_ALIGN(rta->rta_len);

    req.header.nlmsg_len = start - (char*)(&req);
    
    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);
}

void veth_network_namespace(const char* veth_name, int child_pid)
{
    int status = 0;
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;
    struct rtattr *rta = NULL;
    int pid = 0;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_index = if_nametoindex(veth_name);
    start += RTA_ALIGN(sizeof(struct ifinfomsg));

    /* 将veth加入到新network namesapce中 */    
    pid = child_pid;
    #if 1
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_NET_NS_PID, &pid, 4);
    start += RTA_ALIGN(rta->rta_len);
    #else
    {
        char path[512];
        int fd;
        snprintf(path, sizeof(path), "/proc/%d/ns/net", pid);
        fd = open(path, O_RDONLY);
        rta = (struct rtattr *)start;
        __veth_addattr(rta, IFLA_NET_NS_FD, &fd, 4);
        start += RTA_ALIGN(rta->rta_len);
    }
    #endif
    req.header.nlmsg_len = start - (char*)(&req);

    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);

}

int veth_ifindex(const char *veth_name) {
  return if_nametoindex(veth_name);
}

void veth_newname(const char *veth_name, const char* newname) 
{
    int status = 0;
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;    
    struct rtattr *rta = NULL;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_index = if_nametoindex(veth_name);
    start += RTA_ALIGN(sizeof(struct ifinfomsg));
    
    //* 添加属性: 增加if name */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_IFNAME, newname, strlen(newname)+1);
    start += RTA_ALIGN(rta->rta_len);
    
    req.header.nlmsg_len = start - (char*)(&req);

    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);
}

void veth_addbr(const char *veth_name, const char* brname) 
{
    int status = 0;
    int ifindex_br = 0;
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;    
    char *response = NULL;    
    struct rtattr *rta = NULL;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 消息 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_index = if_nametoindex(veth_name);
    start += RTA_ALIGN(sizeof(struct ifinfomsg));
    
    //* 添加属性: 增加if name */
    ifindex_br = if_nametoindex(brname);
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_MASTER, &ifindex_br, 4);
    start += RTA_ALIGN(rta->rta_len);
    
    req.header.nlmsg_len = start - (char*)(&req);

    netlink_send(&handle, &req.header);

    // 接收消息
    status = netlink_recv(&handle, &response);
    if (status > 0) {
        struct nlmsghdr *h = (struct nlmsghdr *)response;
        if (h->nlmsg_type == NLMSG_ERROR) {
            struct nlmsgerr *err = (struct nlmsgerr *)NLMSG_DATA(h);
            int error = err->error;

            if (!error) {
                //错误为0 表示没有具体错误
            } else {
                errno = -error;              
                fprintf(stderr, "ERROR: %s\n", strerror(-error));
                netlink_close(&handle);
                exit(1);
            }            
        }
        free(response);
    }
    netlink_close(&handle);
}



