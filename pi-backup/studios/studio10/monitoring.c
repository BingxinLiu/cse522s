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
#include <sys/epoll.h>

#define MAX_EVENTS 10

const int num_expected_args = 5;

int
main(int argc, char *argv[])
{
    char * path_to_OOMhandler, * path_to_notificationInterface;
    char * path_to_2nd_OOMhandler, * path_to_2nd_notificationInterface;
    char string[33], string2[33];
    int efd, efd2, fd1, fd2, fd12, fd22, epollfd, nfds;
    int num, num2, n;
    uint64_t u64;
    ssize_t result;
    struct epoll_event ev, ev2, events[MAX_EVENTS];
    // fd1 -> OOMhandler
    // fd2 -> notification interface

    if (argc != num_expected_args)
    {
        printf("Usage: ./monitoring <path to 1st child's memory.oom_control> <path to 1st child's cgroup.event_control> <path to 2nd child's memory.oom_control> <path to 2nd child's cgroup.event_control>\n");
        return -1;
    }
    path_to_OOMhandler = argv[1];
    path_to_notificationInterface = argv[2];
    path_to_2nd_OOMhandler = argv[3];
    path_to_2nd_notificationInterface = argv[4];

    if ((efd = eventfd(0, 0)) == -1)
    {
        printf("Error in eventfd().\n");
        return -1;
    }
    if ((efd2 = eventfd(0, 0)) == -1)
    {
        printf("Error in 2nd eventfd().\n");
        return -1;
    }
        

    fd1 = open(path_to_OOMhandler, O_RDONLY);
    fd2 = open(path_to_notificationInterface, O_WRONLY);

    fd12 = open(path_to_2nd_OOMhandler, O_RDONLY);
    fd22 = open(path_to_2nd_notificationInterface, O_WRONLY);

    num = sprintf(string, "%d %d", efd, fd1);
    num2 = sprintf(string2, "%d %d", efd2, fd12);

    write(fd2, string, num);
    write(fd22, string2, num2);

    epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        printf("Error in epoll_create1()\n");
        return -1;
    }
    ev.events = EPOLLIN;
    ev.data.fd = efd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, efd, &ev) == -1)
    {
        printf("Error: in epoll_ctl1\n");
        return -1;
    }
    ev2.events = EPOLLIN;
    ev2.data.fd = efd2;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, efd2, &ev2) == -1)
    {
        printf("Error: in epoll_ctl2\n");
        return -1;
    }

    while(1)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            printf("Error in epoll_wait()\n");
            return -1;
        }

        for (n = 0; n < nfds; ++n)
        {
            if (events[n].data.fd == efd)
            {
                printf("An OOM event occurs at child1!\n");
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, efd, &ev) == -1)
                {
                    printf("Error: in epoll_ctl1 MOD.\n");
                    return -1;
                }
            } else if (events[n].data.fd == efd2)
            {
                printf("An OOM event occurs at child2!\n");
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, efd2, &ev2) == -1)
                {
                    printf("Error: in epoll_ctl2 MOD.\n");
                    return -1;
                }
            } else
            {
                printf("Some unrecognized event occurs!\n");
            }
            
            
        }

    }
    return 0; 
}


        // result = read(efd, &u64, sizeof(uint64_t));
        // if (result != sizeof(uint64_t))
        // {
        //     printf("Error in read().\n");
        //     return -1;
        // }
        // printf("An OOM event occurs!\n");

        // sudo su&& echo 204800 > /sys/fs/cgroup/memory/child1/memory.limit_in_bytes&&cd studios/studio10&&echo $$ > /sys/fs/cgroup/memory/child1/tasks&&/monitored

        //sudo su&& echo 204800 > /sys/fs/cgroup/memory/child2/memory.limit_in_bytes&&cd studios/studio10&&echo $$ > /sys/fs/cgroup/memory/child2/tasks&&./monitored