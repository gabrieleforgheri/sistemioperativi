#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

#define NUM_CHILDREN 10

// Array globale per i PID dei figli (necessario per l'handler del padre)
pid_t child_pids[NUM_CHILDREN] = {0};

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

// --- LOGICA DEL PADRE ---

// Handler del padre: inoltra il segnale ricevuto a tutti i figli vivi
void parent_handler(int sig) {
    for (int i = 0; i < NUM_CHILDREN; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], sig);
        }
    }
}

// --- LOGICA DEI FIGLI (Esercizio 3 adattato) ---

static volatile sig_atomic_t sigusr1_recived = 0;
static volatile sig_atomic_t sigusr2_recived = 0;
static volatile sig_atomic_t sigint_recived = 0;

void child_handler(int sig) {
    if (sig == SIGUSR1) sigusr1_recived = 1;
    if (sig == SIGUSR2) sigusr2_recived = 1;
    if (sig == SIGINT)  sigint_recived = 1;
}

// (Inserisci qui le tue funzioni print_progress, print_last_prime, print_final_summary)

static int is_prime(long n, long fact, long pr, long last) {
    if (n < 2) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    long iter = 0;
    for (long d = 3; d <= n/d; d += 2) {
        if (n % d == 0) return 0;
        if (++iter % 1000 == 0) {
            if (sigusr1_recived) { sigusr1_recived = 0; print_progress(fact, pr); }
            if (sigusr2_recived) { sigusr2_recived = 0; print_last_prime(last); }
        }
    }
    return 1;
}

void run_child_logic(long start, long end, int child_id) {
    // 1. Setup segnali specifico per il figlio
    struct sigaction sa = {0};
    sa.sa_handler = child_handler;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    long last_p = 0, p_count = 0, f_count = 0;

    for (long n = start; n <= end; n++) {
        if (is_prime(n, f_count, p_count, last_p)) {
            p_count++;
            last_p = n;
        }
        f_count++;

        // Gestione segnali post-test
        if (sigusr1_recived) { sigusr1_recived = 0; print_progress(f_count, p_count); }
        if (sigusr2_recived) { sigusr2_recived = 0; print_last_prime(last_p); }
        if (sigint_recived) {
            printf("[Child %d] SIGINT received, exiting...\n", child_id);
            break;
        }
        if (n == LONG_MAX) break;
    }
    print_final_summary(f_count, p_count, last_p);
    exit(EXIT_SUCCESS);
}

// --- MAIN ---

int main(int argc, char *argv[]) {
    if (argc != 3) return EXIT_FAILURE;
    long nstart = atol(argv[1]);
    long nend = atol(argv[2]);

    // 1. Setup segnali del PADRE
    struct sigaction sa_p = {0};
    sa_p.sa_handler = parent_handler;
    sigaction(SIGUSR1, &sa_p, NULL);
    sigaction(SIGUSR2, &sa_p, NULL);
    sigaction(SIGINT, &sa_p, NULL);

    // 2. Calcolo dei range
    long total_range = nend - nstart + 1;
    long step = total_range / NUM_CHILDREN;

    for (int i = 0; i < NUM_CHILDREN; i++) {
        long c_start = nstart + (i * step);
        long c_end = (i == NUM_CHILDREN - 1) ? nend : (c_start + step - 1);

        pid_t pid = fork();
        if (pid == 0) {
            run_child_logic(c_start, c_end, i);
        } else {
            child_pids[i] = pid;
        }
    }

    // 3. Il padre aspetta i figli
    int status;
    pid_t exited_pid;
    while ((exited_pid = wait(&status)) > 0) {
        // Opzionale: segna il figlio come morto nell'array
        for (int i = 0; i < NUM_CHILDREN; i++) {
            if (child_pids[i] == exited_pid) child_pids[i] = 0;
        }
    }

    printf("Tutti i figli hanno terminato. Padre esce.\n");
    return 0;
}