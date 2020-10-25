#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

const int num_expected_args = 3;

int
main (int argc, char *argv[])
{
    char * str;
    int timePeriod, i;
    pid_t pid;
    if (argc != num_expected_args)
    {
        printf("Usage: ./anomalous_process <String> <Integer>\n");
        return -1;
    }

    timePeriod = atoi(argv[2]);
    printf("PID = %d, Command Line = ", getpid());
    for (i = 0; i < argc; ++i)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    

    while (1)
    {
        sleep(timePeriod);
        pid = fork();
        if ( pid == -1 )
        {
            printf("Error: fork fails.\n");
        }
        if ( pid == 0 )
        {
            printf("In child process, PID = %d, PPID = %d.\n", getpid(), getppid());
        }
    }

    return 0;


}