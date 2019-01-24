#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mount.h>
#include "lib/veth.h"

#define NOT_OK_EXIT(code, msg); {if(code == -1){perror(msg); exit(-1);} }

struct container_run_para {
    char *hostname;
    int  ifindex;
};

char* const container_args[] = {
    "/bin/sh",
    "-l",
    NULL
};

static char container_stack[1024*1024]; //靠靠靠靠靠1M

/**
 * 设置挂载点
 */
static void mount_root() 
{
#if 1
    mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
#else
    //remount "/proc" to make sure the "top" and "ps" show container's information
    if (mount("proc", "containerc_roots/rootfs/proc", "proc", 0, NULL) !=0 ) {
        perror("proc");
    }
    if (mount("sysfs", "containerc_roots/rootfs/sys", "sysfs", 0, NULL)!=0) {
        perror("sys");
    }
    if (mount("none", "containerc_roots/rootfs/tmp", "tmpfs", 0, NULL)!=0) {
        perror("tmp");
    }

    if (mount("udev", "containerc_roots/rootfs/dev", "devtmpfs", 0, NULL)!=0) {
        perror("dev");
    }
    if (mount("devpts", "containerc_roots/rootfs/dev/pts", "devpts", 0, NULL)!=0) {
        perror("dev/pts");
    }
    
    if (mount("shm", "containerc_roots/rootfs/dev/shm", "tmpfs", 0, NULL)!=0) {
        perror("dev/shm");
    }
   
    if (mount("tmpfs", "containerc_roots/rootfs/run", "tmpfs", 0, NULL)!=0) {
        perror("run");
    }
    /* 
     * 模仿Docker的从外向容器里mount相关的配置文件 
     * 你可以查看：/var/lib/docker/containers/<container_id>/目录，
     * 你会看到docker的这些文件的。
     */
    if (mount("./containerc_roots/conf/hosts", "./containerc_roots/rootfs/etc/hosts", "none", MS_BIND, NULL)!=0 ||
        mount("./containerc_roots/conf/hostname", "./containerc_roots/rootfs/etc/hostname", "none", MS_BIND, NULL)!=0 ||
        mount("./containerc_roots/conf/resolv.conf", "./containerc_roots/rootfs/etc/resolv.conf", "none", MS_BIND, NULL)!=0 ) {
        perror("conf 000");
    }
    /* 模仿docker run命令中的 -v, --volume=[] 参数干的事 */
    if (mount("/tmp/t1", "./containerc_roots/rootfs/mnt", "none", MS_BIND, NULL)!=0) {
        perror("mnt");
    }
    /* chroot 隔离目录 */
    if (chdir("./containerc_roots/rootfs") != 0 || chroot("./") != 0){
        perror("chdir/chroot");
    }
#endif 
}


/**
 * 容器启动 -- 实际为子进程启动
 */
static int container_run(void *param)
{
    char ipv4[] = "192.168.23.20/24";
    struct container_run_para *cparam = (struct container_run_para*)param;
    //设置主机名
    sethostname(cparam->hostname, strlen(cparam->hostname));
    
    mount_root();
    sleep(1);
    veth_newname("veth1", "eth0");
    veth_up("eth0");
    veth_config_ipv4("eth0", ipv4);
    /* 用新进程替换子进程上下文 */
    execv(container_args[0], container_args);

     //从这里开始的代码将不会被执行到，因为当前子进程已经被上面的sh替换掉了

    return 0;
}

int main(int argc, char *argv[])
{
    struct container_run_para  para;
    pid_t child_pid;

    if (argc < 2) {
        printf("Usage: %s <child-hostname>\n", argv[0]);
        return -1;
    }

    veth_create("veth0", "veth1");
    veth_up("veth0");
    /* 将veth0加入到docker网桥中 */
    veth_addbr("veth0", "docker0");
    
    para.hostname = argv[1];
    para.ifindex = veth_ifindex("veth1");
    
    /**
     * 1、创建并启动子进程，调用该函数后，父进程将继续往后执行，也就是执行后面的waitpid
     * 2、栈是从高位向低位增长，所以这里要指向高位地址
     * 3、SIGCHLD 表示子进程退出后 会发送信号给父进程 与容器技术无关
     * 4、创建各个namespace
     */
    child_pid = clone(container_run,
                      container_stack + sizeof(container_stack),
                      /*CLONE_NEWUSER|*/
                      CLONE_NEWPID|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWUTS| SIGCHLD,
                      &para);

    /* 将veth添加到新namespace中 */
    veth_network_namespace("veth1", child_pid);
    
    NOT_OK_EXIT(child_pid, "clone");

    /* 等待子进程结束 */
    waitpid(child_pid, NULL, 0);

    return 0;
}


