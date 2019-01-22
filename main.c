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

/**
 * 挂载点修改
 */
static void mount_root() 
{
    mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);
    mount("proc", "/proc", "proc", 0, NULL);
}

/**
 * 容器启动 -- 实际为子进程启动
 */
static int container_run(void *hostname)
{
    //设置主机名
    sethostname(hostname, strlen(hostname));

    mount_root();

    /* 用新进程替换子进程上下文 */
    execlp("bash", "bash", (char *) NULL);

    //从这里开始的代码将不会被执行到，因为当前子进程已经被上面的bash替换掉了

    return 0;
}

static char child_stack[1024*1024]; //设置子进程的栈空间为1M

int main(int argc, char *argv[])
{
    pid_t child_pid;

    if (argc < 2) {
        printf("Usage: %s <child-hostname>\n", argv[0]);
        return -1;
    }

    /**
     * 1、创建并启动子进程，调用该函数后，父进程将继续往后执行，也就是执行后面的waitpid
     * 2、栈是从高位向低位增长，所以这里要指向高位地址
     * 3、SIGCHLD 表示子进程退出后 会发送信号给父进程 与容器技术无关
     * 4、创建各个namespace
     */
    child_pid = clone(container_run,
                    child_stack + sizeof(child_stack),
                    /*CLONE_NEWUSER|*/
                    CLONE_NEWPID|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWUTS| SIGCHLD,
                    argv[1]);

    NOT_OK_EXIT(child_pid, "clone");

    /* 等待子进程结束 */
    waitpid(child_pid, NULL, 0);

    return 0;
}

