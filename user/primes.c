#include "kernel/types.h"
#include "user/user.h"

int main()
{
    int fd[2];
    if(pipe(fd) < 0)
    {
        printf("prime: failed to create pipe\n");
        exit(1);
    }
    int pid = fork();
    if(pid < 0)
    {
        printf("prime: failed to fork\n");
        exit(1);
    }
    if(pid)
    {
        close(fd[0]);
        for(int i = 2; i <= 35; i++)
            write(fd[1], &i, sizeof(int));
        close(fd[1]);
        int status;
        while(wait(&status) > 0);
        exit(0);
    }
    else
    {
        int p = -1, num;
        int LP, RP[2];
        LP = fd[0];
        close(fd[1]);
        RP[0] = RP[1] = -1;
        while(read(LP, &num, sizeof(int)) == sizeof(int))
        {
            if(p < 0)
            {
                p = num;
                printf("prime %d\n", p);
                continue;
            }
            if(num % p)
            {
                if(RP[1] < 0)
                {
                    pipe(RP);
                    int ch_pid = fork();
                    if(ch_pid)write(RP[1], &num, sizeof(int));
                    else
                    {
                        LP = RP[0];
                        close(RP[1]);
                        RP[0] = RP[1] = -1;
                        p = -1;
                    }
                }
                else write(RP[1], &num, sizeof(int));
            }
        }
        if(RP[1] > 0)close(RP[1]);
        int status;
        while(wait(&status) > 0);
        exit(0);
    }
}