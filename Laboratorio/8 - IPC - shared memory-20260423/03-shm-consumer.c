#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

// this is the consumer process, will only read from the shared memory!

int main(){
    /* the size (in bytes) of shared memory object */
    const int SIZE = 4096;
    /* name of the shared memory object */
    const char *name = "SO";
    /* shared memory file descriptor */
    int fd;
    /* pointer to shared memory obect */
    char *ptr;

    /* open the "file" that will back the shared memory object */
    fd = shm_open(name, O_RDONLY, 0666);
    if (fd == -1){
	    perror("shm_open");
	    exit(1);
    }
    
    /* memory map the shared memory object. This is not MAP_ANONYMOUS, so fd has a meaning! */
    ptr = (char *)mmap(NULL, SIZE, PROT_READ, MAP_SHARED, fd, 0);
    /* read from the shared memory object */
    printf("%s",(char *)ptr);
    /* remove the shared memory object */
    shm_unlink(name);
    return 0;
}
