#include "shared_memory.h"

int
main(int argc, char const *argv[])
{
    int shm_fd, i;
    datatype * ptr;
    int array[SHARED_MEM_SIZE];

    
    shm_fd = shm_open(SHM_NAME, O_RDWR, S_IRWXU);
    if (shm_fd == -1)
    {
        printf("Error: follower fail to open shm.");
        exit(-1);
    }

    ptr = mmap(NULL, sizeof(datatype), \
                PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        printf("Error: follower fail to mmap.");
        exit(-1);
    }

    // notify the leader it is safe to writing
    ptr->write_guard = 1;
    // wait for the leader to finish writing
    printf("follower created, waiting for the leader to finish writing.\n");
    while ( ptr->read_guard == 0 ) {}

    // the leader finished writing, start to read abd print
    if (memcpy((void *)array, (void *) ptr->data, sizeof(array)) == NULL)
    {
        printf("Error: follower fail to memcpy");
        exit(-1);
    }
    /*
    for (i = 0; i < SHARED_MEM_SIZE; ++i)
    {
        printf("array[%d] = %d\n", i, array[i]);
    }
    */
    
    // notify the leader reading finished, and unlink self.
    printf("reading finished, unlink self.\n");
    ptr->delete_guard = 1;
    munmap(ptr, sizeof(datatype));
    close(shm_fd);
    

    return 0;
}

