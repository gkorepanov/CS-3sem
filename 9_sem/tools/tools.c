#include <string.h>
#include <stdio.h>
#include "alerts.h"
#include "tools.h"

long getnumber (char* str) {
#define BASE 10
    char *endptr;
    long val;


    errno = 0;    /* To distinguish success/failure after call */
    val = strtol(str, &endptr, BASE);

    /* Check for various possible errors */

    if (errno != 0 && val == 0)
       ERR("Error parsing the input string.\n")


    if (endptr == str)
       ERR("No digits were found\n")


    if (*endptr != '\0')
       ERR("Further characters after number: %s\n"\
               "Please, input a correct number.\n", endptr)

    /* If we got here, strtol() successfully parsed a number */
    return val;
};

