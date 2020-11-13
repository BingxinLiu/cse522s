#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char *argv[])
{
    char *p;

   if (argc > 1) {
        if (mallopt(M_CHECK_ACTION, atoi(argv[1])) != 1) {
            fprintf(stderr, "mallopt() failed");
            exit(EXIT_FAILURE);
        }
    }

   p = malloc(1000);
    if (p == NULL) {
        fprintf(stderr, "malloc() failed");
        exit(EXIT_FAILURE);
    }

   free(p);
    printf("main(): returned from first free() call\n");

   free(p);
    printf("main(): returned from second free() call\n");

   exit(EXIT_SUCCESS);
}