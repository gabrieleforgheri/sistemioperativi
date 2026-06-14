#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1
#define ROCK 0
#define PAPER 1
#define SCISSORS 2
#define LOST 0
#define WON 1
#define TIE 2
#define WIN_SCORE 5

typedef struct {
    pid_t pid;
    int rps; //this will contain either ROCK, PAPER or SCISSORS
} RPSMsg;

typedef struct {
    pid_t pid;
    int result; //this will contain LOST, WON or TIE    
} ResultMsg;

volatile sig_atomic_t got_start_signal = 0;

void start_handler(int sig) {
    got_start_signal = 1;
}

int check_rps_winner(int mine, int his) {
	if (mine == his) return TIE;
	if (mine == ROCK && his == SCISSORS) return WON;
	if (mine == PAPER && his == ROCK ) return WON;
	if (mine == SCISSORS && his == PAPER) return WON;
	return LOST;
}

char* printable_result(int result){
	if (result == WON) return "won";
	if (result == LOST) return "lost";
	if (result == TIE) return "tied";

}

char* printable_rps(int rps){
	if (rps == ROCK) return "rock";
	if (rps == PAPER) return "paper";
	if (rps == SCISSORS) return "scissors";
}

void child_work(int write_to_parent_fd, int write_to_brother_fd, int read_from_brother_fd) {
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
	//handle got_start_signal
        got_start_signal = 0;

	//create a new message with my guess
        RPSMsg my_rps_msg;
        my_rps_msg.pid = getpid();
        my_rps_msg.rps = (rand() % 3); //generate 

	printf("My pid is %d and I played %s\n",my_rps_msg.pid,printable_rps(my_rps_msg.rps));

	//send my rps to brother
        if (write(write_to_brother_fd, &my_rps_msg, sizeof(my_rps_msg)) != sizeof(my_rps_msg)) {
            break; // parent probably closed pipe
        }

	//read rps from brother
	RPSMsg his_rps_msg;
	int read_bytes = read(read_from_brother_fd, &his_rps_msg, sizeof(his_rps_msg));
        if (read_bytes != sizeof(his_rps_msg)) {
            fprintf(stderr, "Failed to read brother rps\n");
            break;
        }

	//did I won?
	int rps_result = check_rps_winner(my_rps_msg.rps, his_rps_msg.rps);
	
	//send result to parent
	ResultMsg msg;
	msg.pid = getpid();
	msg.result = rps_result;

        //send my rps to brother
        if (write(write_to_parent_fd, &msg, sizeof(msg)) != sizeof(msg)) {
            break; // parent probably closed pipe
        }

    }
    
    printf("Child %d exiting cleanly...\n",getpid());
    close(write_to_parent_fd);
    close(write_to_brother_fd);
    close(read_from_brother_fd);
    _exit(0);
}

int main(void) {
    int pipe1top[2]; //will use this for child1 -> parent communication
    int pipe2top[2]; //will use this for child2 -> parent communication
    int pipe1to2[2]; //will use this for child1 -> child2 communication
    int pipe2to1[2]; //will use this for child2 -> child1 communication

    // create all the pipes
    if (pipe(pipe1top) == -1 || 
	pipe(pipe2top) == -1 || 
	pipe(pipe1to2) == -1 || 
	pipe(pipe2to1) == -1 )   
    {
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
	 * will now close the unised pipes
	 */
        close(pipe1top[READ_END]);
        close(pipe2top[READ_END]);
        close(pipe2top[WRITE_END]);
        close(pipe1to2[READ_END]);
        close(pipe2to1[WRITE_END]);

	//now execute child_work on the write end of pipe1
        child_work(pipe1top[WRITE_END],pipe1to2[WRITE_END],pipe2to1[READ_END]);
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
        close(pipe1top[READ_END]);
        close(pipe1top[WRITE_END]);
        close(pipe2top[READ_END]);
        close(pipe1to2[WRITE_END]);
        close(pipe2to1[READ_END]);

	//now execute child_work on the write end of pipe2
        child_work(pipe2top[WRITE_END],pipe2to1[WRITE_END],pipe1to2[READ_END]);
        
    }

    /* I am the parent
     * will now close the unused end of all pipes
     */
    close(pipe1top[WRITE_END]);
    close(pipe2top[WRITE_END]);
    close(pipe1to2[READ_END]);
    close(pipe1to2[WRITE_END]);
    close(pipe2to1[READ_END]);
    close(pipe2to1[WRITE_END]);

    // initialize game state
    int score1 = 0;
    int score2 = 0;
    int round = 1;
    ssize_t read_bytes = 0;

    printf("=== Rock, Paper, Scissors ===\n");
    printf("Two child processes play rock, paper, scissors\n");
    printf("Parent uses SIGUSR1 to start each round; children reply through pipes.\n");
    printf("First to %d points wins.\n\n",WIN_SCORE);


    while (score1 < WIN_SCORE && score2 < WIN_SCORE) { // main loop of the game
        char line[32];
        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
	
	// wake up both children 
        kill(c1, SIGUSR1);
        kill(c2, SIGUSR1);

        ResultMsg g1, g2;

	// read result from child 1 
	// this will block the parent until there is something to read
	read_bytes = read(pipe1top[READ_END], &g1, sizeof(g1));
	if (read_bytes != sizeof(g1)) {
            fprintf(stderr, "Failed to read child 1 result\n");
            break;
        }
	
	// read result from child 2 
	// this will block the parent until there is something to read
	read_bytes = read(pipe2top[READ_END], &g2, sizeof(g2));
	if (read_bytes != sizeof(g2)) {
            fprintf(stderr, "Failed to read child 2 result\n");
            break;
        }

        printf("\nRound %d\n", round);
        printf("Child 1 (pid %d) %s\n", g1.pid, printable_result(g1.result));
        printf("Child 2 (pid %d) %s\n", g2.pid, printable_result(g2.result));

	// check who wins this round and update game state
        if (g1.result == WON && g2.result == LOST) {
            score1++;
            printf("-> Child 1 wins the round\n");
        } else if (g1.result == LOST && g2.result == WON) {
            score2++;
            printf("-> Child 2 wins the round\n");
        } else if (g1.result == TIE && g2.result == TIE){
            printf("-> Tie: no points!\n");
        } else {
	    printf("Someone is cheating! I am closing the game!");
            break;
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
    close(pipe1top[READ_END]);
    close(pipe2top[READ_END]);

    return 0;
}
