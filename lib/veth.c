#include <stdio.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <linux/veth.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "netlink.h"

struct veth_request {
    struct nlmsghdr     header;
    char                buf[1024]; /* 真1024真 */
};

static int __veth_addattr(struct rtattr *rta, int type, const void *data, int alen)
{
    int len = RTA_LENGTH(alen);// 4真真 len真真
    rta->rta_type = type;
    rta->rta_len = len;
    if (alen)
        memcpy(RTA_DATA(rta), data, alen);
    return 0;
}

/**
 * 真veth paire 真up真
 */
void veth_create() 
{
    struct netlink_handle handle;
    struct veth_request request;
    char *start = NULL;
    struct rtattr *rta = NULL;
    struct rtattr *rta_linkinfo = NULL;
    struct rtattr *rta_infodata = NULL;
    struct rtattr *rta_infopeer = NULL;
    struct ifinfomsg *ifmsg_header = NULL;
    struct veth_request req = {
        .header.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg)),
        .header.nlmsg_flags = NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL,
        .header.nlmsg_type = RTM_NEWLINK
    };

    netlink_open(&handle, 0);

    start = req.buf;
    /* netlink msg 真4真真 */    
    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_flags = IFF_UP; // up
    ifmsg_header->ifi_change = IFF_UP; // up
    start += RTA_ALIGN(sizeof(struct ifinfomsg));

    /* 真真: 真真veth 真真 */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_IFNAME, "veth0", strlen("veth0")+1);
    start += RTA_ALIGN(rta->rta_len);

    /* 真真: 真link info真 */
    rta_linkinfo = (struct rtattr *)start;
    rta_linkinfo->rta_type = IFLA_LINKINFO;
    start += sizeof(struct rtattr);

    /* 真真: 真kind */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_INFO_KIND, "veth", strlen("veth"));
    start += RTA_ALIGN(rta->rta_len);

    /* 真真: 真info data真 */
    rta_infodata = (struct rtattr *)start;
    rta_infodata->rta_type = IFLA_INFO_DATA;
    start += sizeof(struct rtattr);
    /* 真真: 真info peer真 */
    rta_infopeer= (struct rtattr *)start;
    rta_infopeer->rta_type = VETH_INFO_PEER;
    start += sizeof(struct rtattr);

    ifmsg_header = (struct ifinfomsg *)start;
    ifmsg_header->ifi_family = AF_UNSPEC;
    ifmsg_header->ifi_flags = IFF_UP; // up
    ifmsg_header->ifi_change = IFF_UP; // up
    start += RTA_ALIGN(sizeof(struct ifinfomsg));

    /* 真真: 真真veth 真真 */
    rta = (struct rtattr *)start;
    __veth_addattr(rta, IFLA_IFNAME, "veth1", strlen("veth1")+1);
    start += RTA_ALIGN(rta->rta_len);

    rta_infopeer->rta_len = start - (char*)rta_infopeer;
    
    rta_infodata->rta_len = start - (char*)rta_infodata;

    rta_linkinfo->rta_len = start - (char*)rta_linkinfo;

    req.header.nlmsg_len = start - (char*)(&req);
    
    netlink_send(&handle, &req.header);
    
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


