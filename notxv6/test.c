#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
double
now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
int main()
{
    srandom(0);
    int p;
    for (int i = 0; i < 10; i++)
    {
        printf("%ld ", random());
    }
    printf("\n");
    for (int i = 0; i < 10; i++)
    {
        printf("%f ", now());
    }
    return 0;
}