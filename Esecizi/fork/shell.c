#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXIN 200
#define MAXPAR 10

int main() {
    char in[MAXIN];

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(in, sizeof(in), stdin) == NULL) {
            putchar('\n');
            break;
        }

        in[strcspn(in, "\n")] = '\0';

        if (in[0] == '\0') {
            continue;
        }

        if (strcmp(in, "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }

        char* argv[MAXPAR];
        int argc = 0;

        char *tok = strtok(in, " \t");
        while (tok != NULL && argc < MAXPAR - 1) {
            argv[argc++] = tok;
            tok = strtok(NULL, " \t");
        }

        argv[argc] = NULL;
        if (argc == 0) continue;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        }
        else if (pid == 0) {
            execve(argv[0], argv, NULL);
            perror("fork");
            _exit(1);
        }
        wait(NULL);
    }
}