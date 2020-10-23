#include "shared_memory.h"

int
main(int argc, char const *argv[])
{
    int shm_fd, i;
    datatype * ptr;
    int array[SHARED_MEM_SIZE];

    
    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRWXU);
    if (shm_fd == -1)   
    {
        printf("Error: leader fail to open shm.");
        exit(-1);
    }
    if (ftruncate(shm_fd, sizeof(datatype)) == -1)
    {
        printf("Error: leader fail to ftruncate");
        exit(-1);
    }

    ptr = mmap(NULL, sizeof(datatype), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED)
    {
        printf("Error: leader fail to mmap.");
        exit(-1);
    }

    if (ptr->read_guard != 0 || ptr->write_guard != 0 || ptr->delete_guard !=0)
    {
        printf("Error: Shared memory region is not properly destroyed after being used.");
        munmap(ptr, sizeof(datatype));
        shm_unlink(SHM_NAME);
        close(shm_fd);
        exit(-1);
    }
    
    // initialize non-data fields
    ptr->read_guard = ptr->write_guard = ptr->delete_guard = 0;
    // wait for the follower to be created
    printf("Initialized, waiting for the follower to be created.\n");
    while ( ptr->write_guard == 0 ) {}

    // start to write the data to the struct
    srand(SHARED_MEM_SIZE);
    for (i = 0; i < SHARED_MEM_SIZE; i++)
    {
        array[i] = rand();
        // printf("array[%d] = %d\n", i, array[i]);
    }
    if (memcpy((void *)(ptr->data), array, sizeof(array)) == NULL)
    {
        printf("Error: leader fail to memcpy");
        exit(-1);

    }

    // writing finished, notify the follower to read
    ptr->read_guard = 1;

    // wait for the follower to read
    printf("writing finished, waiting for the follower to read.\n");
    while ( ptr->delete_guard == 0 ) {}

    // close the shared memory area
    printf("the follower have read, destroy the shared memory.\n");
    munmap(ptr, sizeof(datatype));
    shm_unlink(SHM_NAME);
    if ( close(shm_fd) != 0 )
    {
        printf("Error: leader fail to close the shared memory");
        exit(-1);
    }

    return 0;
}

