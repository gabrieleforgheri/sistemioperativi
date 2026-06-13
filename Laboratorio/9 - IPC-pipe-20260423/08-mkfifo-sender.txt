#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "08-mkfifo-message.h"

int main(void) {
    printf("[Sender] Opening FIFO for writing...\n");

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    Order sent;
    sent.part_id = 101;
    strncpy(sent.customer, "Alice Johnson", sizeof(sent.customer) - 1);
    sent.customer[sizeof(sent.customer) - 1] = '\0';
    sent.amount = 249.99;
    sent.priority = 2;
    strncpy(sent.note, "Deliver before Friday and confirm by email", sizeof(sent.note) - 1);
    sent.note[sizeof(sent.note) - 1] = '\0';

    printf("[Sender] Sending Task message...\n");

    ssize_t n = write(fd, &sent, sizeof(sent));
    if (n == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (n != sizeof(sent)) {
        fprintf(stderr, "[Sender] Partial write: wrote %d bytes, expected %d\n",
                n, sizeof(sent));
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("[Sender] Message sent successfully\n");

    close(fd);
    return 0;
}
