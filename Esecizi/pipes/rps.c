#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#define ROUND 5
#define TOTROUND 10
#define PIPEN 2
#define ROCK 0
#define PAPER 1
#define SCISSORS 2


typedef struct {
    int pick;
    pid_t pid;
}msg ;



static volatile sig_atomic_t sigusr1_recived = 0;

static void handle_sigusr1(int sig) {
    sigusr1_recived = 1;
}

static void install_handler(int signo, void (*handler)(int)) {
    struct sigaction sa = {0};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signo, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

static void child_logic(int write_fd, int child_n) {
    srand(getpid());
    install_handler(SIGUSR1, handle_sigusr1);

    printf("[CHILD %d | PID: %d] Wait for round to start...\n", child_n, getpid());

    while (1) {
        pause();

        if (sigusr1_recived) {
            sigusr1_recived = 0;

            msg currentmsg;
            currentmsg.pid = getpid();
            currentmsg.pick = rand()%3;

            if (write(write_fd, &currentmsg, sizeof(msg)) == -1) {
                perror("write in child");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main(void) {
    int pipe1[PIPEN];
    int pipe2[PIPEN];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 ) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 < 0) { /* error occurred */
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        //figlio 1
        close(pipe1[0]);
        close(pipe2[0]);
        close(pipe2[1]);

        child_logic(pipe1[1], 1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) { /* error occurred */
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        //figlio 2
        close(pipe2[0]);
        close(pipe1[0]);
        close(pipe1[1]);

        child_logic(pipe2[1], 2);
    }

    //padre
    close(pipe1[1]);
    close(pipe2[1]);

    //srand(getpid());

    printf("[PARENT] ++++ GAME STARTED ++++\n");

    int score1 = 0, score2 = 0;
    for (int i=1; i<=TOTROUND && score1<=ROUND && score2<=ROUND; i++) {
        printf("[PARENT] Round %d, scores %d vs %d\n", i, score1, score2);
        printf("[PARENT] Press ENTER to start the RPS game...\n");

        int c;
        while ((c= getchar()) != '\n' && c != EOF);

        // int number = (rand()%10) +1;
        // printf("[PARENT] Parent choose %d\n", number);

        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);

        msg ch1, ch2;
        read(pipe1[0], &ch1, sizeof(msg));
        read(pipe2[0], &ch2, sizeof(msg));

        printf("[PARENT] Recived from child 1: %d\n", ch1.pick);
        printf("[PARENT] Recived from child 2: %d\n", ch2.pick);

        // int dist1 = abs(number - ch1.guess);
        // int dist2 = abs(number - ch2.guess);


        if ((ch1.pick == ROCK && ch2.pick == ROCK) || (ch1.pick == PAPER && ch2.pick == PAPER) ||
            (ch1.pick == SCISSORS && ch2.pick == SCISSORS)) {
            printf("[PARENT] TIE!\n");
            score2++;
            score1++;
        }
        else if ((ch1.pick == ROCK && ch2.pick == PAPER) || (ch1.pick == SCISSORS && ch2.pick == ROCK)
            || (ch1.pick == PAPER && ch2.pick == SCISSORS)) {
            printf("[PARENT] Child 2 WON the round\n");
            score2++;
        }
        else if ((ch1.pick == ROCK && ch2.pick == SCISSORS) || (ch1.pick == SCISSORS && ch2.pick == PAPER)
            || (ch1.pick == PAPER && ch2.pick == ROCK)) {
            printf("[PARENT] Child 1 WON the round\n");
            score1++;
        }
        else{
            perror("round");
        }
    }

    printf("\n[PARENT] ++++ GAME OVER ++++\n");

    kill(pid1, SIGTERM);
    kill(pid2, SIGTERM);

    close(pipe1[1]);
    close(pipe2[1]);

    wait(NULL);
    wait(NULL);

    printf("[PARENT] Goodbye!\n");

    return 0;
}
