#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1
#define MAX_RAND 10 //will generate random int from 1 to MAX_RAND (included)
#define WIN_SCORE 5

typedef struct {
    pid_t pid;
    int guess;
} GuessMsg;

volatile sig_atomic_t got_start_signal = 0;

void start_handler(int sig) {
    got_start_signal = 1;
}

void child_work(int write_fd) {
    struct sigaction sa = {0};
    sa.sa_handler = start_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    //initialize the random number generator with something that depends
    //both on the current time and the pid of the current process
    //this makes sure that both child will have different sequneces
    //of pseudorandom numbers
   
    srand((unsigned int)time(NULL) + (unsigned int)getpid());

    while (1) {
        pause();  // wait for parent to signal a new round

        if (!got_start_signal) {
	    // exit from pause() caused by a different signal
	    // go back to pause() again
            continue;
        }
	//handle git_start_signal right now
        got_start_signal = 0;

	//create a new message with my guess
        GuessMsg msg;
        msg.pid = getpid();
        msg.guess = (rand() % 10) + 1;

	//send my guess to parent
        if (write(write_fd, &msg, sizeof(msg)) != sizeof(msg)) {
            break; // parent probably closed pipe
        }
    }
    
    printf("Child %d exiting cleanly...\n",getpid());
    close(write_fd);
    _exit(0);
}

int distance_from_secret(int guess, int secret) {
    int d = guess - secret;
    return d < 0 ? -d : d;
}

int main(void) {
    int pipe1[2]; //will use this for child1 -> parent communication
    int pipe2[2]; //will use this for child2 -> parent communication

    // create the two pipes
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        return 1;
    }

    // create the first child
    pid_t c1 = fork();
    if (c1 == -1) {
        perror("fork");
        return 1;
    }
    if (c1 == 0) {
	/* I am child c1
	 * will now close the read end of pipe1 and both ends of pipe2
	 */
        close(pipe1[READ_END]);
        close(pipe2[READ_END]);
        close(pipe2[WRITE_END]);

	//now execute child_work on the write end of pipe1
        child_work(pipe1[WRITE_END]);
    }

    // I am the parent

    // create the second child
    pid_t c2 = fork();
    if (c2 == -1) {
        perror("fork");
        return 1;
    }
    if (c2 == 0) {
	/* I am child c2
	 * will now close the read end of pipe2 and both ends of pipe1
	 */
        close(pipe2[READ_END]);
        close(pipe1[READ_END]);
        close(pipe1[WRITE_END]);

	//now execute child_work on the write end of pipe2
        child_work(pipe2[WRITE_END]);
    }

    /* I am the parent
     * will now close the write end of both pipes
     */
    close(pipe1[WRITE_END]);
    close(pipe2[WRITE_END]);

    //init the random number generator of the parent
    srand((unsigned int)time(NULL)+(unsigned int)getpid());

    // initialize game state
    int score1 = 0;
    int score2 = 0;
    int round = 1;
    ssize_t read_bytes = 0;

    printf("=== Guess Race ===\n");
    printf("Two child processes compete by guessing a number from 1 to %d.\n",MAX_RAND);
    printf("Parent uses SIGUSR1 to start each round; children reply through pipes.\n");
    printf("First to %d points wins.\n\n",WIN_SCORE);


    while (score1 < WIN_SCORE && score2 < WIN_SCORE) { // main loop of the game
        char line[32];
        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
	
	// generate secret number ot guess for this round
        int secret = (rand() % MAX_RAND) + 1;

	// wake up both children 
        kill(c1, SIGUSR1);
        kill(c2, SIGUSR1);

        GuessMsg g1, g2;

	// read guess from child 1 
	// this will block the parent until there is something to read
	read_bytes = read(pipe1[READ_END], &g1, sizeof(g1));
	if (read_bytes != sizeof(g1)) {
            fprintf(stderr, "Failed to read child 1 guess\n");
            break;
        }
	
	// read guess from child 2 
	// this will block the parent until there is something to read
	read_bytes = read(pipe2[READ_END], &g2, sizeof(g2));
	if (read_bytes != sizeof(g2)) {
            fprintf(stderr, "Failed to read child 2 guess\n");
            break;
        }

        printf("\nRound %d\n", round);
        printf("Secret number: %d\n", secret);
        printf("Child 1 (pid %d) guessed %d\n", g1.pid, g1.guess);
        printf("Child 2 (pid %d) guessed %d\n", g2.pid, g2.guess);

	// check who wins this round and update game state
        int d1 = distance_from_secret(g1.guess, secret);
        int d2 = distance_from_secret(g2.guess, secret);

        if (d1 < d2) {
            score1++;
            printf("-> Child 1 wins the round\n");
        } else if (d2 < d1) {
            score2++;
            printf("-> Child 2 wins the round\n");
        } else {
            score1++;
            score2++;
            printf("-> Tie: both get a point\n");
        }

        printf("Score: Child 1 = %d, Child 2 = %d\n\n", score1, score2);
        round++;
    }

    // out of the main game loop, at least one child reached WIN_SCORE
    // or some error condition happened. The game is finished
    if (score1 > score2) {
        printf("Child 1 wins the game!\n");
    } else if (score2 > score1) {
        printf("Child 2 wins the game!\n");
    } else {
        printf("The game ends in a tie!\n");
    }

    printf("Parent exiting cleanly...\n");

    // terminate both children
    // note: children do not handle sigterm, hence they do not
    // exit cleanly (e.g. close the pipe) after receiving it. 
    // this is something you should now how to fix...
    kill(c1, SIGTERM);
    kill(c2, SIGTERM);

    // wait for the child to terminate
    waitpid(c1, NULL, 0);
    waitpid(c2, NULL, 0);

    // close the pipes
    close(pipe1[READ_END]);
    close(pipe2[READ_END]);

    return 0;
}
