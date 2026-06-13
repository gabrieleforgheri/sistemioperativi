#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define REAL_DATA_SIZE (100 * 1024 * 1024)   // 100 MB
#define FILE_SIZE      (1024LL * 1024 * 1024) // 1 GB

int main() {
    /* create a file and open it in write mode */
    int fd = open("sparse_file.bin", O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* use lseek to move forward in the file. 
     * Since the file is empty, this creates a "hole" in the
     * file.
     */ 
    off_t offset = lseek(fd, FILE_SIZE - REAL_DATA_SIZE, SEEK_SET);
    if (offset == (off_t)-1) {
        perror("lseek");
        close(fd);
        return 1;
    }

    /* Allocate a buffer and fill it with some data
     * This data will be rally written in the file
     */
    char *buffer = malloc(REAL_DATA_SIZE);
    if (!buffer) {
        perror("malloc");
        close(fd);
        return 1;
    }
    //fill the buffer with "A"
    memset(buffer, 'A', REAL_DATA_SIZE);

    /* Write the buffer at the end of the file,
     * right after the "hole" created by lseek()
     */
    if (write(fd, buffer, REAL_DATA_SIZE) != REAL_DATA_SIZE) {
        perror("write");
        free(buffer);
        close(fd);
        return 1;
    }

    free(buffer);
    
    close(fd);

    printf("Sparse file created: 1GB logical size, 100MB actual disk usage.\n");
    return 0;
}
