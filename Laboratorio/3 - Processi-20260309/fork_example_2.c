#include <stdio.h>
#include <unistd.h>

int main() {
    int x = 10;

    pid_t fork_result = fork();

    if (fork_result == 0) {
        // Child
        x += 5;
        printf("Child: x = %d\n", x);
    } else {
        // Parent
        x -= 5;
        printf("Parent: x = %d\n", x);
    }

    return 0;
}
