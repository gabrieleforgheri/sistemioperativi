#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define N 5

static volatile sig_atomic_t sigusr1_received = 0;
static volatile sig_atomic_t sigint_received = 0;
static volatile sig_atomic_t sigalarm_recived = 0;

static void handle_sigusr1(int sig){
    sigusr1_received = 1;
}

static void handle_sigint(int sig){
    sigint_received = 1;
}

static void handle_sigalrm(int sig) {
    sigalarm_recived = 1;
    alarm(N);
}

static void print_status(int iteration_counter){
    printf("Iterated %d times so far\n",iteration_counter);
}

static void handle_received_signals(int iteration_counter) {
    if(sigusr1_received == 1){
        sigusr1_received = 0;
        printf("[Debug] Segnale ricevuto correttamente \n");
        print_status(iteration_counter);
    }

    if (sigalarm_recived == 1) {
        sigalarm_recived = 0;
        printf("Alarm recived \n");
    }
}

int main(int argc, char *argv[]) {
    bool test = 0;

    if (argc > 2 ) {
        printf("Usage: ./warmupB <-t> \n");
        return 0;
    }
    if (argc == 2) {
        if (strcmp(argv[1], "-t") == 0) {
            test = 1;
        }
        else {
            printf("Usage: ./warmupB <-t> \n");
        }

    }

    struct sigaction sa={0};

    /*register handler for SIGUSR1*/
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1,&sa,NULL);

    /*register handler for SIGINT*/
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    //register handler for sigalrm
    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    if (test) {
        printf("[Debug] Provo SIGUSR1 \n");
        raise(SIGUSR1);
    }

    int iteration_counter=0;
    alarm(N);
    /* this is the main loop, make sure to check for
     * received signals within this loop */
    int i;
    for(i=0; i<3600 && !sigint_received; i++) {
        printf("Faccio qualcosa [%d] \n",getpid());
        sleep(1);
        handle_received_signals(i);
    }

    printf("Completed %d iterations. Exiting gracefully...\n",i);

    return 0;
}
