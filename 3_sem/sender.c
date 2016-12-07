#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[]) {
    int  fd, pipefd;
    pid_t cpid;
#define BUFSIZE 10000
    char buf[BUFSIZE];
    int bytesnum = 0;
    

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (mkfifo("sr.fifo", 0666) == -1) {
        fprintf(stderr, "Error: pipe can't be created");
        exit(EXIT_FAILURE);
    }

    if ((fd = open(argv[1], 0)) == -1) {
        fprintf(stderr, "Error: cannot open file");
        exit(EXIT_FAILURE);

    }

    printf("Pipe created, file opened\n");

    if ((pipefd = open("sr.fifo", O_WRONLY)) == -1) {
        fprintf(stderr, "Error: cannot open pipe");
        exit(EXIT_FAILURE);
    }

    printf("Pipe opened!\n");

    while (bytesnum = read(fd, &buf, BUFSIZE)) {
        write(pipefd, &buf, bytesnum);
    }

    close(pipefd);          /* Reader will see EOF */
    unlink("sr.fifo");
    exit(EXIT_SUCCESS);
}


 
