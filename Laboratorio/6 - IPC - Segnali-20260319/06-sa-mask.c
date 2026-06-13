#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Handling signal %d\n", sig);
    sleep(5);  // simulate long handler
    printf("Handler finished\n");
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    /* Block SIGTERM while handler runs */
    sigaddset(&sa.sa_mask, SIGTERM);

    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    printf("Send SIGINT (Ctrl+C). SIGTERM will be blocked during handler.\n");

    while (1) {
        printf("Process %d running...\n",getpid());
	sleep(1);
    }

    return 0;
}
