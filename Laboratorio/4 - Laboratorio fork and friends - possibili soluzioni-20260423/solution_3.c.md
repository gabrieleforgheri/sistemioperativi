// orphans_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: n must be a positive integer.\n");
        return 1;
    }

    // Create n children
    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            // Child: print PID and PPID, sleep 10s, print again
            pid_t mypid = getpid();
            pid_t ppid1 = getppid();

            printf("[child]  pid=%ld ppid=%ld. That's all folks!\n",
                   (long)mypid, (long)ppid1);

	    _exit(0);
        }

        // Parent continues creating more children
    }

    // Parent: sleep 30s then collect results from zombie children
    sleep(30);

    for (int i = 0; i<n; i++) {
	    wait(NULL);
    }

    return 0;
}
