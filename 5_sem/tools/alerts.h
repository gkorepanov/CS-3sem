// In that file, some macros for errors are defined

#ifndef ALERTS_H
#define ALERTS_H

#define DEBUGMODE


extern int pid;

#define RESET   "\033[0m"
#define BOLDBLUE    "\033[1m\033[33m"

#define ERR(...)\
    {\
        char buffer[500];\
        int n = sprintf(buffer, BOLDBLUE "PID %d: " RESET, pid);\
        sprintf(buffer+n,  __VA_ARGS__);\
        write(STDERR_FILENO, buffer, strlen(buffer));\
        exit(EXIT_FAILURE);\
    }
#ifdef DEBUGMODE
#define PRINT(...)\
    {\
        char buffer[500];\
        int n = sprintf(buffer, BOLDBLUE "PID %d: " RESET, pid);\
        sprintf(buffer+n,  __VA_ARGS__);\
        write(STDERR_FILENO, buffer, strlen(buffer));\
    }
#endif

#ifndef DEBUGMODE
#define PRINT(...)
#endif

#endif
