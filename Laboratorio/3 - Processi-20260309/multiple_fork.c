#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main() {

    printf("All begins vith process %d, child of bash process %d\n",getpid(),getppid());		
    
    fork();  // 1st fork
    fork();  // 2nd fork
    fork();  // 3rd fork

    printf("PID=%d, PPID=%d\n", getpid(), getppid());

    return 0;
}
