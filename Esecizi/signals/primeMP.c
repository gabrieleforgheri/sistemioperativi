#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#DEFINE N 10

/*
 * place here the flags that are initialized to 0
 * and will be set to 1 by the handler for each
 * handled signal
*/
static volatile sig_atomic_t sigusr1_recived = 0;
static volatile sig_atomic_t sigusr2_recived = 0;
static volatile sig_atomic_t sigint_recived = 0;

/*
 * place here the simple handlers that, when executed,
 * will set the corresponding flag to 1
 * 
*/
void handle_sigusr1(int sig) {
    sigusr1_recived = 1;
}
void handle_sigusr2(int sig) {
    sigusr2_recived = 1;
}
void handle_sigint(int sig) {
    sigint_recived = 1;
}

/* function to parse a string containing a number into a long
 * read the manpage of strtol [man 3 strtol] to better understand
 * how it works, its results and possible error conditions. 
 * This function might prove useful during the final exam...
 */
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

/* function to print progress information. This is what
 * should be printed when SIGUSR1 is received
 */
static void print_progress(long factored_count, long prime_count) {
    printf("Numbers factored so far: %lu \n", factored_count);
    printf("Primes found so far: %lu \n", prime_count);
}


/* function to print the last prime found. This is what
 * should be printed when SIGUSR2 is received
 */
static void print_last_prime(long last_prime) {
    printf("Last prime found so far: %lu \n", last_prime);
}

/* function to print the final summary. This is what
 * should be printed when the program terminates
 * gracefully
 */
static void print_final_summary(long factored_count, long prime_count, long last_prime) {
    printf("\n-- Final summary --\n");
    printf("Factored %lu numbers\n", factored_count);
    printf("Found %lu prime numbers\n", prime_count);
    printf("Largest prime found: %lu\n",last_prime);
}

int handle_recived_signals(long factored_count, long prime_count, long last_prime) {
    if (sigusr1_recived) {
        sigusr1_recived = 0;
        printf("SIGUSR1 Recived \n");
        print_progress(factored_count, prime_count);
    }
    if (sigusr2_recived) {
        sigusr2_recived = 0;
        printf("SIGUSR2 Recived \n");
        print_last_prime(last_prime);
    }
    if (sigint_recived) {
        sigint_recived = 0;
        printf("SIGINT Recived, exiting gracefully. \n");
        return 1;
    }
    return 0;
}

/* implementation of a simple primality test
 * + loop checks
 */
static int is_prime(long n, long factored_count, long prime_count, long last_prime) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n%2 == 0) return 0;

    long iter = 0;
    for (long d=3; d<=n/d; d+=2) {
        if (n % d == 0) return 0;

        if (++iter % 1000 == 0) {
            if (sigusr1_recived) {
                sigusr1_recived = 0;
                printf("SIGUSR1 Recived \n");
                print_progress(factored_count, prime_count);
            }
            if (sigusr2_recived) {
                sigusr2_recived = 0;
                printf("SIGUSR2 Recived \n");
                print_last_prime(last_prime);
            }
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {

    struct sigaction sa = {0};

    //SIGUSR1
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    //SIGUSR2
    sa.sa_handler = handle_sigusr2;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR2, &sa, NULL);

    //SIGINT
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    long last_prime=0;
    long prime_count=0;
    long factored_count=0;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s nstart nend\n", argv[0]);
        return EXIT_FAILURE;
    }

    long nstart = parse_l(argv[1]);
    long nend = parse_l(argv[2]);

    if (nstart > nend) {
        fprintf(stderr, "Error: nstart must be <= nend\n");
        return EXIT_FAILURE;
    }
    
    printf("PID: %d\n", getpid());
    printf("Factoring numbers from %ld to %ld\n", nstart, nend);

    int pid;

    for(int i = 0; i<N; i++){
        pid = fork();

        if(pid < 0){
            printf("[Errore grosso]\n");
            return EXIT_FAILURE;
        }
        if(pid == 0){
            //figlio dio can can dio dio can
            for (long n = nstart; n <= nend; n++) {
            int prime = is_prime(n, factored_count, prime_count, last_prime);
            factored_count++;

            if (prime) {
                prime_count++;
                last_prime = n;
            }

            /* just a check to avoid the corner case in which n==LONG_MAX. If this is the 
            * case, the n++ in the for loop overflows n and we might never end the for loop
            */ 
            if (n == LONG_MAX) {
                break;
            }
            if (handle_recived_signals(factored_count, prime_count, last_prime)) {
                break;
            }
        }


            break;
        }
        



        
    }
    
    

    /* this is the main loop, make sure to check for
     * received signals within this loop */
    

    print_final_summary(factored_count,prime_count,last_prime);

    return EXIT_SUCCESS;
}
