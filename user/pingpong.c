#include "kernel/types.h"
#include "user/user.h"

int main()
{
    int p_pid = getpid(); 
    int p[2];
    if(pipe(p) < 0)
    {
        fprintf(2, "pingpong: failed to create pipe\n");
        exit(1);
    }
    int pid = fork();
    if(pid < 0)
    {
        fprintf(2, "pingpong: failed to fork\n");
        exit(1);
    }
    if(pid != 0)//in the parent proc
    {
        char buf = 'a';
        write(p[1], &buf, 1);
        int state;
        if(wait(&state) == pid)
        {
            if(read(p[0], &buf, 1) > 0)
            printf("%d: received pong\n", p_pid);
            exit(0);
        }   
        else
        {
            fprintf(2, "pingpong: error when return from child proc\n");
            exit(1);
        }
    }
    else
    {
        int ch_pid = getpid();
        char buf;
        if(read(p[0], &buf, 1) > 0)
            printf("%d: received ping\n", ch_pid);
        write(p[1], &buf, 1);
        exit(0);
    }
}