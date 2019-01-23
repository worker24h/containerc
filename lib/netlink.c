
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
//#include <linux/socket.h>
#include "netlink.h"

#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

int netlink_open(struct netlink_handle *handle, unsigned int groups)
{
    socklen_t addr_len;
    int sndbuf = 32768;
    int rcvbuf = 1024 * 1024;
    int one = 1;

    memset(handle, 0, sizeof(*handle));

    handle->fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
    if (handle->fd < 0) {
        perror("Cannot open netlink socket");
        return -1;
    }

    if (setsockopt(handle->fd, SOL_SOCKET, SO_SNDBUF,
               &sndbuf, sizeof(sndbuf)) < 0) {
        perror("SO_SNDBUF");
        return -1;
    }

    if (setsockopt(handle->fd, SOL_SOCKET, SO_RCVBUF,
               &rcvbuf, sizeof(rcvbuf)) < 0) {
        perror("SO_RCVBUF");
        return -1;
    }

    memset(&handle->local, 0, sizeof(handle->local));
    handle->local.nl_family = AF_NETLINK;
    handle->local.nl_groups = groups;

    if (bind(handle->fd, (struct sockaddr *)&handle->local,
         sizeof(handle->local)) < 0) {
        perror("Cannot bind netlink socket");
        return -1;
    }
    addr_len = sizeof(handle->local);
    if (getsockname(handle->fd, (struct sockaddr *)&handle->local,
            &addr_len) < 0) {
        perror("Cannot getsockname");
        return -1;
    }
    if (addr_len != sizeof(handle->local)) {
        fprintf(stderr, "Wrong address length %d\n", addr_len);
        return -1;
    }
    if (handle->local.nl_family != AF_NETLINK) {
        fprintf(stderr, "Wrong address family %d\n",
            handle->local.nl_family);
        return -1;
    }
    return 0;
}

int netlink_close(struct netlink_handle *handle)
{
    if (handle->fd) {
        close(handle->fd);
        handle->fd = -1;
    }
}


int netlink_send(struct netlink_handle *handle, struct nlmsghdr *buffer)
{
    int status;
    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = buffer->nlmsg_len
    };
    struct sockaddr_nl nladdr = { .nl_family = AF_NETLINK };
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1, //表示有一个iovec结构
    };

    status = sendmsg(handle->fd, &msg, 0);
    if (status < 0) {
        perror("Cannot talk to rtnetlink");
        return -1;
    }
    return 0;
}



static int __rtnl_recvmsg(int fd, struct msghdr *msg, int flags)
{
    int len;

    do {
        len = recvmsg(fd, msg, flags);
    } while (len < 0 && (errno == EINTR || errno == EAGAIN));

    if (len < 0) {
        fprintf(stderr, "netlink receive error %s (%d)\n",
            strerror(errno), errno);
        return -errno;
    }

    if (len == 0) {
        fprintf(stderr, "EOF on netlink\n");
        return -ENODATA;
    }

    return len;
}

static int rtnl_recvmsg(int fd, struct msghdr *msg, char **answer)
{
    struct iovec *iov = msg->msg_iov;
    char *buf;
    int len;

    iov->iov_base = NULL;
    iov->iov_len = 0;

    len = __rtnl_recvmsg(fd, msg, MSG_PEEK | MSG_TRUNC);
    if (len < 0)
        return len;

    buf = malloc(len);
    if (!buf) {
        fprintf(stderr, "malloc error: not enough buffer\n");
        return -ENOMEM;
    }

    iov->iov_base = buf;
    iov->iov_len = len;

    len = __rtnl_recvmsg(fd, msg, 0);
    if (len < 0) {
        free(buf);
        return len;
    }

    if (answer)
        *answer = buf;
    else
        free(buf);

    return len;
}


int netlink_recv(struct netlink_handle *handle, char **response)
{
    struct iovec iov;
    struct sockaddr_nl nladdr = { .nl_family = AF_NETLINK };
    struct msghdr msg = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iov,
        .msg_iovlen = 1, //表示有一个iovec结构
    };

    return rtnl_recvmsg(handle->fd, &msg, response);
}
