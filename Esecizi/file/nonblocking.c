#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <asm-generic/errno-base.h>

#define NCLIENTS 1
#define FIFO_PREFIX "/tmp/file-control-lab-fifo_"

int main(int argc, char *argv[]) {
    int value = 0;    // number to be read from clients
    ssize_t nread;    // number of bytes that read() will returns
    int n = NCLIENTS; // number of clients (and fifos) that the server expects
    int nclosed = 0; // number of fifos that no longer have a writer process
    int fds[NCLIENTS]; //array of file descriptors to read from
    long unsigned int accumulator = 0;
    char fifo_name[256];
	struct pollfd poll_set[NCLIENTS];


    /* This init loop creates and opens all fifos. The server will not enter
     * its main reading loop until all NCLIETNS opened their own fifo in
     * write mode
     */
    for (int i = 0; i < n; i++) {
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);

	// first remove any existing fifo with the same name
        unlink(fifo_name);

	// then create the new fifo
        if (mkfifo(fifo_name, 0666) == -1) {
            perror("mkfifo");
            exit(1);
        }
    }

    /* This other loop opens the fifo after they have all been created.
     * Why not pot everything in the first loop?
     */
    for (int i = 0; i < n; i++) {
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);
        fds[i] = open(fifo_name, O_RDONLY | O_NONBLOCK);
        if (fds[i] == -1) {
            perror("open");
            exit(1);
        }
    }
    /* If we reached this point, we have NCLIENTS writers that already opened
     * the respective fifo in write mode */

	//populating the poll
	for (int i = 0; i < NCLIENTS; i++) {
		poll_set[i].fd = fds[i];       // Il file descriptor della FIFO
		poll_set[i].events = POLLIN;   // Voglio sapere quando ci sono dati da leggere!
		poll_set[i].revents = 0;       // Sarà il kernel a riempire questo campo
	}

    /* This is the main loop. The server performs blocking read() on all the fifo
     * within an inner for loop. It will only stop when there is no writer on none
     * of the fifos.
     */
	while (nclosed < n) {
		int ready = poll(poll_set, n, -1);
		if (ready < 0) {
			perror("poll");
			exit(1);
		}

		for (int i = 0; i < n; i++) {
			if (poll_set[i].fd == -1) {
				continue;
			}

			if (poll_set[i].revents & POLLHUP) {
				close(poll_set[i].fd);
				poll_set[i].fd = -1;
				fds[i] = -1;
				nclosed++;
				continue;
			}

			if (poll_set[i].revents & POLLIN) {
				while ((read(poll_set[i].fd, &value, sizeof(value)))>0){
					accumulator += value;
				}
				if (nread == -1 && errno != EAGAIN) {
					perror("read error");
					exit(1);
				}
			}
		}
	}


    printf("Completed a loop, accumulator = %ld\n", accumulator);


    // This loop unlinks all the fifos
    for (int i = 0; i < n; i++) {
        char fifo_name[256];
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);

        unlink(fifo_name);
    }

    printf("Final accumulator: %ld\n", accumulator);

    return 0;
}
