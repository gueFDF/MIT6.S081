#include "../kernel/types.h"
#include "../user/user.h"
int main()
{
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);
    int ret = fork();
    if (ret == 0) //子进程
    {
        char buf;
        close(p2[0]);         //关闭读端
        close(p1[1]);         //关闭写端
        read(p1[0], &buf, 1); //从父进程读
        printf("%d: received ping\n", getpid());
        write(p2[1], &buf, 1); //发送给子进程
        exit(1);
    }   
    else //父进程
    {
        char buf;
        close(p1[0]);         //关闭读端
        close(p2[1]);         //关闭写端
        write(p1[1], "!", 1); //发送给子进程
        read(p2[0], &buf, 1); //接收子进程
        printf("%d: received pong\n", getpid());
        wait(0);
    }
    return 0;
}