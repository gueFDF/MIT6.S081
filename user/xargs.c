#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"
#define NULL 0
#define ARG 64
void task(char **argv)
{
    if (fork() == 0) //子进程
    {
        exec(argv[0], argv);
        exit(0);
    }
    return;
}
int main(int argc, char *argv[])
{
    int i = 0;
    char buf[1024];              //存放所有参数(内存池)
    char *ntr = buf, *ptr = buf; //指向一个参数开始和结束的位置
    char *arg[ARG];              //存放参数的指针数组，每个元素指向一个字符串，也就是参数
    char **temp = arg;           //指向指针数组起点
    for (i = 1; i < argc; i++)   //循环将参数加入素组当中
    {
        *temp = argv[i];
        temp++;
    }
    char **str = temp;           //置为开始位置
    while (read(0, ptr, 1) != 0) //循环读取参数（从标准输入）
    {
        if (*ptr == '\n' || *ptr == ' ') //一行参数读入完成
        {
            char t = *ptr;
            *ptr = '\0'; //将空格替换成'\0'
            *(str++) = ntr;
            ntr = ptr + 1;    //改变一个参数的开始位置
                              // printf("%c\n",t);
            if (t == '\n') //一行执行完毕
            {
                *str = NULL; //表示该行参数结束
                task(arg); //交给子进程执行任务
                str = temp; //重新回到开始位置
            }
        }
        ptr++;
    }
    if (str != temp) //如果最后一行结束时不是'\n'
    {
        *ptr = '\0';
        *(str++) = ntr;
        *str = NULL;
        task(arg);
    }
    while (wait(0) != -1)
        ; //循环回收子进程
    exit(0);
}
