#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main() {

    int *shared = mmap(NULL, 	//memory address. Let the kernel decide it! 
		  sizeof(int), 	//size of the shared memory area, in bytes
                  PROT_READ | PROT_WRITE,	//what can I do with the shared memory
                  MAP_SHARED | MAP_ANONYMOUS,	//this memory is shared and anonymous
                  -1, 				// file descriptor of a backing file. Ignored in anonymous mapping
		  0);				// offset within the file. Ignored in anonymous mapping

    *shared = 0;

    if (fork() == 0) {
        *shared = 42;
        printf("child wrote %d\n", *shared);
	_exit(0);
    } else {
        wait(NULL);
        printf("parent reads %d\n", *shared);
    }

    munmap(shared,sizeof(int)); //this unmaps the shared memory. It is also automatically unmapped on process exit
    exit(0);
}
