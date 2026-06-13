#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATH "/tmp/myfifo"

int main(void) {
    char buffer[256];
    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    
    for(int i=1; i<6; i++){
        /* should use a more robust write_full function
        * or at least check the returned value for error conditions...
        * but you already know how to do that...
        */
	snprintf(buffer,sizeof(buffer),"Message number %d\n\0",i);

        write(fd, buffer, strlen(buffer));
	sleep(5);
    }

    close(fd);
    return 0;
}
