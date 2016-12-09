#include "alerts.h"

#define PRINT_NUMBER \
    printf(YELLOW "[%zu]" RESET, NODE); \

#ifdef DEBUG
#define PRINTCH(...) \
    PRINT_NUMBER \
    PRINT(__VA_ARGS__)
#else
#define PRINTCH(...)
#endif
