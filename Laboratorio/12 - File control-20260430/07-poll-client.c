/* This is the client code. 
 * 
 * Each client reads a number from 0 to NCLIENTS-1 from the command line.
 * This number will tell which fifos the client will use.
 *
 * If the client number is i then it will use:
 * - /tmp/client_i_in to send requests to the server
 * - /tmp/client_i_out to read replies from server
 *
 * Requests contain the pid of the client and a couter that is
 * initialized at 0 and increments for each request.
 *
 * The client waits for a random number of seconds (between 1 and MAXWAIT),
 * then issues a request, waits for the reply and starts again.    
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define NCLIENTS 10
#define MAXWAIT 10

typedef struct{
	pid_t client_pid;
	int counter;
} Message;

int main(int argc, char *argv[]) {
    Message msg;
    char reply[256];
    char fifo_name[64];
    int fd_to_server;
    int fd_from_server;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <client_number>\n", argv[0]);
        exit(1);
    }

    int id = atoi(argv[1]); //I am client i !

    if (id < 0 || id > NCLIENTS-1 ) {
        fprintf(stderr, "Client number must be between 0 and %d\n",NCLIENTS-1);
        exit(1);
    }

    srand(time(NULL) ^ getpid()); //seed the random number generator


    printf("about to open\n");
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_in", id);
    printf("opened to server\n");
    fd_to_server = open(fifo_name, O_WRONLY);
    snprintf(fifo_name, sizeof(fifo_name), "/tmp/client_%d_out", id);
    fd_from_server = open(fifo_name, O_RDONLY);
    printf("opened from server\n");

    if (fd_to_server < 0 || fd_from_server < 0) {
        perror("open fifo");
        exit(1);
    }

    msg.client_pid = getpid();	

    for(int nreq=0; nreq < 10; nreq++){ 
        
	int delay = (rand() % MAXWAIT) + 1;
        sleep(delay);
 
        msg.counter = nreq;       
        write(fd_to_server, &msg, sizeof(msg));

        read(fd_from_server, reply, sizeof(reply));

        printf("Client %d - pid %ld - received: %s\n", id, msg.client_pid, reply);
    }

    close(fd_to_server);
    close(fd_from_server);

    return 0;
}
