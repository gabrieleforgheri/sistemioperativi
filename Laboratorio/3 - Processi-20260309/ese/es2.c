
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./es2 <N>\n");
        return 1;
    }
    pid_t fork_result;
    const int n = atoi(argv[1]);

    for (int i =1; i<=n; i++) {
        fork_result = fork();
        if (fork_result < 0) {
            printf("Errore nella fork");
            return 1;
        }
        else if (fork_result == 0) {
            //child
            printf("[CHILD] Hello from child %d\nPID: %d, parent PID: %d\n ", i, getpid(), getppid());
            sleep(10);
            printf("[CHILD] Hello from child %d\nPID: %d, parent PID: %d\n ", i, getpid(), getppid());
            _exit(0);
        }
        else {
            //parent
            sleep(5);
        }
    }

    return 0;


}