#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    int fds[2];
    pipe(fds);

    int flags = fcntl(fds[1], F_GETFL);
    fcntl(fds[1], F_SETFL, flags | O_NONBLOCK);

    char buf[2000];

    printf("Writing (non-blocking)...\n");

    while (1) {
        ssize_t n = write(fds[1], buf, sizeof(buf));

        if (n < 0 && errno == EAGAIN) {
            printf("Pipe full (EAGAIN)\n");
            break;
        } else if (n > 0) {
            printf("wrote %zd bytes\n", n);
        } else {
            perror("write");
            break;
        }
    }
}
