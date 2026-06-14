#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define FIFO_C1_TO_C2 "/tmp/fifo_c1_to_c2" //communication from client1 to client2
#define FIFO_C2_TO_C1 "/tmp/fifo_c2_to_c1" //communication from client2 to client1
#define FIFO_C1_TO_S  "/tmp/fifo_c1_to_server" //communication from client1 to server
#define FIFO_C2_TO_S  "/tmp/fifo_c2_to_server" //communication from client2 to server

#define ROCK 0
#define PAPER 1
#define SCISSORS 2
#define LOST 0
#define WON 1
#define TIE 2
#define WIN_SCORE 5

typedef struct {
    pid_t pid;
    int result; //this will contain LOST, WON or TIE    
} ResultMsg;

typedef struct {
    pid_t pid;
} RegisterMsg;

int create_and_open_fifo_or_die(const char *fifo_name) {
    /* create one FIFO with the given name
     * note: if the FIFO already exists, mkfifo() fails with EEXIST.
     * in that case, we simply reuse the existing FIFO.
     */
    if (mkfifo(fifo_name, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(1);
        }
    }
    
    /* open the FIFOs on which to wait for clients	
     * NOTE: FIFOs are opened with the O_RDRW flag
     * This *DOES NOT* make them bidirectional!
     * A fifo (as a pipe) is unidirectional! 
     * One process writes to the fifo, another
     * process reads from the fifo.
     *
     * The "server" process opens all FIFOs in RDWR mode
     * to avoid other processes that only open them in RD or 
     * RW mode to blok on open
     */

    int fd = open(fifo_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    return fd;
}
	
char* printable_result(int result){
        if (result == WON) return "won";
        if (result == LOST) return "lost";
        if (result == TIE) return "tied";

}

int main(void) {
    // create all FIFOs needed for communication
    int fifo_c1_to_c2_fd = create_and_open_fifo_or_die(FIFO_C1_TO_C2);
    int fifo_c2_to_c1_fd = create_and_open_fifo_or_die(FIFO_C2_TO_C1);
    int fifo_c1_to_s_fd = create_and_open_fifo_or_die(FIFO_C1_TO_S);
    int fifo_c2_to_s_fd = create_and_open_fifo_or_die(FIFO_C2_TO_S);

    // initialize game state
    int score1 = 0;
    int score2 = 0;
    int round = 1;
    ssize_t read_bytes = 0;
    ResultMsg res1, res2;
    RegisterMsg reg1, reg2;
    
    printf("=== Rock, Paper, Scissors ===\n");
    printf("Two child processes play rock, paper, scissors\n");
    printf("Parent uses SIGUSR1 to start each round; children reply through pipes.\n");
    printf("First to %d points wins.\n\n",WIN_SCORE);
    printf("\n\nWaiting for two clients to start register...\n\n");

    // read registration from client 1
    // this will block the parent until there is something to read
    read_bytes = read(fifo_c1_to_s_fd, &reg1, sizeof(reg1));
    if (read_bytes != sizeof(reg1)) {
        fprintf(stderr, "Failed to read client 1 registration\n");
        exit(1);
    }

    // read result from client 2 
    // this will block the parent until there is something to read
    read_bytes = read(fifo_c2_to_s_fd, &reg2, sizeof(reg2));
    if (read_bytes != sizeof(reg2)) {
        fprintf(stderr, "Failed to read child 2 result\n");
        exit(1);
    }

    printf("Registered clients with PID %d and %d. Let us start the game!\n",reg1.pid,reg2.pid);

    while (score1 < WIN_SCORE && score2 < WIN_SCORE) { // main loop of the game
        char line[32];
        printf("Press ENTER for round %d...", round);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // wake up both children 
        kill(reg1.pid, SIGUSR1);
        kill(reg2.pid, SIGUSR1);


        // read result from client 1
        // this will block the parent until there is something to read
        read_bytes = read(fifo_c1_to_s_fd, &res1, sizeof(res1));
        if (read_bytes != sizeof(res1)) {
            fprintf(stderr, "Failed to read child 1 result\n");
            break;
        }

        // read result from client 2 
        // this will block the parent until there is something to read
        read_bytes = read(fifo_c2_to_s_fd, &res2, sizeof(res2));
        if (read_bytes != sizeof(res2)) {
            fprintf(stderr, "Failed to read child 2 result\n");
            break;
        }

        printf("\nRound %d\n", round);
        printf("Client 1 (pid %d) %s\n", res1.pid, printable_result(res1.result));
        printf("Client 2 (pid %d) %s\n", res2.pid, printable_result(res2.result));

        // check who wins this round and update game state
        if (res1.result == WON && res2.result == LOST) {
            score1++;
            printf("-> Client 1 wins the round\n");
        } else if (res1.result == LOST && res2.result == WON) {
            score2++;
            printf("-> Client 2 wins the round\n");
        } else if (res1.result == TIE && res2.result == TIE){
            printf("-> Tie: no points!\n");
        } else {
            printf("Someone is cheating! I am closing the game!");
            break;
        }

        printf("Score: Client 1 = %d, Client 2 = %d\n\n", score1, score2);
        round++;
    }

    // out of the main game loop, at least one child reached WIN_SCORE
    // or some error condition happened. The game is finished
    if (score1 > score2) {
        printf("Client 1 wins the game!\n");
    } else if (score2 > score1) {
        printf("Client 2 wins the game!\n");
    } else {
        printf("The game ends in a tie!\n");
    }

    printf("Server exiting cleanly...\n");

    // terminate both children
    // note: children do not handle sigterm, hence they do not
    // exit cleanly (e.g. close the pipe) after receiving it. 
    // this is something you should now how to fix...
    kill(reg1.pid, SIGTERM);
    kill(reg2.pid, SIGTERM);

    close(fifo_c1_to_c2_fd);
    close(fifo_c2_to_c1_fd);
    close(fifo_c1_to_s_fd);
    close(fifo_c2_to_s_fd);

    unlink(FIFO_C1_TO_C2);
    unlink(FIFO_C2_TO_C1);
    unlink(FIFO_C1_TO_S);
    unlink(FIFO_C2_TO_S);

    return 0;
}
