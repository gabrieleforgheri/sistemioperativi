#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/stat.h>

#define REG_FIFO "/tmp/registration-fifo"

int main(void) {

    char nomefifo[64];
    pid_t pid = getpid();

    snprintf(nomefifo, sizeof(nomefifo), "/tmp/worker-%ld-fifo", (long)pid);

    if (mkfifo(nomefifo, 0666) == -1) {
        perror("fifo creation");
        exit(1);
    }
    /*
     * 1- get your own pid
     * 2- open the fifo
     * 3- write the pid to the fifo
     * 4- close the fifo
     */


    int fd = open(REG_FIFO, O_WRONLY);

    if (write(fd, &pid, sizeof(pid_t)) == -1) {
        perror("write");
        exit(1);
    }

    close(fd);

    //START step 2

    int fdm = open(nomefifo, O_RDONLY, 0666);
    if (fdm == -1) {
        perror("open fifo");
        exit(1);
    }

    int buf;

    while (read(fdm, &buf, sizeof(int)) != sizeof(int)) {
        sleep(1);
    }
    printf("Worker %ld - index %d\n", (long)pid, buf);
    //END step 2

    return 0;
}
