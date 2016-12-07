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
    int pipefd[2], fd;
    pid_t cpid;
#define BUFSIZE 10000
    char buf[BUFSIZE];
    int bytesnum = 0;
    

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error: pipe can't be created");
        exit(EXIT_FAILURE);
    }

    cpid = fork();

    if (cpid == -1) {
        fprintf(stderr, "Error: fork can't be done");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {            /* Child writes argv[1] to pipe */
        if ((fd = open(argv[1], 0)) == -1) {
            fprintf(stderr, "Error: cannot open file");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);          /* Close unused read end */
        while (bytesnum = read(fd, &buf, BUFSIZE)) {
            write(pipefd[1], &buf, bytesnum);
        }

        close(pipefd[1]);          /* Reader will see EOF */

        exit(EXIT_SUCCESS);

    } else {
        close(pipefd[1]);          /* Close unused write end */
        while (bytesnum = read(pipefd[0], &buf, BUFSIZE))
            write(STDOUT_FILENO, &buf, bytesnum);

        close(pipefd[0]);
        wait(NULL);                /* Wait for child */
        exit(EXIT_SUCCESS);
    }
}


 