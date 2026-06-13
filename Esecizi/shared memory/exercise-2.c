#include <ctype.h>   //for toupper() function
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_SIZE 1024
#define EMPTY 0
#define DATA_READY 1
#define RESULT_READY 2

/*
 * place here the flags for handling signals
 */

static volatile sig_atomic_t sigusr1_recived = 0;
static volatile sig_atomic_t sigusr2_recived = 0;
static volatile sig_atomic_t sigint_recived = 0;

/*
 * place hear the signal handler functions
 */
static void handle_sigusr1(int sig) {
	sigusr1_recived = 1;
}

static void handle_sigusr2(int sig) {
	sigusr2_recived = 1;
}

static void handle_sigint(int sig) {
	sigint_recived = 1;
}

void handler(int sig) {

}

typedef struct {
	char input[SHM_SIZE];
	char output[SHM_SIZE];
	int status;
}shared_data;



static void install_handler(int signo, void (*handler)(int)) {
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(signo, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static void to_uppercase(char *s) {
    for (; *s != '\0'; ++s) {
        *s = (char)toupper((unsigned char)*s);
    }
}

int main(void) {

    // create shared memory area with mmap()
	shared_data *shm = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (shm == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	shm->status = 0;

    pid_t pid = fork();

    if (pid == 0) {
		// Child process

		//install required signal handlers
		install_handler(SIGUSR1, handle_sigusr1);
    	install_handler(SIGUSR2, handle_sigusr2);
    	install_handler(SIGINT, handle_sigint);


        while(1) {
	    /*
	     * main loop of the child
	     *
	     * 1- call pause()
	     * 2- check if sigint and in case break the cicle
	     * 3- copy data from shared memory
	     * 4- convert to uppercase
	     * 5- print it to stdout
	     * 6- send sigusr2 to parent
	     */
        	pause();
        	if (sigint_recived) {
				break;
        	}
        	if (sigusr1_recived) {
        		sigusr1_recived = 0;
				while (shm->status != DATA_READY) {
					sleep(1);
				}
        		char data[SHM_SIZE];
        		strcpy(data, shm->input);
        		shm->status = 0;

        		to_uppercase(data);
        		printf("[CHILD] String converted: %s\n", data);

        		strcpy(shm->output, data);
        		shm->status = 2;

        		kill(getppid(), SIGUSR2);
        	}
        	/*
				 * out of the main loop, we received a sigint!
				 *
				 * 1- unmap shared memory
				 * 2- print nice goodbye message
				 * 3- exit
				 */
        }
    	munmap(shm, SHM_SIZE * sizeof(char));
    	printf("\n[CHILD] Goodbye!\n");
    	exit(0);
        }


    // parent process

    //install required signal handlers
	install_handler(SIGUSR1, handle_sigusr1);
	install_handler(SIGUSR2, handle_sigusr2);
	install_handler(SIGINT, handle_sigint);

    while(1) {

	/*
	 * main loop of the parent
	 *
	 * 1- ask for user input
	 * 2- read user input
	 * 3- check for sigint and in case break the cycle
	 * 4- copy user input into shared memory
	 * 5- send sigusr1 to child
	 * 6- call pause
	 * 7- check for sigint and in case break the cycle
	 */
    	char input[SHM_SIZE];
    	printf("[PARENT] Write a sentence\n");
    	if (fgets(input, SHM_SIZE, stdin) == NULL) {
    		if (sigint_recived) {
    			break;
    		}
    	}

    	input[strcspn(input, "\n")] = 0;

    	while (shm->status != EMPTY) {
    		sleep(1);
    	}
    	memcpy(shm->input, input, sizeof(char)*SHM_SIZE);
    	shm->status = 1;

		kill(pid, SIGUSR1);
    	pause();

    	while (shm->status != RESULT_READY) {
    		sleep(1);
    	}

    	printf("[PARENT] Child has produced: %s\n", shm->output);
    	shm->status = 0;

    	if (sigusr2_recived) {
    		sigusr2_recived = 0;
    	}

    	if (sigint_recived) {
    		break;
    	}
    }

    /*
     * out of the main loop, we received a sigint!
     *
     * 1- send sigint to child
     * 2- wait for the child
     * 3- unmap shared memory
     * 4- print nice goodbye message
     * 3- exit
     */

	kill(pid, SIGINT);
	wait(NULL);
	munmap(shm, sizeof(shared_data));
	printf("\n[PARENT] Goodbye!\n");
	exit(EXIT_SUCCESS);
}
