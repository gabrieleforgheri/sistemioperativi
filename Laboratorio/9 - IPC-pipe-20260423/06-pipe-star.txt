#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NCHILDREN 5
#define ROUNDS 4

int main() {
    int to_parent[NCHILDREN][2]; //array of pipes used by children to send data to the parent
    int to_child[NCHILDREN][2];  //array of pipes used by the parent to send data to children

    for (int i = 0; i < NCHILDREN; i++) {
        if (pipe(to_parent[i]) == -1 || pipe(to_child[i]) == -1) {
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
            // Child i
            for (int j = 0; j < NCHILDREN; j++) {
                if (j != i) {
                    close(to_parent[j][0]);
                    close(to_parent[j][1]);
                    close(to_child[j][0]);
                    close(to_child[j][1]);
                }
            }

            // Child uses:
            // to_parent[i][1] to send to parent
            // to_child[i][0]  to receive from parent
            close(to_parent[i][0]);
            close(to_child[i][1]);

            for (int r = 0; r < ROUNDS; r++) {
                int msg = i * 100 + r;

                printf("[Child %d] sends %d\n", i+1, msg);

                if (write(to_parent[i][1], &msg, sizeof(msg)) != sizeof(msg)) {
                    perror("write child->parent");
                    exit(1);
                }

		sleep(1);

                if (read(to_child[i][0], &msg, sizeof(msg)) != sizeof(msg)) {
                    perror("read parent->child");
                    exit(1);
                }

                printf("[Child %d] received reply %d\n", i+1, msg);
            }

            close(to_parent[i][1]);
            close(to_child[i][0]);
            exit(0);
        }
    }

    // Parent
    for (int i = 0; i < NCHILDREN; i++) {
        // Parent uses:
        // to_parent[i][0] to receive from child i
        // to_child[i][1]  to send to child i
        close(to_parent[i][1]);
        close(to_child[i][0]);
    }

    for (int r = 0; r < ROUNDS; r++) {
        int values[NCHILDREN];

        // Collect one message from each child
        for (int i = 0; i < NCHILDREN; i++) {
            if (read(to_parent[i][0], &values[i], sizeof(values[i])) != sizeof(values[i])) {
                perror("read child->parent");
                exit(1);
            }
            printf("[Parent] received %d from child %d\n", values[i], i+1);
        }

        // Send one reply to each child
        for (int i = 0; i < NCHILDREN; i++) {
            int reply = values[i] + 1000;

            printf("[Parent] replies %d to child %d\n", reply, i+1);

            if (write(to_child[i][1], &reply, sizeof(reply)) != sizeof(reply)) {
                perror("write parent->child");
                exit(1);
            }
        }
    }

    for (int i = 0; i < NCHILDREN; i++) {
        close(to_parent[i][0]);
        close(to_child[i][1]);
    }

    for (int i = 0; i < NCHILDREN; i++)
        wait(NULL);

    return 0;
}
