#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>

#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define REG_FIFO "/tmp/registration-fifo"

volatile sig_atomic_t sigusr1_recived = 0;

void handle_sigusr1(int sig) {
    (void)sig;
    sigusr1_recived = 1;
}

int is_prime(int x) {
    if (x < 2)
        return 0;

    if (x == 2)
        return 1;

    if (x % 2 == 0)
        return 0;

    for (int d = 3; d * d <= x; d += 2) {
        if (x % d == 0)
            return 0;
    }

    return 1;
}

int main(void) {

    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

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
    //printf("Worker %ld - index %d\n", (long)pid, buf);
    //END step 2

    //START STEP3
    while (!sigusr1_recived) {
        pause();
    }
    sigusr1_recived = 0;
    char* nomeshm = "/numbers-shm";

    int fds = shm_open(nomeshm, O_RDONLY, 0666);
    if (fds == -1) {
        perror("shm");
        exit(1);
    }
    if (lseek(fds, buf * sizeof(int), SEEK_SET) == -1) {
        perror("lseek");
        exit(1);
    }
    int num;
    if (read(fds, &num, sizeof(num)) != sizeof(int)) {
        perror("read shm");
        exit(1);
    }
    close(fds);
    if (is_prime(num)) {
        printf("%d is prime\n", num);
    }
    else {
        printf("%d is not prime\n", num);
    }

    return 0;
}
