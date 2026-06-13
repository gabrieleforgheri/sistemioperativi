#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Caught signal %d\n", sig);
}

int main(void) {
    /* Install handler using the deprecated signal() API */
    signal(SIGINT, handler);

    while (1) {
    	printf("Process %d running... Press Ctrl+C to trigger SIGINT.\n",getpid());
        sleep(1);
    }

    return 0;
}
