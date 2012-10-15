/* @LICENCE("Public", "2008")@ */

#include <stdio.h>

int
main(int argc, char **argv)
{
    int i;

    /* Print program arguments if there are any. */
    if (argc > 0) {
        for (i = 0; i < argc; i++) {
            printf("[%s]", argv[i]);
            fflush(stdout);
        }
    }
    /* Otherwise, print error message. */
    else {
        printf("No command line arguments\n");
    }

    /* Finished */
    while(1);
}
