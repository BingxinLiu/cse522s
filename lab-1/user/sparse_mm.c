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

const int num_expected_args = 3;
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
	unsigned index, row, col; //loop indicies
	unsigned matrix_size, squared_size, n;
	double *A;

	if (argc != num_expected_args) {
		printf("Usage: ./sparse_mm <size of matrices> <number of rows assigned>\n");
		exit(-1);
	}

	matrix_size = atoi(argv[1]);
	n = atoi(argv[2]);
	
	if (matrix_size > sqrt_of_UINT32_MAX) {
		printf("ERROR: Matrix size must be between zero and %u!\n", sqrt_of_UINT32_MAX);
		exit(-1);
	}

    fd = open(DEV_NAME, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }

	squared_size = matrix_size * matrix_size;

    A = (double *)mmap_malloc(fd, sizeof(double) * squared_size);

	for (row = 0; row < n; row++) {
		for (col = 0; col < matrix_size; col++) {
        	A[row*matrix_size + col] = rand();
		}
	}

    printf("Assignment done\n");

    return 0;
}
