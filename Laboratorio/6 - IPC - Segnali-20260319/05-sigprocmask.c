#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void) {
    sigset_t newmask, oldmask;

    /* Create a signal set containing SIGINT */
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);

    /* Block SIGINT and save the old mask */
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);
    printf("SIGINT blocked for 10 seconds. Press Ctrl+C now...\n");

    for(int i=0; i<10; i++){
        printf("Process %d running...\n",getpid());
	sleep(1);
	} 

    /* Restore the previous signal mask */
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    printf("SIGINT unblocked\n");

    /* Wait so you can press Ctrl+C again */
    while (1) {
        printf("Process %d running...\n",getpid());
	sleep(1);
    }

    return 0;
}
