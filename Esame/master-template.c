#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

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

    return 0;
}
