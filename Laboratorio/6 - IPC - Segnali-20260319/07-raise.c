#include <stdio.h>
#include <signal.h>

void handler(int sig) {
    static int count = 0;

    printf("Handler start (count=%d)\n", count);

    if (count == 0) {
        count++;
        printf("Raising SIGINT again inside handler\n");
        raise(SIGINT);
    }

    printf("Handler end\n");
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);   /* no additional blocked signals */
    sa.sa_flags = SA_NODEFER; //try to set to SA_NODEFER and see the difference

    sigaction(SIGINT, &sa, NULL);

    printf("Sending SIGINT to myself\n");
    raise(SIGINT);

    printf("Back in main\n");
    return 0;
}
