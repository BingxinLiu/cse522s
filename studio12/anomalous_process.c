#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>

int
main (int argc, char *argv[])
{
    char * str;
    int timePeriod, i, forkRate, terminateRate, randomNumber;
    pid_t pid;
    bool random;

    if ( !strcmp(argv[1], "randombranch") )
    {
        random = true;
        if (argc != 5)
        {
            printf("Usage: ./anomalous_process randombranch <Integer> <Integer> <Integer>\n");
            return -1;
        }
        forkRate = atoi(argv[3]);
        terminateRate = atoi(argv[4]);
        printf("forkRate = %d, terminateRate = %d\n", forkRate, terminateRate);
    } else
    {
        random = false;
        if (argc != 3)
        {
            printf("Usage: ./anomalous_process slowbranch <Integer>\n");
            return -1;
        }
    }

    timePeriod = atoi(argv[2]);
    printf("PID = %d, Command Line = ", getpid());
    for (i = 0; i < argc; ++i)
    {
        printf("%s ", argv[i]);
    }
    printf("\n");
    
    srand(time(0));

    while (1)
    {
        sleep(timePeriod);
        pid = 1;
        if (random)
        {
            randomNumber = rand() % 100 + 1;
            if (randomNumber <= forkRate)
            {
                
                printf("randomNum = %d, forkRate = %d\n", randomNumber, forkRate);
                pid = fork();
                if ( pid == -1 )
                {
                    printf("Error: fork fails.\n");
                }
                if ( pid == 0 )
                {
                    srand(time(0));
                    printf("In child process, PID = %d, PPID = %d.\n", getpid(), getppid());
                    continue;
                }
            }
        }
        if (!random)
        {
            pid = fork();
            if ( pid == -1 )
            {
                printf("Error: fork fails.\n");
            }
            if ( pid == 0 )
            {
                srand(time(0));
                printf("In child process, PID = %d, PPID = %d.\n", getpid(), getppid());
            }
            continue;
        }
        if (random)
        {
            randomNumber = rand() % 100 + 1;
            if (randomNumber <= terminateRate)
            {
                printf("randomNum = %d, terminateRate = %d.\n", randomNumber, terminateRate);
                break;
            }
        }

    }

    return 0;


}