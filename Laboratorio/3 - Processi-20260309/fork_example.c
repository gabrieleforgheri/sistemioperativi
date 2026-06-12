#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t fork_result;
    pid_t parent_pid;
    pid_t my_pid;


    fork_result = fork();  // Create a new process

    if (fork_result < 0) {
        // Fork failed
        perror("fork failed");
        return 1;
    } else if (fork_result == 0) {
        // Child process
        printf("Hello from the child process!\n");
	parent_pid = getppid();
	my_pid = getpid();
    } else {
        // Parent process
        printf("Hello from the parent process!\n");
	parent_pid = getppid();
	my_pid = getpid();
    }

    printf("My PID: %d, Parent PID: %d Fork result: %d\n", my_pid, parent_pid, fork_result);

    return 0;
}
