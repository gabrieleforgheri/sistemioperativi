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

    size_t n;

    char fifoname[64];
    sprintf(fifoname, "/tmp/worker-%ld-fifo", (long)pid);

    int fd = open(REG_FIFO, O_WRONLY);

    write(fd, &pid, sizeof(pid_t));

    close(fd);

    if(mkfifo(fifoname, 0666)  == -1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }
    
    int fd = open(fifoname, O_WRONLY);

    int indice;
    size_t n = read(fd, &indice, sizeof(indice));
    printf("Worker %ld - index %d\n", pid, indice);

    close(fd);


    return 0;
}
