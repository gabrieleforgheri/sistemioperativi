#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig) {
    printf("Caught signal %d\n", sig);
}

int main(void) {
    struct sigaction sa;

    sa.sa_handler = handler;     /* function to handle the signal */
    sigemptyset(&sa.sa_mask);    /* no additional signals blocked */
    sa.sa_flags = 0;             /* no special flags */

    sigaction(SIGINT, &sa, NULL);  /* install handler */
    sigaction(SIGTERM, &sa, NULL);  /* install handler */
    sigaction(SIGKILL, &sa, NULL);  /* install handler */


    while (1) {
    	printf("Process %d running...\n",getpid());
	sleep(1);
    }

    return 0;
}
