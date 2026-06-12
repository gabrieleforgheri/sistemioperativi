#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

typedef struct {
    int part_id;
    char customer[50];
    double amount;
    int priority;
    char note[100];
} Order;

int main(void) {
    int fd[2];

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
        // Child: receives the struct
        close(fd[1]);  // close write end

        Order received;
        ssize_t n = read(fd[0], &received, sizeof(received));
        if (n < 0) {
            perror("read");
            close(fd[0]);
            exit(EXIT_FAILURE);
        }

        if (n == 0) {
            printf("[Child] No data received\n");
            close(fd[0]);
            exit(EXIT_FAILURE);
        }

        printf("[Child] Received order:\n");
        printf("  part_id   = %d\n", received.part_id);
        printf("  customer = %s\n", received.customer);
        printf("  amount   = %.2f\n", received.amount);
        printf("  priority = %d\n", received.priority);
        printf("  note     = %s\n", received.note);

        close(fd[0]);
        exit(EXIT_SUCCESS);

    } else {
        // Parent: sends the struct
        close(fd[0]);  // close read end

        Order sent;
        sent.part_id = 101;
        strncpy(sent.customer, "Alice Johnson", sizeof(sent.customer) - 1);
        sent.customer[sizeof(sent.customer) - 1] = '\0';
        sent.amount = 249.99;
        sent.priority = 2;
        strncpy(sent.note, "Deliver before Friday and confirm by email",
                sizeof(sent.note) - 1);
        sent.note[sizeof(sent.note) - 1] = '\0';

        printf("[Parent] Sending order to child...\n");

        ssize_t n = write(fd[1], &sent, sizeof(sent));
        if (n != sizeof(sent)) {
            perror("write");
            close(fd[1]);
            wait(NULL);
            exit(EXIT_FAILURE);
        }

        close(fd[1]);
        wait(NULL);
    }

    return 0;
}
