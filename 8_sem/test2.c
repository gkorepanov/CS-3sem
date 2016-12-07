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
void doit() {
    doit();
}

void handler(int signo) {
    printf("very bad\n");
}

int main(void)
 {
    SET_SIGNAL(SIGSEGV, handler);
    doit();
    return 0;
}
