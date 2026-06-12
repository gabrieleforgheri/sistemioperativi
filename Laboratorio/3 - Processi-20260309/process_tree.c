#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    pid_t c1, c2, gc;

    printf("PARENT  pid=%d ppid=%d\n", getpid(), getppid());

    // First child
    c1 = fork();
    if (c1 < 0) { perror("fork c1"); exit(1); }

    if (c1 == 0) {
        // In CHILD 1
        printf("CHILD1  pid=%d ppid=%d\n", getpid(), getppid());

        // Grandchild from child1
        gc = fork();
        if (gc < 0) { perror("fork gc"); exit(1); }

        if (gc == 0) {
            // In GRANDCHILD
            printf("GRANDC  pid=%d ppid=%d\n", getpid(), getppid());
            _exit(0);
        } else {
            // Child1 waits for grandchild
            int st;
            waitpid(gc, &st, 0);
            printf("CHILD1  pid=%d: grandchild done\n", getpid());
            _exit(0);
        }
    }

    // Back in PARENT: create second child
    c2 = fork();
    if (c2 < 0) { perror("fork c2"); exit(1); }

    if (c2 == 0) {
        // In CHILD 2
        printf("CHILD2  pid=%d ppid=%d\n", getpid(), getppid());
        _exit(0);
    }

    // Parent waits for both children
    int st1, st2;
    waitpid(c1, &st1, 0);
    waitpid(c2, &st2, 0);
    printf("PARENT  pid=%d: children done\n", getpid());

    return 0;
}
