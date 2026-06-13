#include <stdio.h>

void main(){
	
	char something[50];
	char *filename = "/tmp/SO_test";

	FILE *SO_test;

	printf("Write something on stdin, i will read it and write it in a file named /tmp/SO_test!\n");

	fgets(something,sizeof(something),stdin);

	SO_test = fopen(filename,"w+");

	fprintf(SO_test,"%s",something);

	fclose(SO_test);
	
}
