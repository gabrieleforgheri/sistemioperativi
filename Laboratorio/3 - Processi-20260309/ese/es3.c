#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./es1 <N>\n");
        return 1;
    }

    const int n = atoi(argv[1]);
    pid_t fork_result;

    for (int i = 1; i<=n; i++) {
        fork_result = fork();
        if (fork_result < 0) {
            printf("Errore nella fork");
            return 1;
        }
        if (fork_result == 0){
            //child
            printf("[CHILD %d] PID: %d, Parent PID: %d\n", i, getpid(), getppid());
            _exit(0);
        }
    }
    sleep(10);
    for (int i=0; i<n;i++) {
        wait(NULL);
    }
    return 0;
}
