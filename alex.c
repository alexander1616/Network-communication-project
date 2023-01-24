#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int processOpen(char *filename){
	printf("About to open file: %s\n", filename);
	int fd = open(filename, O_WRONLY);
	if (fd < 0){
		printf("Failed to open file %s, errno:%d\n", filename, errno);
		switch (errno){
			case ENOENT:
				printf("File does not exist.\n");
				break; 
			case EACCES: 
				printf("No permission open for write.\n");
				break;
			default:
				printf("You break this! Errno: %d\n", errno);
				break;
		};
		return -1;
	};
	printf("File %s opened. Fd: %d \n", filename, fd);
	printf("I am going to sleep for 10 seconds \n");
	sleep(10);
	//write(fd, buf, 2);
	return 0;
}

int main(int ac, char* av[]){

	int numParam = ac - 1;
	printf("num param = %d\n", numParam);
	
	char *filename;
	for (int i=1; i<ac; i++){
		filename = av[i];
		processOpen(filename);
	};

	return 0;
}
