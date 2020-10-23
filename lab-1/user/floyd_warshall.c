#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <paging.h>
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define DATA_SIZE 512

const int num_expected_args = 2;
const unsigned sqrt_of_UINT32_MAX = 65536;

static void *
mmap_malloc(int    fd,
            size_t bytes)
{

    void * data;

    data = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "Could not mmap " DEV_NAME ": %s\n", strerror(errno));
        return NULL;
    }

    return data;

    //return malloc(bytes);
}

int main (int argc, char* argv[])
{
    int fd;
	unsigned n, i, j, k;
	double *A, *B;
    double dt1, dt2, dt3;
    struct timeval t1, t2, t3;

	if (argc != num_expected_args) {
		printf("Usage: ./floyd_warshall <number of vertices> \n");
		exit(-1);
	}

	n = atoi(argv[1]);
	
	if (n > DATA_SIZE) {
		printf("ERROR: Number of vertices must be between zero and %u!\n", DATA_SIZE);
		exit(-1);
	}

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }
    A = (double *)mmap_malloc(fd, sizeof(double) * DATA_SIZE * DATA_SIZE);
    for (i = 0; i < DATA_SIZE; i++)
    {
        for (j = 0; j < DATA_SIZE; j++)
        {
            A[i * DATA_SIZE + j] = 1;
        }
    }
    
    gettimeofday(&t1, NULL);
    B = (double *)mmap_malloc(fd, sizeof(double) * DATA_SIZE * DATA_SIZE);

    gettimeofday(&t2, NULL);
    for(k = 0; k < n; k++) {
        for(i = 0; i < DATA_SIZE; i++) {
            B[k * DATA_SIZE + i] = A[k * DATA_SIZE + i];
        }
    }


    gettimeofday(&t3, NULL);

    dt1 = ((double)t2.tv_sec + (double)t2.tv_usec / 1000000) - ((double)t1.tv_sec + (double)t1.tv_usec / 1000000);
    dt2 = ((double)t3.tv_sec + (double)t3.tv_usec / 1000000) - ((double)t2.tv_sec + (double)t2.tv_usec / 1000000);
    dt3 = ((double)t3.tv_sec + (double)t3.tv_usec / 1000000) - ((double)t1.tv_sec + (double)t1.tv_usec / 1000000);
    printf("%lf\t%lf\t%lf\n", dt1, dt2, dt3);

    return 0;
}

