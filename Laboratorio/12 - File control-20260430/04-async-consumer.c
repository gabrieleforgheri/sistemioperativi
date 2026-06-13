#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#define FIFO_PATH "/tmp/myfifo"

static volatile sig_atomic_t got_sigio = 0;

/*
static void drain_fifo(void)
{
    char buf[256];

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf) - 1);

        if (n > 0) {
            buf[n] = '\0';
            printf("Received: %s", buf);
        } else if (n == 0) {
            // Writer closed the FIFO.
            // For a FIFO opened read-only, future writers may still arrive.
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            perror("read");
            break;
        }
    }
}
*/


static void sigio_handler(int signo)
{
    got_sigio = 1;
}

int main(void)
{
    int fd = -1;
    ssize_t n;	
    char buffer[256];

    /* create the fifo (reuse the existing one if already exists
     */
    if (mkfifo(FIFO_PATH, 0666) < 0 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    /* open the fifo and make it non-blocking
     */
    fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    /* install teh handler for handling the SIGIO signal
     * this signal is sent to a process when an async file
     * is ready for I/O operations
     */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGIO, &sa, NULL) < 0) {
        perror("sigaction");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* F_SETOWN does *not* change the user thet owns a file!
     * it associates a process to a file descriptor, so that
     * if the file descryptor is set as O_ASYNC, and it becomes
     * ready for I/O, this is the process that will receive the 
     * signal
     */
    if (fcntl(fd, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* we now use fcntl to read the current flags... */ 
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*... and add the O_NONBLOCK and O_ASYNC ffals to the
     * existing ones
     */ 
    if (fcntl(fd, F_SETFL, flags | O_ASYNC ) < 0) {
        perror("fcntl F_SETFL");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Reader ready.\n");

    for (int i=0; i<30;i++) {
        printf("[Consumer] Pretending to do some useful work...\n");
        sleep(1);

        if (got_sigio) {
            got_sigio = 0;
            n = read(fd,buffer,sizeof(buffer));
	    if (n > 0){
	        buffer[n] = '\0';
	        printf("[Consumer] read %d bytes\n%s\n",n,buffer);
	    } 
        }
    }
    printf("[Consumer] Exiting cleanly\n");
    close(fd);
    unlink(FIFO_PATH);
    return 0;
}
