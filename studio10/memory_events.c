#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

const int num_expected_args = 2;

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
    void * mm_ptr;
    unsigned int mm_size;

    if (argc != num_expected_args)
    {
        printf("Usage: ./memory_events <size of memory in kB>\n");
		exit(-1);
    }

    mm_size = atoi(argv[1]);
    //printf("========= Before malloc ==========\n");
    //display_mallinfo();
    //printf("=============== end ==============\n");

    mm_ptr = malloc(mm_size * 1024);
    //printf("========== After malloc ==========\n");
    //display_mallinfo();
    //printf("=============== end ==============\n");
    free(mm_ptr);
    free(mm_ptr);
    //printf("=========== After free ===========\n");
    //display_mallinfo();
    //printf("=============== end ==============\n");

    exit(EXIT_SUCCESS);
}