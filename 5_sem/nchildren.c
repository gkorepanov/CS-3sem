#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "tools/alerts.h"
#include <string.h>
int pid;

long getuserargs (int argc, char** argv) {
#define BASE 10
    char *endptr, *str;
    long val;

    if (argc != 2) {
       fprintf(stderr, "Usage: %s [n]\n", argv[0]);
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
       fprintf(stderr, "Further characters after number: %s\n"\
               "Please, input a correct number.\n", endptr);
       exit(EXIT_FAILURE);
    }

    /* If we got here, strtol() successfully parsed a number */
    return val;
};

struct msgbuf {
    long mtype;
    char mtext[1];
};


#define PARENT_TYPE INT_MAX
void do_child(int index, int qid) {
    int id = getpid();
    
    struct msgbuf msg;
    
    if (msgrcv(qid, (void *) &msg, sizeof(msg.mtext), index, MSG_NOERROR) == -1)
        ERR("msgrcv error: %s\n", strerror(errno))

    char outbuf[100];
    sprintf(outbuf, "ID: %d; PID: %d; INDEX: %d\n", id, pid, index);
    write(STDOUT_FILENO, outbuf, strlen(outbuf));
    fsync(STDOUT_FILENO);

    msg.mtype = PARENT_TYPE;
    if (msgsnd(qid, (void *) &msg, sizeof(msg.mtext), 0) == -1) 
            ERR("msgsnd error, errno: %s\n", strerror(errno))

    exit(EXIT_SUCCESS);
};


#define PROJ_ID 2872

int main(int argc, char** argv) {
    pid = getpid();
    long n = getuserargs(argc, argv); // number of children to create
    
    int index; // ordinal number of children

    int key = ftok("/tmp", PROJ_ID);
    int qid;
    if((qid = msgget(key, IPC_CREAT | 0666)) == -1)
        ERR("msgget error: %s\n", strerror(errno))


    int fork_ret = 0;
    for (index = 1; index <= n; index++) {
        fork_ret = fork();
        switch (fork_ret) {
            case -1: {
                ERR("ERROR: fork() returned -1.")
            }
            case 0: { // in a child process
                do_child(index, qid);
                break;
            }
        }
    }

    struct msgbuf msg;

    for(index = 1; index <= n; index++) {
        msg.mtype = index;
        
        if (msgsnd(qid, (void *) &msg, sizeof(msg.mtext), 0) == -1) 
            ERR("msgsnd error, errno: %s\n", strerror(errno))
           
        if (msgrcv(qid, (void *) &msg, sizeof(msg.mtext), PARENT_TYPE, MSG_NOERROR) == -1)
            ERR("msgrcv error: %s\n", strerror(errno))
    }
 
    msgctl(qid, IPC_RMID, 0);
    exit(EXIT_SUCCESS);
}
