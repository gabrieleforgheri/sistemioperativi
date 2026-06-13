#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    char cmd[256];
    char* args[100];

    while (true) {
        printf("yRb4G Shell (solo percorsi assoluti!) > ");

        if (fgets(cmd, sizeof(cmd), stdin) == NULL) {
            break;
        }
        cmd[strcspn(cmd, "\n")] = 0;

        int i = 0;
        args[i] = strtok(cmd, " ");

        for(int j = 0; args[j]; j++){
            args[i][j] = tolower(args[i][j]);
        }

        if (!strcmp(args[i], "exit")) {
            break;
        }

        if (args[i] == NULL) {
            continue;
        }

        while (args[i] != NULL) {
            i++;
            args[i] = strtok(NULL, " ");
        }

        pid_t fork_result = fork();

        if (fork_result < 0) {
            printf("Fork fallita, riprova \n");
            continue;
        }
        if (fork_result == 0) {
            //child
           execv(args[0], args);

            printf("%s: command not found.\n", args[0]);

            _exit(1);
        } else {
            wait(NULL);
        }
    }

    return 0;
}
