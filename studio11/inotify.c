#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>

const int num_expected_args = 2;

int
main(int argc, char *argv[])
{
    int inotifyfd, wd;

    if ( argc != num_expected_args )
    {
        printf("Usage: ./inotify <>\n");
        return -1;
    }

    inotifyfd = inotify_init1(0);
    if ( inotifyfd == -1 )
    {
        printf("Error: inotify_init1 fail.\n");
        return -1;
    }
    printf("Initialize the inotify instance successfully. The value of file descriptor is %d.\n", inotifyfd);

    wd = inotify_add_watch(inotifyfd, argv[1], IN_ALL_EVENTS);
    if ( wd == -1 )
    {
        printf("Error: inotify_add_watch fail.\n");
        return -1;
    }
    printf("Adding a watch successfully. The file descriptor is %d, and the command line argument is %s.\n", wd, argv[1]);

    return 0;
}