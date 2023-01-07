#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        fprintf(2, "Usage: sleep times...\n");
        exit(1);
    }
    int sleep_time = atoi(argv[1]);
    if(sleep(sleep_time) < 0)
        printf("sleep failed.\n");
    exit(0);
}