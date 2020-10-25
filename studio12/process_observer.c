#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>

#define BUF_LEN 1024

int
main(int argc, char *argv[])
{
    int fd, wd;
    int i, len;
    char buf[BUF_LEN] __attribute__((aligned(__alignof__(struct inotify_event))));

    fd = inotify_init1(0);
    if ( fd == -1 )
    {
        printf("Error: inotify_init1 fail.\n");
        return -1;
    }

    wd = inotify_add_watch(fd, "/proc", IN_ALL_EVENTS);
    if ( wd == -1 )
    {
        printf("Error: inotify_add_watch fail.\n");
        return -1;
    }

    while (1)
    {
        i = 0;
        len = read(fd, buf, BUF_LEN);
        printf("===========================================================\n");
        while ( i < len )
        {
            struct inotify_event *event = (struct inotify_event *) &buf[i];
            printf("wd=%d mask=%d cookie=%d len=%d dir=%s\n",\
                event->wd, event->mask, event->cookie, event->len, (event->mask & IN_ISDIR) ? "yes" : "no");
            if ( event->len )
            {
                printf("name=%s\n", event->name);
            }
            if (event->mask & IN_ACCESS)
                printf("File was accessed\n");
            if (event->mask & IN_MODIFY)
                printf("File was modified\n");
            if (event->mask & IN_ATTRIB)
                printf("Metadata changed\n");
            if (event->mask & IN_CLOSE_WRITE)
                printf("File opened for writing was closed.\n");
            if (event->mask & IN_CLOSE_NOWRITE)
                printf("File or directory not opened for writing was closed.\n");
            if (event->mask & IN_OPEN)
                printf("File or directory was opened.\n");
            if (event->mask & IN_MOVED_FROM)
                printf("File was move away from the watched directory.\n");
            if (event->mask & IN_MOVED_TO)
                printf("File was move into the watched directory.\n");
            if (event->mask & IN_DELETE)
                printf("File or directory deleted from watched directory.\n");
            if (event->mask & IN_DELETE_SELF)
                printf("Watched file or directory was itself deleted.\n");
            if (event->mask & IN_MOVE_SELF)
                printf("Watched file or directory was itself moved.\n");
            if (event->mask & IN_CREATE)
                printf("File/directory created in watched directory\n");
                
            i += sizeof(struct inotify_event) + event->len;
        }
        
    }


}