#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"

void child_prime(int readpipe[2])
{
    int num;
    read(readpipe[0], &num, sizeof(num)); //读取一个
    if (num == -1)                        //全部读完
    {
        // printf("全部筛选完毕\n");
        exit(0);
    }
    printf("prime %d\n", num);

    int writepipe[2];
    pipe(writepipe);
    //int ret = fork();
    if (fork()==0) //子进程
    {
        close(readpipe[0]);  //子进程用不到
        close(writepipe[1]); //子进程只用到读
        child_prime(writepipe);
    }
    else //父进程
    {
        close(writepipe[0]); //用不到读端
        int temp = 0;
        while (read(readpipe[0], &temp, sizeof(temp)) &&temp!= -1)
        {
            if (temp % num != 0) //筛选
            {
                write(writepipe[1], &temp, sizeof(temp)); //写到管道
            }
        }
        temp = -1;
        write(writepipe[1], &temp, sizeof(temp)); //写入结束标志
        wait(0);
        exit(0);
    }
}

int main()
{
    int input[2];
    pipe(input);
    int ret = fork();
    if (ret == 0) //子进程
    {
        close(input[1]); //关闭写端
        child_prime(input);
        exit(0);
    }
    else //父进程
    {
        int i;
        close(input[0]); //关闭读端
        for (i = 2; i <= 35; i++)
        {
            write(input[1], &i, sizeof(i));
        }
        i = -1;
        write(input[1], &i, sizeof(i)); //告诉子线程，输入完毕
    }
    wait(0);
    exit(0); //退出
}