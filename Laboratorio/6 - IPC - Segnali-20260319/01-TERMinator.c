#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

int main() {
    pid_t pid;

    printf("Enter PID: ");
    if (scanf("%d", &pid) != 1) {
        fprintf(stderr, "Invalid PID input\n");
        return 1;
    }

    if (kill(pid, SIGTERM) == -1) {
        perror("kill failed");
        return 1;
    }

    printf("SIGTERM sent to process %d\n", pid);
    return 0;
}
