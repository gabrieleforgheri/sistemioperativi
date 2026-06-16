#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>

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

    //3
    char *nameshm = "/numbers-shm";
    int fds = shm_open(nameshm, O_CREAT | O_RDWR, 0666);
    if (fds == -1) {
        perror("shared mem");
        exit(1);
    }

    if (ftruncate(fds, n * sizeof(int)) == -1) {
        perror("ftruncate");
        exit(1);
    }

    int *numbers = mmap(NULL, n * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fds, 0);
    if (numbers == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    FILE *fp = fopen("numbers.txt", "r");
    if (fp == NULL) {
        perror("numbers.txt");
        //free(numbers);
        return 1;
    }

    for (int i = 0; i < n; i++) {
        if (fscanf(fp, "%d", &numbers[i]) != 1) {
            fprintf(stderr, "Error: numbers.txt contains fewer than %d integers\n", n);
            fclose(fp);
            //free(numbers);
            return 1;
        }
    }

    // END 3

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

            //START STEP 3
            kill(buf, SIGUSR1);
            printf("Master: sent SIGUSR1 to worker %ld\n", (long)buf);


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


    sleep(1);

    close(fd);
    unlink(REG_FIFO);
    shm_unlink(nameshm);

    return 0;
}
