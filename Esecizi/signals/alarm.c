#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t sigusr1_received = 0;
static volatile sig_atomic_t sigint_received = 0;
static volatile sig_atomic_t sigarlm_recived = 0;

static void handle_sigusr1(int sig){
	sigusr1_received = 1;
}

static void handle_sigint(int sig){
	sigint_received = 1;
}

static void handle_sigalrm(int sig){
    sigarlm_recived = 1;
}

static void print_status(int iteration_counter){
	printf("Iterated %d times so far\n",iteration_counter);
}

static void handle_received_signals(int iteration_counter) {
	if(sigusr1_received == 1){
		sigusr1_received = 0;
		print_status(iteration_counter); 
	}
    if(sigarlm_recived == 1){
        sigarlm_recived = 0;
        printf("Alarm recived\n");
        alarm(5);
    }
}

int main(int argc, char *argv[]) {

    struct sigaction sa={0};
    
    /*register handler for SIGUSR1*/
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1,&sa,NULL);
    
    /*register handler for SIGINT*/
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,&sa,NULL);

    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);


    int iteration_counter=0;

    alarm(5);

    /* this is the main loop, make sure to check for
     * received signals within this loop */
    int i;
    for(i=0; i<3600 && !sigint_received; i++) {
	    printf("Process %d pretending to do something useful...\n",getpid());
        sleep(1);
	    handle_received_signals(i);
    }

    printf("Completed %d iterations. Exiting gracefully...\n",i);

    return 0;
}
