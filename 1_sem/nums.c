#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>


#define BASE 10

int main(int argc, char** argv) {
       char *endptr, *str;
       long val, i;

       if (argc != 2) {
           fprintf(stderr, "Usage: %s [number]\n", argv[0]);
           exit(EXIT_FAILURE);
       }

       str = argv[1];

       errno = 0;    /* To distinguish success/failure after call */
       val = strtol(str, &endptr, BASE);

       /* Check for various possible errors */

       if (errno != 0 && val == 0) {
           fprintf(stderr, "Error parsing the input string.\n");
           exit(EXIT_FAILURE);
       }
       if (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) {
       		fprintf(stderr,"Input number is out of range.\
                    The MAX number is %ld, the MIN number is %ld\n", LONG_MIN, LONG_MAX);
            exit(EXIT_FAILURE);
       }
       if (endptr == str) {
           fprintf(stderr, "No digits were found\n");
           exit(EXIT_FAILURE);
       }

       if (*endptr != '\0') {
           fprintf(stderr, "Further characters after number: %s\nPlease, input a correct number.\n", endptr);
           exit(EXIT_FAILURE);
       }

       /* If we got here, strtol() successfully parsed a number */

       for (i=1; i<=val; i++) {
       		printf("%ld ", i);
       }
       printf("\n");
       exit(EXIT_SUCCESS);

}
