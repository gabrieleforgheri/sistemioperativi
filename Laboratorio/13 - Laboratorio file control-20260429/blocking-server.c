#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NCLIENTS 1000
#define FIFO_PREFIX "/tmp/file-control-lab-fifo_"

int main(int argc, char *argv[]) {
    int value = 0;    // number to be read from clients
    ssize_t nread;    // number of bytes that read() will returns
    int n = NCLIENTS; // number of clients (and fifos) that the server expects
    int nclosed = 0; // number of fifos that no longer have a writer process
    int fds[NCLIENTS]; //array of file descriptors to read from
    long unsigned int accumulator = 0;
    char fifo_name[256];

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
        fds[i] = open(fifo_name, O_RDONLY);   
        if (fds[i] == -1) {
            perror("open");
            exit(1);
        }
    }
    /* If we reached this point, we have NCLIENTS writers that already opened
     * the respective fifo in write mode */

    
    /* This is the main loop. The server performs blocking read() on all the fifo
     * within an inner for loop. It will only stop when there is no writer on none
     * of the fifos.
     */
    while(nclosed < n) {     

        for(int i=0; i<n; i++) { 

	    /* skip to the next iteration if the current fd
	     * has already been closed
	     */	
	    if (fds[i] == -1)
	    	continue;	    

	    /* This is a blocking read, will be stuck here until the 
	     * ith client writes something on the ith fifo
	     */	
            nread = read(fds[i], &value, sizeof(value));

            if (nread != sizeof(value)) {
		 
		if (nread == 0) {
		    /* the read did not block and returned 0 (EOF)
		     * this means that there is no writer left on 
		     * this fd. 
		     */
		    close(fds[i]);	
		    nclosed += 1;
		    fds[i] = -1;
                    continue; 
		    	
		} else {
		    /* Either some error happened (e.g. the read()
		     * got interrupted and returned -1) or we only
		     * got a partial read (>0 bytes but < sizeof(int) bytes).
		     *
		     * We could use a more robust read function, but for simplicity
		     * we decide to handle that as a fatal error.
		     *
		     * We could also exit cleanly by closing all open
		     * fds and unlinking all fifos...
		     */ 	
                    perror("read");
                    exit(1);
		}
            }

            accumulator += value;

        }
	
        printf("Completed a loop, accumulator = %ld\n", accumulator);
    }

    // This loop unlinks all the fifos
    for (int i = 0; i < n; i++) {
        char fifo_name[256];
        snprintf(fifo_name, sizeof(fifo_name), "%s%d", FIFO_PREFIX, i);

        unlink(fifo_name);
    }

    printf("Final accumulator: %ld\n", accumulator);

    return 0;
}
