#include <stdio.h>
#include <unistd.h>

int main() {
    int fds[2];
    pipe(fds);

    char buf[2000]; // 

    printf("Writing a lot (may BLOCK)...\n");
    while (1) {
        write(fds[1], buf, sizeof(buf));
        printf("wrote chunk\n");
    }
}
