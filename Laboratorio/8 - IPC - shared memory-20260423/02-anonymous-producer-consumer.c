#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>

struct shared_data {
    int ready;   // 0 = already consumed, 1 = not consumed yet
    int value;   // value to share
};

int main() {
    struct shared_data *shm = mmap(NULL, sizeof(struct shared_data),
                                   PROT_READ | PROT_WRITE,
                                   MAP_SHARED | MAP_ANONYMOUS,
                                   -1, 0);

    if (shm == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    shm->ready = 0;
    shm->value = 0;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process. Let it be the consumer
        while(1) {  //infinite loop
            while (shm->ready == 0){   
				sleep(1); //if there is nothing to consume, sleep 1 second and try again
	        };
	        //there is something to consume!
            printf("child consumed: %d\n", shm->value);
            shm->ready = 0;   // mark slot as empty
	    
			if(shm->value == 10){
                munmap(shm,sizeof(struct shared_data));
				_exit(0);
			}
		}
    }
    
    // Parent = producer
    for (int i = 1; i <= 10; i++) {
        while (shm->ready == 1)	{
		sleep(1); //if the shared value has not be consumed yet, sleep 1 second
		}

        shm->value = i;
        printf("parent produced: %d\n", shm->value);
		shm->ready = 1;   // mark slot as full
    }

    wait(NULL);
    munmap(shm, sizeof(struct shared_data));
    
    return 0;
}
