#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NCHILDREN 5  // number of children
#define ROUNDS 5     // how many times the token goes around

int main() {
    /* number of processes = 1 (parent) + NCHILDREN
     * need a number of pipes equal to the number of processes
     */
    int pipes[NCHILDREN + 1][2];  //each pipes[i] is a pipe with read and write ends

    for (int i = 0; i < NCHILDREN + 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < NCHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Child i+1:
            // reads from pipes[i][0]
            // writes to pipes[i+1][1]

            for (int j = 0; j < NCHILDREN + 1; j++) {
                if (j != i)
                    close(pipes[j][0]);      // keep only my read end
                if (j != i + 1)
                    close(pipes[j][1]);      // keep only my write end
            }

            int token;
            while (read(pipes[i][0], &token, sizeof(token)) > 0) {
                printf("[Child %d] reads %d\n", i+1, token);

                token++;
		sleep (1);
               
                printf("[Child %d] writes %d\n", i+1, token);
		if (write(pipes[i + 1][1], &token, sizeof(token)) != sizeof(token)) {
                    perror("write");
                    exit(1);
                }
            }

            close(pipes[i][0]);
            close(pipes[i + 1][1]);
            _exit(0);
        }
    }

    // Parent:
    // writes into pipes[0][1]
    // reads from pipes[N][0]

    for (int j = 0; j < NCHILDREN + 1; j++) {
        if (j != NCHILDREN)
            close(pipes[j][0]);      // keep only pipes[N][0] for reading
        if (j != 0)
            close(pipes[j][1]);      // keep only pipes[0][1] for writing
    }

    int token = 0;

    for (int r = 0; r < ROUNDS; r++) {
        printf("[Parent] Round %d. Parent writes %d\n", r+1, token);

        if (write(pipes[0][1], &token, sizeof(token)) != sizeof(token)) {
            perror("write");
            exit(1);
        }

        if (read(pipes[NCHILDREN][0], &token, sizeof(token)) != sizeof(token)) {
            perror("read");
            exit(1);
        }

        printf("[Parent] parent reads %d\n", token);

        token++;
    }

    close(pipes[0][1]);
    close(pipes[NCHILDREN][0]);

    for (int i = 0; i < NCHILDREN; i++)
        wait(NULL);

    return 0;
}
