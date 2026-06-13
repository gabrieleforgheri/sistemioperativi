// orphans_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

            printf("[child]  pid=%ld ppid=%ld (before sleep)\n",
                   (long)mypid, (long)ppid1);

            sleep(10);

            pid_t ppid2 = getppid();
            printf("[child]  pid=%ld ppid=%ld (after  sleep)\n",
                   (long)mypid, (long)ppid2);

            _exit(0);
        }

        // Parent continues creating more children
    }

    // Parent: sleep 5s then exit WITHOUT waiting -> children become orphans
    printf("[parent] pid=%ld created %d children; sleeping 5s then exiting...\n",
           (long)getpid(), n);

    sleep(5);

    printf("[parent] pid=%ld exiting now (no wait).\n", (long)getpid());

    return 0;
}
