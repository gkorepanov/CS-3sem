#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

long getuserargs (int argc, char** argv) {
#define BASE 10
	char *endptr, *str;
	long val, i;

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
	   fprintf(stderr, "Further characters after number: %s\nPlease, input a correct number.\n", endptr);
	   exit(EXIT_FAILURE);
	}

	/* If we got here, strtol() successfully parsed a number */
	return val;
}

int main(int argc, char** argv) {
	long n = getuserargs(argc, argv); // number of children to create
	
	int index; // ordinal number of children

	int fork_ret = 0;
	int status = 0;
	int* pids = calloc(n, sizeof(int));

	for (index = 1; index <= n; index++) {
		fork_ret = fork();
		switch (fork_ret) {
			case -1: {
				fprintf(stderr, "ERROR: fork() returned -1.");
				exit(EXIT_FAILURE);
			}
			case 0: { // in a child process
				int id = getpid();
				int pid = getppid();
				printf("ID: %d; PID: %d; INDEX: %d\n", id, pid, index);
				exit(EXIT_SUCCESS);
			}

			default: { // in a parent process
				pids[index-1] = fork_ret;
			}
		}
	}

	for (index = 1; index <= n; index++) {
		if (waitpid(pids[index-1], &status, 0) == -1) {
			fprintf(stderr, "ERROR: wait() returned -1.");
		}
	}
	
	exit(EXIT_SUCCESS);
}