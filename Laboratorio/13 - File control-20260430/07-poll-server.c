/* This is the server process. The server handles concurrent requests
 * coming from NCLIENTS client processes. 
 *
 * Intended usage: first start the server, then start up to NCLIENTS clients
 *
 * For each client process i (from 0 to NCLIENTS-1) the server creates:
 * - one in_fifo, bound to the file name /tmp/client_i_in , used for 
 *   reding requests that the client will issue to the server. The server
 *   does not know when each client will issue a request.
 * - one out_fifo, bound to the filename /tmp/clinet_1_out , used for
 *   sending replies to the client after having received a request   
 * 
 * Communication protocol:
 * - a request is a struct containing two numbers: 
 *   - the pid of the client
 *   - a counter that starts from zero and is incremented at any request
 * - a reply is the string "Hi client <pid>, this is reply number <counter> !"
 *
 * Server behavior
 * - The server waits for any client to send a request, as soon as the 
 *   request is received, the server will compute and send the reply, then
 *   the server will continue waiting for requests
 */  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define NCLIENTS 10
#define BUF_SIZE 256

typedef struct {
    pid_t client_pid;
    int counter;
} Message;

int main(void) {
    Message msg;
    struct pollfd fds[NCLIENTS]; //array of pollfd structs that we will use to poll()
    int reply_fd[NCLIENTS]; //array of out_fifos that we will use 
				   //for sending replies
    char fifo_name[64];  //will use this to build the name of the fifos 
    char reply[256];     //will be used to build the reply to a client request

    for (int i = 0; i < NCLIENTS; i++) {

	/* This is the initialization loop. Here we create and open all 
	 * the fifos for all the NCLIENTS clients.
	 */    

	/* Here we open all the fifos in RDRW mode, so 
	 * server and cleints lients will  not block when trying to open them.
	 */
        
	snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_in", i );
        mkfifo(fifo_name, 0666);
    
	fds[i].fd = open(fifo_name, O_RDWR);
        if (fds[i].fd < 0) {
            perror("open input fifo");
            exit(1);
        }

        snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_out", i );
        mkfifo(fifo_name, 0666);
        
	reply_fd[i] = open(fifo_name, O_RDWR);
        if (reply_fd[i] < 0 ) {
            perror("open output fifo");
            exit(1);
        }



	/* We set events = POLLIN for this file descriptor. 
	 * When poll() reads this, it will understand that we want to
	 * exit poll() when the file descriptor becomes available for
	 * reading. In a FIFO this happens:
	 * - when someone wrote something to the fifo
	 * - when someone closes the other side of the fifo. In this case
	 *   read() will return 0 (EOF)
	 */
        fds[i].events = POLLIN;
    }

    while (1) {
        printf("Server ready. Waiting for requests from clients...\n");

        int ready = poll(fds, NCLIENTS, -1);

        if (ready < 0) {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < NCLIENTS; i++) {

            if (fds[i].revents & POLLIN) { //check which fd caused the poll to exit
		// it was fd[i] -> client i
	
                ssize_t n = read(fds[i].fd, &msg, sizeof(msg));

                if (n == sizeof(msg)) {
                    printf("Request received: client %ld, counter %d\n", msg.client_pid, msg.counter);


                    if (reply_fd[i] < 0) {
		    	// This only happens the first time that we send a reply 
			// to a given client
                        snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_out", i);
                        reply_fd[i] = open(fifo_name, O_WRONLY);
                    }

                    snprintf(reply, sizeof(reply),
                             "Hi client %ld, this is reply number %d !\n", msg.client_pid, msg.counter);

                    write(reply_fd[i], reply, strlen(reply) + 1);
                } else {
			printf("don't know what happened...\n");
		}
            }
        }
    }

    /* Note: you will never reach this point with the current code.
     * You could probably register an handler for the sigint signal that
     * breaks out of the main server loop, or come up with some other 
     * idea (e.g. service at most N requests and break, service N requests
     * for each client and break, break when a client sends a given message...
     *
     * However, below there is an example of how you could exit cleanly
     */	

    printf("Server exiting cleanly.\n"); 

    for (int i = 0; i < NCLIENTS; i++) {
        close(fds[i].fd);
        if (reply_fd[i] >= 0) close(reply_fd[i]);

        snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_in", i);
        unlink(fifo_name);
        snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_out", i);
        unlink(fifo_name);
    }

    return 0;
}
