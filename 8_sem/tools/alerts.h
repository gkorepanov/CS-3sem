// In that file some macros for errors are defined

#ifndef ALERTS_H
#define ALERTS_H

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


#define RESET   "\033[0m"
#define YELLOW    "\033[1m\033[33m"
#define RED     "\x1b[31m"
#define ERR(...)\
    {\
        fprintf(stderr, YELLOW "[PID %05d] " RESET RED "ERR [LINE %05d] " RESET, getpid(), __LINE__);\
        fprintf(stderr, __VA_ARGS__);\
        fprintf(stderr, "\nERRNO: %s\n", strerror(errno));\
        fflush(stderr);\
        exit(EXIT_FAILURE);\
    }

#define ERRTEST(...) \
    if ((__VA_ARGS__) == -1) \
        ERR("Code line: [" #__VA_ARGS__ "]")

#ifdef DEBUG

#define PRINT(...)\
    {\
        printf(YELLOW "[PID %05d] MSG [LINE %05d] " RESET, getpid(), __LINE__);\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
    }

#else

#define PRINT(...) {};

#endif

#endif


