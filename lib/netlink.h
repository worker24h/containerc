
#ifndef NETLINK_H
#define NETLINK_H
#include <linux/netlink.h>
struct netlink_handle {
    int                 fd;
    struct sockaddr_nl  local;
    struct sockaddr_nl  peer;
    int                 flags;
};
int netlink_open(struct netlink_handle *handle, unsigned int groups);
int netlink_close(struct netlink_handle *handle);
int netlink_send(struct netlink_handle *handle, struct nlmsghdr *buffer);
int netlink_recv(struct netlink_handle *handle, char **response);

#endif

