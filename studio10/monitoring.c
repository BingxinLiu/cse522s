#include <malloc.h>
#include <stdio.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

const int num_expected_args = 3;

int
main(int argc, char *argv[])
{
    char * path_to_OOMhandler, * path_to_notificationInterface;
    char string[33];
    int efd, fd1, fd2;
    int num;
    uint64_t u64;
    ssize_t result;
    // fd1 -> OOMhandler
    // fd2 -> notification interface

    if (argc != num_expected_args)
    {
        printf("Usage: ./monitoring <path to memory.oom_control> <path to cgroup.event_control>\n");
        return -1;
    }
    path_to_OOMhandler = argv[1];
    path_to_notificationInterface = argv[2];

    if ((efd = eventfd(0, 0)) == -1)
    {
        printf("Error in eventfd().\n");
        return -1;
    }
        

    fd1 = open(path_to_OOMhandler, O_RDONLY);
    fd2 = open(path_to_notificationInterface, O_WRONLY);

    num = sprintf(string, "%d %d", efd, fd1);
    printf("copy %d.\n", num);

    write(fd2, string, num);

    while(1)
    {
        result = read(efd, &u64, sizeof(uint64_t));
        if (result != sizeof(uint64_t))
        {
            printf("Error in read().\n");
            return -1;
        }
        printf("An OOM event occurs!\n");
    }
    return 0; 
}