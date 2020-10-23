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

const int num_expected_args = 2;

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
	unsigned i, j, n, m;
	double *A, syscall_time, workload_time;
    struct timeval t1, t2, t3;

	if (argc != num_expected_args) {
		printf("Usage: ./rand_assign_submatrix <size of matrix> \n");
		exit(-1);
	}

	n = atoi(argv[1]);
	
	if (n < 64 || n > 4096) {
		printf("ERROR: Matrix size must be between 64 and 4096\n");
		exit(-1);
	}

    if(4096 % n != 0) {
		printf("ERROR: Matrix size must be a divisor of 4096\n");
		exit(-1);	
    }

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }
    
    m = 4096 / n;

    gettimeofday(&t1, NULL);
    A = (double *)mmap_malloc(fd, sizeof(double) * n * n);

    gettimeofday(&t2, NULL);
   
    for(i = 0; i < m; i++) {
        for(j = 0; j < m; j++) {
            A[i * n + j] = rand();
        }
    }

    gettimeofday(&t3, NULL);
    
    syscall_time = (t2.tv_sec + 1.0 * t2.tv_usec / 1e6) - (t1.tv_sec + 1.0 * t1.tv_usec / 1e6);
    workload_time = (t3.tv_sec + 1.0 * t3.tv_usec / 1e6) - (t2.tv_sec + 1.0 * t2.tv_usec / 1e6);
    printf("%.6lf\t%.6lf\t%.6lf\n", syscall_time, workload_time, syscall_time + workload_time);

    return 0;
}

