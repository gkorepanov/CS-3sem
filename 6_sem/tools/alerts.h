// In that file, some macros for errors are defined

#ifndef ALERTS_H
#define ALERTS_H

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

//#define DEBUGMODE

#define RESET   "\033[0m"
#define BOLDBLUE    "\033[1m\033[33m"
#define RED     "\x1b[31m"
#define ERR(...)\
    {\
        char buffer[500];\
        int n = sprintf(buffer, BOLDBLUE "PID %d: " RESET RED " ERROR: " RESET, getpid());\
        int n2= sprintf(buffer+n,  __VA_ARGS__);\
        *(buffer + n + n2) = '\n';\
        *(buffer + n + n2 + 1) = '\0';\
        write(STDERR_FILENO, buffer, strlen(buffer));\
        fsync(STDERR_FILENO);\
        exit(EXIT_FAILURE);\
    }


#ifdef DEBUG
#define PRINT(...)\
    {\
        char buffer[500];\
        int n = sprintf(buffer, BOLDBLUE "PID %d: " RESET, getpid());\
        int n2= sprintf(buffer+n,  __VA_ARGS__);\
        *(buffer + n + n2) = '\n';\
        *(buffer + n + n2 + 1) = '\0';\
        write(STDOUT_FILENO, buffer, strlen(buffer));\
        fsync(STDOUT_FILENO);\
    }


#define DBG(...)\
    {\
        char buffer[500];\
        int n = sprintf(buffer, BOLDBLUE "PID %d: " RESET, getpid());\
        sprintf(buffer+n,  __VA_ARGS__);\
        write(STDOUT_FILENO, buffer, strlen(buffer));\
        fsync(STDOUT_FILENO);\
    }
#endif


#ifndef DEBUG
#define PRINT(...) {};
#define DBG(...) {};
#endif

#endif
