#include "alerts.h"

#define PRINT_NUMBER \
    printf(YELLOW "[%d]" RESET, number); \

#ifdef DEBUG
#define PRINTCH(...) \
    PRINT_NUMBER \
    PRINT(__VA_ARGS__)
#else
#define PRINTCH(...)
#endif

#define ERRCH(...) \
    PRINT_NUMBER \
    ERR(__VA_ARGS__)
