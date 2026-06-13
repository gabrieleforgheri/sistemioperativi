#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("SIGALRM received\n");
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGALRM, &sa, NULL);

    printf("Setting alarm for 5 seconds...\n");
    alarm(5);

    printf("Waiting for alarm...\n");
    pause();

    printf("Program continues after alarm\n");

    return 0;
}
