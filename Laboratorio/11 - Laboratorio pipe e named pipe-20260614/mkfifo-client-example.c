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

int main(int argc, char* argv[]) {

    int from_other_client_fd;
    int to_other_client_fd;
    int to_server_fd;
    int myself; // 1 or 2
    int other; // 1 or 2
    int read_bytes, write_bytes;
    Message msg_to_send;
    Message msg_to_receive;
    char message_content[MAX_TEXT];

    /* I am a client process.
     * If argv[1] == "1" I will behave as client1. 
     * If argv[1] == "2" I will behave as client2.
     */
 
    if (argc != 2 || ( strcmp(argv[1],"1") != 0 && strcmp(argv[1],"2") != 0 ) ){
	    printf("Usage: %s <n>, where <n> can be 1 or 2\n",argv[0]);
	    exit(1);
    }

    if (!strcmp(argv[1],"1")){
	//I am client1!
        myself = 1;
	other = 2;

        //open the FIFOs on which to read from the other client       
	//NOTE: if we do not open the fifo in read mode, any other
	//process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C2_TO_C1, O_RDONLY);

        //open the FIFOs on which to write to the other client       
        to_other_client_fd = open(FIFO_C1_TO_C2, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C1_TO_S, O_WRONLY);

    } else {
	//I am client2!
	myself = 2;
	other = 1;
        
        //open the FIFOs on which to read from the other client       
	//NOTE: if we do not open the fifo in read mode, any other
	//process trying to open it in write mode will block!
        from_other_client_fd = open(FIFO_C1_TO_C2, O_RDONLY);

	//open the FIFOs on which to write to the other client       
        to_other_client_fd = open(FIFO_C2_TO_C1, O_WRONLY);

        //open the FIFOs on which to write to the server       
        to_server_fd = open(FIFO_C2_TO_S, O_WRONLY);
    }

    printf("I am Client%d, my pid is %d\n", myself, getpid());
    fflush(NULL);
    if (from_other_client_fd == -1 || to_other_client_fd == -1 || to_server_fd == -1) {
        perror("open");
        exit(1);
    }

    printf("I am Client%d, my pid is %d\n", myself, getpid());
    fflush(NULL);

    // create the message to send to the other client
    msg_to_send.pid = getpid();
    sprintf(message_content,"Hello from client%d to client%d", myself, other);
    strncpy(msg_to_send.text, message_content, MAX_TEXT - 1);
    msg_to_send.text[MAX_TEXT - 1] = '\0';

    // send the message to the other client
    printf("Client%d is sending message %s to client%d\n", myself, msg_to_send.text, other); 
    write_bytes = write(to_other_client_fd, &msg_to_send, sizeof(msg_to_send));
    if (write_bytes != sizeof(msg_to_send)) {
        perror("Could not write a complete message:");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }

    printf("Client%d sent the message to client%d\n", myself, other); 

    // read the message from the other client
    printf("Client%d is reading a message from client%d\n", myself, other); 
    read_bytes = read(from_other_client_fd, &msg_to_receive, sizeof(msg_to_receive));
    if (read_bytes != sizeof(msg_to_receive)) {
        perror("Could not read a complete message:");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }
    printf("Client%d received message %s from client%d\n", myself, msg_to_receive.text, other); 

    // now both clients send a message to the server
    msg_to_send.pid = getpid();
    sprintf(message_content,"Client%d finished!\n", myself);
    strncpy(msg_to_send.text, message_content, MAX_TEXT - 1);
    msg_to_send.text[MAX_TEXT - 1] = '\0';

    printf("Client%d is sending message %s to server\n", myself, msg_to_send.text);
    write_bytes = write(to_server_fd, &msg_to_send, sizeof(msg_to_send));
    if (write_bytes != sizeof(msg_to_send)) {
        perror("Could not write a complete message:");
        close(to_other_client_fd);
        close(from_other_client_fd);
        close(to_server_fd);
        exit(1);
    }

    printf("Client%d sent its final message to the server\n",myself);
    return(0);
}
