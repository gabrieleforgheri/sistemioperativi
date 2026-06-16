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

    if(mkfifo(REG_FIFO, 0666) == -1 && errno != EEXIST){
        perror("mkfifo");
        exit(1);
    }

    int fd = open(REG_FIFO, O_RDONLY);

    for(int i = 0; i<n;){
        pid_t buf;
        fflush(stdout);

        ssize_t readb = read(fd, &buf, sizeof(pid_t));

        if (readb == sizeof(pid_t)) {
            printf("Master: registration request received from PID %ld\n", (long)buf);

            //START step 2
            char fifoname[64];

            snprintf(fifoname, sizeof(fifoname), "/tmp/worker-%ld-fifo", (long)buf);
            int fdw = open(fifoname, O_WRONLY);
            if (fdw == -1) {
                perror("fifo open");
                exit(1);
            }
            write(fdw, &i, sizeof(int));

            printf("Master: sent index %d to worker %ld\n", i, (long)buf);
            //END STEP 2
            i++;
        }
        else if (readb == 0) {
            close(fd);
            fd = open(REG_FIFO, O_RDONLY);
            if (fd == -1) {
                perror("FIFO");
                exit(1);
            }
        }
        else {
            perror("read");
            exit(1);
        }

    }

    close(fd);
    unlink(REG_FIFO);

    return 0;
}
