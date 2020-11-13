#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

int
main(int argc, char * argv[])
{
    void * mm_ptr, * uninitialized_ptr;

    if (argc > 1) {
        if (mallopt(M_CHECK_ACTION, atoi(argv[1])) != 1)
        {
            fprintf(stderr, "mallopt() failed");
            exit(EXIT_FAILURE);
        }
    }

    printf("=== free two times. ===\n");
    mm_ptr = malloc(1024);
    free(mm_ptr);
    free(mm_ptr);

    printf("=== free uninitialized pointer. ===\n");
    free(uninitialized_ptr);


    printf("=== realloc with invalid ptr ===\n");
    uninitialized_ptr = realloc(uninitialized_ptr, 1024);

    
    printf("= Writes into a freed memory region =\n");
    strcpy(mm_ptr, "Writes into a freed memory region.\n");


    printf("= writes past the boundary of the allocated block =\n");
    mm_ptr = (char *) malloc(16);
    strcpy(mm_ptr, "Writes 50 characters into a 16-byte memory block.\n");



    exit(EXIT_SUCCESS);
}

