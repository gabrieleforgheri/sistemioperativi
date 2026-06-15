/*
 * Semplice versione con singolo processo e iterativa contenente funzioni di utilità.
 *
 * Si suggerisce di concentrarsi su main(), modificandolo per realizzare gli step previsti dall'esame.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <sys/wait.h>

#define MAX_LINES 30
#define MAX_LINE_LENGTH 256

typedef struct {
    pid_t pid;
    int score;
    char line[MAX_LINE_LENGTH];
} result;

typedef struct {
    int n;
    result results[MAX_LINES];
} shared_data ;

int anomaly_score(const char *line)
{
    int score = 0;

    if (strstr(line, "ERROR") != NULL)
        score += 5;
    if (strstr(line, "FAIL") != NULL)
        score += 3;
    if (strstr(line, "DENIED") != NULL)
        score += 4;
    if (strstr(line, "ROOT") != NULL || strstr(line, "root") != NULL)
        score += 4;
    if (strstr(line, "WARNING") != NULL)
        score += 1;

    return score;
}

struct line_msg {
    int index;
    char line[MAX_LINE_LENGTH];
};

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static int parse_n(const char *s)
{
    char *end = NULL;
    long n = strtol(s, &end, 10);

    if (*s == '\0' || *end != '\0' || n <= 0 || n > MAX_LINES) {
        fprintf(stderr, "N deve essere un intero tra 1 e %d\n", MAX_LINES);
        exit(EXIT_FAILURE);
    }

    return (int)n;
}

static void read_lines(const char *filename, char lines[MAX_LINES][MAX_LINE_LENGTH], int n)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        die("fopen");

    for (int i = 0; i < n; i++) {
        if (fgets(lines[i], MAX_LINE_LENGTH, fp) == NULL) {
            fprintf(stderr, "Il file contiene meno di %d righe\n", n);
            fclose(fp);
            exit(EXIT_FAILURE);
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <file_log> <N>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n = parse_n(argv[2]);
    char lines[MAX_LINES][MAX_LINE_LENGTH];

    read_lines(argv[1], lines, n);


    int fd[2];

    shared_data *shared = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE
        , MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // a questo punto sono già stati letti i parametri da riga di comando e le prime n righe dal file
    // di log. Siamo pronti per iniziare l'elaborazione
    for (int i = 0; i < n; i++) {

	// in questa versione è il singolo processo a invocare anomaly_score e stampare il risultato.
	// modificare questo comportamento per creare le pipe e i figli che calcolano anomaly score (step 1)
	// aggiungere la memoria condivisa (step 2) e il figlio finale di riepilogo (step 3)

        if (pipe(fd) == -1) {
            perror("pipe");
            exit(1);
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            //children chiude la scrittura
            close(fd[1]);

            char buf[MAX_LINE_LENGTH];

            read(fd[0], buf, sizeof(buf));
            close(fd[0]);

            int score = anomaly_score(buf);
            //printf("riga %d: pid=%ld score=%d\n", i, (long)getpid(), score);
            //fflush(stdout);

            shared->results[i].pid = getpid();
            strcpy(shared->results[i].line, buf);
            shared->results[i].score = score;
            shared->n = i;

            exit(score);
        }

        //padre
        close(fd[0]);

        write(fd[1], lines[i], sizeof(lines));



        close(fd[0]);

        wait(NULL);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        //son😭😭😭😭😭😭
        //https://pbs.twimg.com/media/GvOQP2DWIAAbm_R.jpg
        int i = 0, score = 0, max_score = 0;
        int max = shared->results[0].score;
        for (; i<n; i++) {
            printf("riga %d: pid=%ld score=%d\n", i, (long)shared->results[i].pid, shared->results[i].score);
            score += shared->results[i].score;
            if (shared->results[i].score > max) {
                max = shared->results[i].score;
                max_score = i;
            }
        }
        float med = (float)score / (float)n;

        printf("\n");
        printf("righe elaborate: %d\n", i);
        printf("score totale: %d\n", score);
        printf("score medio: %.2f\n", med);
        printf("score massimo: %d\n", max);
        printf("riga con score massimo: %d\n", max_score);

    }
    // for (int i = 0; i<n; i++) {
    //     printf("riga %d: pid=%ld score=%d\n", i, (long)shared->results[i].pid, shared->results[i].score);
    // }
    munmap(shared, sizeof(shared_data));
}
//天�