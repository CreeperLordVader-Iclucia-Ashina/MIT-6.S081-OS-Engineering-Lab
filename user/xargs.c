#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"
#define BUFSIZE 512

int main(int argc, char **argv)
{
    static char buf[BUFSIZE * MAXARG];
    static char* Argv[MAXARG];
    static int Argc = 0;
    if(argc < 2)
    {
        printf("Usage: xargs [programme] [arguments]\n");
        exit(0);
    }
    for(int i = 1; i < argc; i++)
    {
        Argv[Argc] = (char*)malloc(sizeof(char) * BUFSIZE);
        strcpy(Argv[Argc++], argv[i]);
    }
    Argc--;
    while(1)
    {
        gets(buf, BUFSIZE * MAXARG);
        if(buf[0] == '\0' || buf[0] == '\n' || buf[0] == '\r') break;
        int n = strlen(buf);
        if(buf[n - 1] == '\n' || buf[n - 1] == '\r')buf[--n] = '\0';
        int m = 0;
        for(int i = 0; i < n; i++)
        {
            if(buf[i] == ' ')
            {
                Argv[Argc][m] = '\0';
                m = 0;
            }
            else 
            {
                if(!m)
                {
                    Argc++;
                    if(!Argv[Argc])Argv[Argc] = (char*) malloc(sizeof(char) * BUFSIZE);
                } 
                Argv[Argc][m++] = buf[i];
            }
        }
        Argv[Argc][m++] = '\0';
        Argc++;
        int pid = fork();
        if(pid == 0)
        {
            Argv[Argc] = 0;
            exec(argv[1], Argv);
            exit(0);
        }
        else 
        {
            Argc = argc - 2;
            m = 0;
            int status;
            wait(&status);
        }
    }
    exit(0);
}