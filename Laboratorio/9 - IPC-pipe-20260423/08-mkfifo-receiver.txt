#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include "08-mkfifo-message.h"

int main(void) {
    // Create the FIFO if it doesn't exist
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    printf("[Receiver] FIFO ready at %s\n", FIFO_PATH);
    printf("[Receiver] Opening FIFO for reading...\n");
    fflush(stdout);

    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    printf("[Receiver] Waiting for an Order message...\n");
    fflush(stdout);

    Order received;
    ssize_t n = read(fd, &received, sizeof(received));
    if (n < 0) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (n == 0) {
        printf("[Receiver] No writer connected or FIFO closed without data\n");
        close(fd);
        exit(EXIT_SUCCESS);
    }

    if (n != sizeof(received)) {
        fprintf(stderr, "[Receiver] Partial read: got %d bytes, expected %d\n",
                n, sizeof(received));
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("[Receiver] Order received:\n");
    printf("  part_id   = %d\n", received.part_id);
    printf("  customer = %s\n", received.customer);
    printf("  amount   = %.2f\n", received.amount);
    printf("  priority = %d\n", received.priority);
    printf("  note     = %s\n", received.note);

    close(fd);
    return 0;
}
