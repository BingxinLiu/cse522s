#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

static void
display_mallinfo(void)
{
    struct mallinfo mi;

    mi = mallinfo();

    printf("Total non-mmapped bytes (arena):       %d\n", mi.arena);
    printf("# of free chunks (ordblks):            %d\n", mi.ordblks);
    // printf("# of free fastbin blocks (smblks):     %d\n", mi.smblks);
    printf("# of mapped regions (hblks):           %d\n", mi.hblks);
    printf("Bytes in mapped regions (hblkhd):      %d\n", mi.hblkhd);
    // printf("Max. total allocated space (usmblks):  %d\n", mi.usmblks);
    // printf("Free bytes held in fastbins (fsmblks): %d\n", mi.fsmblks);
    printf("Total allocated space (uordblks):      %d\n", mi.uordblks);
    printf("Total free space (fordblks):           %d\n", mi.fordblks);
    printf("Topmost releasable block (keepcost):   %d\n", mi.keepcost);

    return;
}

int
main(int argc, char * argv[])
{
    void * mm_ptr, * uninitialized_ptr;

    /*
    if (argc > 1) {
        if (mallopt(M_CHECK_ACTION, atoi(argv[1])) != 1) {
                   fprintf(stderr, "mallopt() failed");
                   exit(EXIT_FAILURE);
               }
    }
    */

    printf("=== free two times. ===\n");
    mm_ptr = malloc(1024);
    free(mm_ptr);
    free(mm_ptr);
    printf("========= End =========\n");

    printf("=== free uninitialized pointer. ===\n");
    // free(uninitialized_ptr);
    printf("=============== End ===============\n");

    printf("=== realloc with invalid ptr ===\n");
    // uninitialized_ptr = realloc(uninitialized_ptr, 1024);
    printf("============== End ==============\n");
    
    printf("= Writes into a freed memory region =\n");
    // strcpy(mm_ptr, "Writes into a freed memory region.\n");
    printf("================ End ================\n");

    printf("= writes past the boundary of the allocated block =\n");
    // mm_ptr = (char *) malloc(16);
    // strcpy(mm_ptr, "Writes 50 characters into a 16-byte memory block.\n");
    printf("======================= End =======================\n");


    exit(EXIT_SUCCESS);
}

