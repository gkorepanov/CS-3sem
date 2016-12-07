#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PRINT(...)\
    {\
        fprintf(stderr, "%d: %s", pid, __VA_ARGS__);\
    }

int main(int argc, char *argv[]) {
    int pipefd;
    int pid = getpid();

#define BUFSIZE 1
    char buf[BUFSIZE];
    int bytesnum = 0;
    
    

    if ((pipefd = open("sr.fifo", O_RDONLY | O_NONBLOCK)) == -1) {
        PRINT("Error: cannot open pipe\n");
        exit(EXIT_FAILURE);
    }

        
    while (bytesnum = read(pipefd, &buf, BUFSIZE))
        write(STDOUT_FILENO, &buf, bytesnum);

    PRINT("Finished read.\n");
    close(pipefd);
    PRINT("Pipe closed, exit(0)\n");
    exit(EXIT_SUCCESS);
}


 
