#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

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
    int rps; //this will contain either ROCK, PAPER or SCISSORS
} RPSMsg;

typedef struct {
    pid_t pid;
    int result; //this will contain LOST, WON or TIE    
} ResultMsg;

typedef struct {
    pid_t pid;
} RegisterMsg;

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

char* printable_rps(int rps){
        if (rps == ROCK) return "rock";
        if (rps == PAPER) return "paper";
        if (rps == SCISSORS) return "scissors";
	return "should not happen...";
}

int main(int argc, char* argv[]) {

    int from_other_client_fd;
    int to_other_client_fd;
    int to_server_fd;
    int myself; // 1 or 2
    int other; // 1 or 2
    int read_bytes, write_bytes;
    RegisterMsg reg_msg;
    ResultMsg res_msg;
    RPSMsg my_rps_msg, his_rps_msg;

    struct sigaction sa = {0};
    sa.sa_handler = start_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    //initialize the random number generator with something that depends
    //both on the current time and the pid of the current process
    //this makes sure that both child will have different sequneces
    //of pseudorandom numbers

    srand((unsigned int)time(NULL) + (unsigned int)getpid());


    /* I am a client process.
     * If argv[1] == "1" I will behave as client1. 
     * If argv[1] == "2" I will behave as client2.
     */
    if (argc != 2 || ( strcmp(argv[1],"1") !=0 && strcmp(argv[1],"2") !=0 ) ){
	    printf("Usage: %s <n>, where <n> can be 1 or 2\n",argv[0]);
	    exit(1);
    }

    if (!strcmp(argv[1],"1")){
	//I am client1!
        myself = 1;
	other = 2;

        //open the FIFOs on which to read from the other client       
	//NOTE: if we do not open the fifo in read mode, any other
	//process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C2_TO_C1, O_RDONLY);

        //open the FIFOs on which to write to the other client       
        to_other_client_fd = open(FIFO_C1_TO_C2, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C1_TO_S, O_WRONLY);

    } else {
	//I am client2!
	myself = 2;
	other = 1;
        
        //open the FIFOs on which to read from the other client       
	//NOTE: if we do not open the fifo in read mode, any other
	//process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C1_TO_C2, O_RDONLY);

	//open the FIFOs on which to write to the other client       
        to_other_client_fd = open(FIFO_C2_TO_C1, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C2_TO_S, O_WRONLY);
    }

    printf("I am Client%d, my pid is %d\n", myself, getpid());
    
    if (from_other_client_fd == -1 || to_other_client_fd == -1 || to_server_fd == -1) {
        perror("open");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }

    // create and send the registration message to send to the server
    reg_msg.pid = getpid();
    write_bytes = write(to_server_fd, &reg_msg, sizeof(reg_msg));
    if (write_bytes != sizeof(reg_msg)) {
        perror("Could not write a complete message:");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }

    printf("I am Client %d and I sent the registration message. Starting the game loop...\n",myself);

    while (1) {
        pause();  // wait for server to signal a new round

        if (!got_start_signal) {
            // exit from pause() caused by a different signal
            // go back to pause() again
            continue;
        }
        //handle got_start_signal
        got_start_signal = 0;

        //create a new message with my guess
        my_rps_msg.pid = getpid();
        my_rps_msg.rps = (rand() % 3); //generate 

        printf("I am Client %d and I played %s\n", myself, printable_rps(my_rps_msg.rps));

        //send my rps to other client
        if (write(to_other_client_fd, &my_rps_msg, sizeof(my_rps_msg)) != sizeof(my_rps_msg)) {
            break; // other client probably closed pipe
        }

        //read rps from other client
        RPSMsg his_rps_msg;
        int read_bytes = read(from_other_client_fd, &his_rps_msg, sizeof(his_rps_msg));
        if (read_bytes != sizeof(his_rps_msg)) {
            fprintf(stderr, "Failed to read other client's rps\n");
            break;
        }

        //did I won?
        int rps_result = check_rps_winner(my_rps_msg.rps, his_rps_msg.rps);

        //send result to parent
        res_msg.pid = getpid();
        res_msg.result = rps_result;

        //send result to server
        if (write(to_server_fd, &res_msg, sizeof(res_msg)) != sizeof(res_msg)) {
            break; // parent probably closed pipe
        }

    }

    printf("Child %d exiting cleanly...\n",getpid());
    close(to_server_fd);
    close(to_other_client_fd);
    close(from_other_client_fd);
    exit(0);

}
