#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./es4 <N1> <N2> ...\n");
        return 1;
    }
    int sizen = argc - 1;
    long* n = calloc(sizen, sizeof(long));
    for (int i = 0 ; i< sizen; i++) {
        n[i] = atol(argv[i+1]);
    }
    pid_t fork_result;

    for (int i = 0; i < sizen ; i++) {
        fork_result = fork();
        if (fork_result < 0) {
            printf("Errore nella fork.");
        }
        else if (fork_result == 0) {
            //child
            if (n[i] < 2) {
                _exit(0);
            }
            if (n[i] == 2) {
                printf("[CHILD %d] PID: %d, il numero %ld e' primo\n", i+1, getpid(), n[i]);                _exit(1);
            }
            if (n[i] % 2 == 0) {
                _exit(0);
            }
            for (long j=3; (j*j)<=n[i]; j+=2) {
                if (n[i] % j == 0) {
                    _exit(0);
                }
            }
            printf("[CHILD %d] PID: %d, il numero %ld e' primo\n", i+1, getpid(), n[i]);            _exit(1);
        }
    }

    int status;
    int prime = 0;
    for (int i =0; i<sizen; i++) {
        wait(&status);

        if (WIFEXITED(status)) {
            int child_returned = WEXITSTATUS(status);
            prime += child_returned;
        }
    }

    printf("[PADRE] Numero di figli con numeri primi: %d \n", prime);
    free(n);
    return 0;
}
