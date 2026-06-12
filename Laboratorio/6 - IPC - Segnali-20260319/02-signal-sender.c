#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main(void) {
    pid_t pid;
    int sig;

    printf("Enter PID and signal number: ");

    if (scanf("%d %d", &pid, &sig) != 2) {
        fprintf(stderr, "Invalid input\n");
        return EXIT_FAILURE;
    }

    if (kill(pid, sig) == -1) {
        perror("kill");
        return EXIT_FAILURE;
    }

    printf("Signal %d sent to process %d\n", sig, pid);
    return EXIT_SUCCESS;
}
