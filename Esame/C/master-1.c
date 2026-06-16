#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

#define REG_FIFO "/tmp/registration-fifo"


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (n <= 0) {
        fprintf(stderr, "Error: n must be a positive integer\n");
        return 1;
    }

    /*
     * 1- create and open the fifo
     * 2- in a loop for n iterations
     * 	  - read the pid from the fifo
     * 	  - print "Master: registration request received from PID %d\n"
     * 3- close and remove the fifo
     */

    //int fd[2];


    if(mkfifo(REG_FIFO, 0666) == -1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }

    int fd = open(REG_FIFO, O_RDONLY);

    for(int i = 0; i<n; i++){
        pid_t buf;
        size_t n = read(fd, &buf, sizeof(pid_t));
        buf = atoi(buf);

        fflush(stdout);
        printf("Master: registration request recived from PID %ld\n", (long)buf);


        char fifoname[64];
        sprintf(fifoname, "/tmp/worker-%ld-fifo", (long)pid);

        int fdw = open(fifoname, O_WRONLY);

        write(fdw, i, sizeof(int));

        printf("Master: sent index %d to worker %ld\n", i, (long)buf);

        close(fdw);
    }


    int fds = shm_open("/numbers-shm", O_CREAT | O_RDONLY, 0666);
    if(fds == -1){
        perror("shm open");
        exit(1);
    }
    const int size = 4096;

    ftruncate(fds, size);

    char *ptr = (char *)mmap(0, size, PROT_WRITE, MAP_SHARED, fdw, 0);

    



    close(fd);


    return 0;
}
