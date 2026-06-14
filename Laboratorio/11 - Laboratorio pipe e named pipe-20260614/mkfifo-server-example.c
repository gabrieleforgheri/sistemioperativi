#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_C1_TO_C2 "/tmp/fifo_c1_to_c2" //communication from client1 to client2
#define FIFO_C2_TO_C1 "/tmp/fifo_c2_to_c1" //communication from client2 to client1
#define FIFO_C1_TO_S  "/tmp/fifo_c1_to_server" //communication from client1 to server
#define FIFO_C2_TO_S  "/tmp/fifo_c2_to_server" //communication from client2 to server

#define MAX_TEXT 128

typedef struct {
    pid_t pid;
    char text[MAX_TEXT];
} Message;

int create_and_open_fifo_or_die(const char *fifo_name) {
    /* create one FIFO with the given name
     * note: if the FIFO already exists, mkfifo() fails with EEXIST.
     * in that case, we simply reuse the existing FIFO.
     */
    if (mkfifo(fifo_name, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(1);
        }
    }
    
    /* open the FIFOs on which to wait for clients	
     * NOTE: FIFOs are opened with the O_RDRW flag
     * This *DOES NOT* make them bidirectional!
     * A fifo (as a pipe) is unidirectional! 
     * One process writes to the fifo, another
     * process reads from the fifo.
     *
     * The "server" process opens all FIFOs in RDWR mode
     * to avoid other processes that only open them in RD or 
     * RW mode to blok on open
     */

    int fd = open(fifo_name, O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    return fd;
}
	

int main(void) {
    Message msg1;
    Message msg2;
    int read_bytes;

    printf("=== FIFO communication example ===\n");
    printf("The server will create all FIFOs, then wait for clients.\n");
    printf("The clients will exchange one message with each other,\n");
    printf("then each client will send one final message to the server.\n\n");

    // create all FIFOs needed for communication
    int fifo_c1_to_c2_fd = create_and_open_fifo_or_die(FIFO_C1_TO_C2);
    int fifo_c2_to_c1_fd = create_and_open_fifo_or_die(FIFO_C2_TO_C1);
    int fifo_c1_to_s_fd = create_and_open_fifo_or_die(FIFO_C1_TO_S);
    int fifo_c2_to_s_fd = create_and_open_fifo_or_die(FIFO_C2_TO_S);

    // wait for the message of client1
    read_bytes = read(fifo_c1_to_s_fd, &msg1, sizeof(msg1));
    if (read_bytes != sizeof(msg1)) {
        fprintf(stderr, "Failed to read a complete message from %s\n", FIFO_C1_TO_S);
        close(fifo_c1_to_s_fd);
        exit(1);
    }
    printf("\nServer received final message from client1\n");
    printf("  sender pid: %d\n", msg1.pid);
    printf("  text: %s\n", msg1.text);

    // wait for the message of client2
    read_bytes = read(fifo_c2_to_s_fd, &msg2, sizeof(msg2));
    if (read_bytes != sizeof(msg2)) {
        fprintf(stderr, "Failed to read a complete message from %s\n", FIFO_C2_TO_S);
        close(fifo_c2_to_s_fd);
        exit(1);
    }
    printf("\nServer received final message from client2\n");
    printf("  sender pid: %d\n", msg2.pid);
    printf("  text: %s\n", msg2.text);

    printf("\nServer: both clients completed their work\n");
    printf("Server exiting cleanly...\n");

    close(fifo_c1_to_c2_fd);
    close(fifo_c2_to_c1_fd);
    close(fifo_c1_to_s_fd);
    close(fifo_c2_to_s_fd);

    unlink(FIFO_C1_TO_C2);
    unlink(FIFO_C2_TO_C1);
    unlink(FIFO_C1_TO_S);
    unlink(FIFO_C2_TO_S);

    return 0;
}
