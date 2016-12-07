#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>


int main(int argc, char** argv) {
	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [program] [args].\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (execvp(argv[1], argv+1) == -1) {
		fprintf(stderr, "execvp returned an error.\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}