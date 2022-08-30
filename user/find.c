#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#define NAME 512
void find(char *path, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if ((fd = open(path, 0)) < 0) //打开失败
    {
        printf("find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        printf("find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    switch (st.type)
    {
    case T_FILE: //文件
        if (strcmp(path + strlen(path) - strlen(target), target) == 0)
            printf("%s\n", path); //符合条件，打印
        break;
    case T_DIR: //目录
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0)
            {
                printf("find: connot stat %s\n", buf);
                continue;
            }
            if (strcmp(buf + strlen(buf) - 2, "/.") != 0 && strcmp(buf + strlen(buf) - 3, "/..") != 0)
            {
                find(buf, target); //递归查找
            }
        }
        break;
    default:
        break;
    }
    close(fd);
}
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("find : error\n");
        exit(0);
    }
    else
    {
        char target[NAME];
        target[0] = '/';
        strcpy(target + 1, argv[2]);
        find(argv[1], target);
        exit(0);
    }
}