#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <stdbool.h>

#define BUF_LEN 1024
#define LIST_LEN 1024

int original_pid = 0;

void
printChain(int pid)
{
    FILE *file = NULL;
    char buf[BUF_LEN];
    char tmp[10];
    int ppid;

    if (pid == original_pid)
    {
        printf("%d\n", pid);
        return;
    }

    printf("%d->", pid);

    sprintf(buf, "/proc/%d/status", pid);
    file = fopen(buf, "r");
    if (file) {
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        fgets(buf, sizeof(buf), file);
        sscanf(buf, "%s %d", tmp, &ppid);
        fclose(file);
        printChain(ppid);
    }

    return;

}

int
main(int argc, char *argv[])
{
    int * old_list;
    int * new_list;
    int old_list_len, new_list_len;
    bool create, delete, first;

    DIR *dir = NULL;
    struct dirent *de = NULL;

    int i, j, k;

    dir = opendir("/proc");
    if( !dir )
    {
        printf("Error: open dir /proc fail.\n");
        return -1;
    }

    i = 0;
    first = true;
    old_list = (int *) malloc(sizeof(int) * LIST_LEN);
    while ( (de = readdir(dir)) )
    {
        if ( (de->d_name[0] < '0') || (de->d_name[0] > '9') )  continue;
        old_list[i++] = atoi(de->d_name);
        old_list_len = i;
    }
    
    closedir(dir);

    while (1)
    {
        dir = opendir("/proc");
        if( !dir )
        {
            printf("Error: open dir /proc fail.\n");
            return -1;
        }

        i = 0;
        new_list = (int *) malloc(sizeof(int) * LIST_LEN);
        while ( (de = readdir(dir)) )
        {
            if ( (de->d_name[0] < '0') || (de->d_name[0] > '9') )  continue;

            new_list[i++] = atoi(de->d_name);
            new_list_len = i;
        }

        for (j = 0; j < new_list_len; ++j)
        {
            create = true;
            for (k = 0; k < old_list_len; ++k)
            {
                if (new_list[j] == old_list[k]) create = false;
            }
            if (create)
            {
                if (first)
                {
                    first = false;
                    original_pid = new_list[j];
                }
                printf("Process(pid = %d) has been created.\n", new_list[j]);
                printChain(new_list[j]);
            }
        }

        for (j = 0; j < old_list_len; ++j)
        {
            delete = true;
            for (k = 0; k < new_list_len; ++k)
            {
                if (new_list[j] == old_list[k]) delete = false;
            }
            if (delete) printf("Process(pid = %d) has been delete.\n", old_list[j]);
        }

        old_list_len = new_list_len;
        free(old_list);
        old_list = new_list;

        closedir(dir);

    }


}

/*
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
*/
