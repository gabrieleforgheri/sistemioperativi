#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fds[2];
    pipe(fds);

    char buf[16];

    printf("About to read (this will BLOCK)...\n");
    ssize_t n = read(fds[0], buf, sizeof(buf));

    printf("Read returned: %zd\n", n);
}
