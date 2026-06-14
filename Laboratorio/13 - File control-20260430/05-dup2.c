#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    int logfd = open("output.log",
                     O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (logfd == -1) { perror("open"); return 1; }

    /* Redirect stdout (fd 1) to the log file */
    if (dup2(logfd, 1) == -1) {
        perror("dup2"); return 1;
    }
    close(logfd);          /* original fd no longer needed */

    printf("This goes to output.log\n");
    return 0;
}
