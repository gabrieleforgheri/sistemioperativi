
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: ./es1 <N>\n");
        return 1;
    }

    const int n = atoi(argv[1]);
    pid_t fork_result;
    int* result = calloc(n, sizeof(int));
    pid_t* child_pid = calloc(n, sizeof(pid_t));

    for (int i = 1; i <= n; i++) {
        fork_result = fork();
        if (fork_result < 0) {
            printf("Error in fork\n");
            return 1;
        }
        else if (fork_result == 0) {
            //child
            srand(getpid());
            int random_number = rand() % 256;
            printf("[CHILD] Child %d - PID: %d\n", i, getpid());
            printf("[CHILD] Random number: %d\n", random_number);
            _exit(random_number);
        }
        else {
            int status;
            waitpid(fork_result, &status, 0);

            if (WIFEXITED(status)) {
                int child_status = WEXITSTATUS(status);
                printf("[PARENT] Child %d generated: %d\n", i, child_status);
                result[i-1] = child_status;
                child_pid[i-1] = fork_result;

            }
            else {
                printf("[PARENT] Child %d exited not normally \n", n);
            }
        }
    }

    int max = result[0];
    pid_t max_ch = child_pid[0];
    for (int i = 0; i < n; i++) {
        if (result[i] > max) {
            max = result[i];
            max_ch = child_pid[i];
        }
    }
    printf("[PARENT] Max child number: %d - PID: %d\n", max, max_ch);

    free(result);
    return 0;
}