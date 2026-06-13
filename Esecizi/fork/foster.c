#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static long parse_l(const char *s) {
    char *end = NULL;
    errno = 0;
    long value = strtol(s, &end, 10);

    if (errno != 0 || *end != '\0') {
        fprintf(stderr, "Invalid %s\n", s);
        exit(EXIT_FAILURE);
    }

    return value;
}

int main(int argc, char* argv[]){
    if(argc != 2){
        printf("Usage %s n\n", argv[0]);
        exit(1);
    }

    int max = 0;
    pid_t maxpid = 0;

    pid_t forkresult;

    long n = parse_l(argv[1]);

    if (n < 1) {
        printf("Usage %s n\n", argv[0]);
        exit(1);
    }

    for(int i=0; i<n; i++){
        forkresult = fork();

        if (forkresult < 0) {
            printf("Kablao\n");
            exit(1);
        }
        else if (forkresult == 0) {
            printf("CHILD %d, PID %d, PARENT PID %d\n", i, getpid(), getppid());
            sleep((10));
            printf("CHILD %d, PID %d, PARENT PID %d\n", i, getpid(), getppid());

            _exit(0);
        }
        else {
            //parent
            sleep(5);
            exit(0);
        }

    }
    return 0;
}