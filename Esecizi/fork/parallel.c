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

static int is_prime(long x) {
    if (x<2) {
        return 0;
    }
    if (x == 2) {
        return 1;
    }
    if (x%2 == 0) {
        return 0;
    }
    for (long d = 3; d*d<=x; d+=2) {
        if (x%d == 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if(argc < 2){
        printf("Usage %s n\n", argv[0]);
        exit(1);
    }

    int len = argc-1;

    long* ns = calloc(sizeof(long), len);

    for (int i=0;i<len; i++) {
        ns[i] = parse_l(argv[i+1]);
    }

    pid_t forkresult;
    long current;
    int tot = 0;
    for (int i=0; i<len; i++) {
        forkresult = fork();

        if (forkresult < 0) {
            exit(-1);
        }
        else if (forkresult ==0 ) {
            current = ns[i];

            if (is_prime(current)) {
                printf("%ld is prime! Child: %d | %d \n",current, i+1, getpid());
                exit(1);
            }
            exit(0);
        }
        else {
            int status;

            waitpid(forkresult, &status, 0);
            if (WIFEXITED(status)) {
                int child_status = WEXITSTATUS(status);
                tot += child_status;
            }
            else {
                printf("Il bimbo non e' uscito normale \n");
            }

        }
    }
    printf("Prime number found: %d \n On a total of: %d \n", tot, len);

    free(ns);
    return 0;
}