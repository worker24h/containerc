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
void veth_create() 
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
    __veth_addattr(rta, IFLA_IFNAME, "veth0", strlen("veth0")+1);
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
    __veth_addattr(rta, IFLA_IFNAME, "veth1", strlen("veth1")+1);
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

void veth_up() 
{
    
}

void veth_config() 
{
    
}

void veth_network_namespace() 
{
    
}



