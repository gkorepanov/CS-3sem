#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "tools/alerts.h"
#include <signal.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/prctl.h>

//============================================================================
//                             FAST SIGNAL SET
//============================================================================

struct sigaction act; 
sigset_t set;

#define SET_SIGNAL(signum, handler) \
    act.sa_handler = handler; \
    ERRTEST(sigaction(signum, &act, NULL))

#define SEND_SIGNAL(signum) \
    ERRTEST(kill(pid, signum))
    

int signal_bit;

#define SIG0 SIGUSR1
#define SIG1 SIGUSR2

#define WAIT sigsuspend(&set);


//============================================================================
//                              SIGNAL HANDLERS
//============================================================================

void bit_accepted(int signo) {
    switch (signo) {
        case SIG0: {
            signal_bit = 0;
            break;
        }
        case SIG1: {
            signal_bit = 1;
            break;
        }
    }
}






//============================================================================
//                               CHILD CODE
//============================================================================

// discriminating the parent's death after sending him EOF from failure
int is_eof;

void parent_death(int signo) {
    if (!is_eof)
        ERR("Parent has died, terminating")

    PRINT("Caught parent terminated successfully")
}

// 00 -- bit 0
// 01 -- bit 1
// 1 -- EOF

#define SEND_AND_CHECK(signum)\
    SEND_SIGNAL(signum)\
    WAIT

#define DOUBLE_SEND(signum1, signum2)\
    SEND_AND_CHECK(signum1)\
    SEND_AND_CHECK(signum2)

void send_data(int data, int pid) {
    switch (data) {
        case 0: {
            DOUBLE_SEND(SIG0, SIG0)
            break;
        }
        case 1: {
            DOUBLE_SEND(SIG0, SIG1)
            break;
        }

        default: { // EOF
            SEND_AND_CHECK(SIG1)
            break;
        }
    }
}


void do_child(int pid, char* filename) {
    PRINT("CHILD")
    int fd;
    ERRTEST(fd = open(filename, O_RDONLY))

    char byte;
    int i, bit;

    while (read(fd, &byte, 1)) {
        for(i = 0; i < 8; i++) {
            bit = (byte >> i) & 1;
                
            send_data(bit, pid);

        }
    }

    is_eof = 1;
    send_data(-1, pid);

    PRINT("CHILD DONE")
}



//============================================================================
//                               PARENT CODE
//============================================================================


#define SEND_RESPONSE SEND_SIGNAL(SIG0)

void child_death() {
    ERR("Child has died, terminating")
}

void process_bit(char bit) {
    static char byte = 0;
    static unsigned int count = 0;

    byte |= (bit << count++);
    if (count == 8) {
        putchar(byte);
        byte = count = 0;
    }
}

void do_parent(const int pid) {
    PRINT("PARENT")

    while(1) {
        WAIT
        if (signal_bit) {
            SET_SIGNAL(SIGCHLD, SIG_IGN)
            SEND_RESPONSE
            break;
        }
        else {
            SEND_RESPONSE
            WAIT
            process_bit(signal_bit);
            SEND_RESPONSE
        }  

    }
    PRINT("PARENT DONE")
}



//============================================================================
//                                  MAIN CODE
//============================================================================



int main(int argc, char *argv[]) {    
    //init sigset
    sigemptyset(&set);   

    // define handlers
    SET_SIGNAL(SIGCHLD, child_death) // child dies => parent do as well
    SET_SIGNAL(SIGHUP, parent_death) // parent dies => child do as well
    SET_SIGNAL(SIG0, bit_accepted)
    SET_SIGNAL(SIG1, bit_accepted)

    // temporaryly block user signals
    sigaddset(&set, SIG0); 
    sigaddset(&set, SIG1);
    sigprocmask(SIG_SETMASK, &set, NULL); // Block user signals till fork()
    
    // filling set to call sigsuspend later
    sigemptyset(&set);

    if (argc != 2)
        ERR("Usage: %s [filename]", *argv)
 
    
    int fork_ret;
    int parent_pid = getpid();
    ERRTEST(fork_ret = fork())
    if (fork_ret)
        do_parent(fork_ret);
    else {
        ERRTEST(prctl(PR_SET_PDEATHSIG, SIGHUP))
        if (getppid() != parent_pid)
            ERR("Parent died before prctl()")

        do_child(parent_pid, argv[1]);
    }
    return 0;  
}
