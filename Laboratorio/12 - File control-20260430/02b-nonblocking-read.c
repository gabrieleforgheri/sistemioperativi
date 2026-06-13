#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int main() {
    int fds[2];
    pipe(fds);

    int flags = fcntl(fds[0], F_GETFL);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

    char buf[16];

    printf("About to read (non-blocking)...\n");
    ssize_t n = read(fds[0], buf, sizeof(buf));

    if (n < 0 && errno == EAGAIN)
        printf("No data available (EAGAIN)\n");
    else
        printf("Read returned: %zd\n", n);
}
