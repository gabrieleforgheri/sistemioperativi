#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXLINE 1024
#define MAXARGS 64

int main(void) {
    char line[MAXLINE];

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            // EOF (Ctrl-D) or error
            putchar('\n');
            break;
        }

        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';

        // Empty line -> new prompt
        if (line[0] == '\0') continue;

        // Minimal builtin
        if (strcmp(line, "exit") == 0) break;

        // Tokenize by spaces/tabs
        char *argv[MAXARGS];
        int argc = 0;

        char *tok = strtok(line, " \t");
        while (tok != NULL && argc < MAXARGS - 1) {
            argv[argc++] = tok;
            tok = strtok(NULL, " \t");
        }
        argv[argc] = NULL;
        if (argc == 0) continue;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // Child: execve with NO environment
            execve(argv[0], argv, NULL);
            perror("execve");
            _exit(1);
        }

        // Parent: wait for child
        wait(NULL);
    }

    return 0;
}
