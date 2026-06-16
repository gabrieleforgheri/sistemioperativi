#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>

#include <errno.h>

#define REG_FIFO "/tmp/registration-fifo"

int main(void) {

    /*
     * 1- get your own pid
     * 2- open the fifo
     * 3- write the pid to the fifo
     * 4- close the fifo
     */	

    pid_t pid = getpid();


    int fd = open(REG_FIFO, O_WRONLY);

    if (write(fd, &pid, sizeof(pid_t)) == -1) {
        perror("write");
        exit(1);
    }

    close(fd);

    return 0;
}
