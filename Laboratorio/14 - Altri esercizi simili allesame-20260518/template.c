#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFSIZE 1024

void process_data(char *str) {
    /* Esempio: inversione della stringa */
    size_t len = strlen(str);

    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
        len--;
    }

    for (size_t i = 0; i < len / 2; i++) {
        char tmp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = tmp;
    }
}

int main(void) {
    int pipefd[2];
    pid_t pid;

    /*
     * TODO:
     * 1. Creare una pipe anonima con pipe()
     * 2. Creare un figlio con fork()
     * 3. Nel padre:
     *    - leggere una stringa da stdin
     *    - inviarla al figlio tramite pipe    
     *    - attendere la terminazione del figlio
     * 4. Nel figlio:
     *    - leggere la stringa dalla pipe
     *    - chiamare process_data()
     *    - stampare il risultato
     */

    return 0;
}
