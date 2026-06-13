#include <stdio.h>
#include <unistd.h>

void main(){
	while(1){
		printf("I am process %d and i am still running...\n",getpid());
		sleep(1);
    }

}
