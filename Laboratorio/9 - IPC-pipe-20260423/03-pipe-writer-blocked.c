// writer_blocks_full_pipe.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#define ITERATIONS 17

int main(void) {
    int fd[2];
    char buf[4096];
    memset(buf, 'A', sizeof(buf));

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child: wait, then read some data to make room
        close(fd[1]);
        printf("[Child] sleeping 5 seconds while the parent fills the pipe...\n");
        sleep(5);

        printf("[Child] Reading some data to free space in the pipe...\n");
        fflush(stdout);
	ssize_t total = 0;
        for (int i=0; i<ITERATIONS; i++){
	    ssize_t n = read(fd[0], buf, sizeof(buf));
            if (n < 0) {
                perror("read");
                exit(EXIT_FAILURE);
            }
	    total += n;

            printf("[Child] Read %d bytes\n", total);
	}
	printf("[Child] done reading!\n");
	exit(0);

    } else {
        // Parent: this write should block until child reads
        close(fd[0]);

    	ssize_t total = 0;
    	for (int i=0; i<ITERATIONS; i++) {
            ssize_t n = write(fd[1], buf, sizeof(buf));
            if (n > 0) {
                total += n;
	        printf("[Parent] written %d bytes\n",total);
            } else {
                perror("write while filling pipe");
                exit(EXIT_FAILURE);
            }
        }

        printf("[parent] Done!\n");
        close(fd[1]);
        wait(NULL);
    }

    return 0;
}
