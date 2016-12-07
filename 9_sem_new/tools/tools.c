#include <string.h>
#include "alerts.h"
#include "tools.h"

#define BASE 10
long _strtolong(char* str) {
    char *endptr;
    long val;

    // To distinguish success/failure after call
    errno = 0;  

    val = strtol(str, &endptr, BASE);

    // Check for various possible errors

    if (errno != 0 && val == 0)
       ERR("Parsing the input number.")


    if (endptr == str)
       ERR("No digits found.")


    if (*endptr != '\0')
       ERR("FOund characters after number: %s", endptr)

    // Success
    return val;
};

