#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define FIFO_C1_TO_C2 "/tmp/fifo_c1_to_c2" //communication from client1 to client2
#define FIFO_C2_TO_C1 "/tmp/fifo_c2_to_c1" //communication from client2 to client1
#define FIFO_C1_TO_S  "/tmp/fifo_c1_to_server" //communication from client1 to server
#define FIFO_C2_TO_S  "/tmp/fifo_c2_to_server" //communication from client2 to server

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

int main() {





}

