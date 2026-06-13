#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t fork_result = fork();

    if (fork_result < 0) {
        perror("fork failed");
        return 1;
    } 
    else if (fork_result == 0) {
        // Child process
        printf("Child: doing some work...\n");
        sleep(1);

        int result = 42;
        printf("Child: exiting with value %d\n", result);

        exit(result);  // send value to parent
    } 
    else {
        // Parent process
        int status;

        printf("Parent: will now suspend until child terminates...\n");
        waitpid(fork_result, &status, 0);
        printf("Child terminated! let us see how it terminated...\n");
	
	
	// WIFEXITED and WEXITSTATUS are macros that simplify the decoding of
	// "status". We will understand exactly how they work after having discussed
	// signals. For now, just take them as black magic
        if (WIFEXITED(status)) {
            int child_status = WEXITSTATUS(status);
            printf("Parent: child returned %d\n", child_status);
        } else {
	    // instead of terminating normally, the child process has been 
	    // terminated before returning a value 
            printf("Parent: child did not exit normally\n");
        }
    }

    return 0;
}
