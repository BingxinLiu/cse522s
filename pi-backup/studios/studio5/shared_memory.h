#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#define SHM_NAME "/shared_memory_region"
#define SHARED_MEM_SIZE 2000000

typedef struct datatype_t
{
    volatile int write_guard;
    volatile int read_guard;
    volatile int delete_guard;
    volatile int data[SHARED_MEM_SIZE];
} datatype;
