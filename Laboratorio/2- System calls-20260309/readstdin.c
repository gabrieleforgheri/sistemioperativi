#include <stdio.h>

void main(){
	
	char something[50];

	printf("Write something on stdin, i will read it!\n");

	fgets(something,sizeof(something),stdin);
	
}
