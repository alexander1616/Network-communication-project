#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <errno.h>
#include "../dumpbuffer/dumpbuffer.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr



void func(int sockfd)
{
	int counter = 0;
	uint32_t myTag = 1000;
	uint32_t outTag;
	uint32_t inTag;
    char buff[MAX];
    int n;
	// randomize myTag
	pid_t pid = getpid();
	unsigned int seed;
	seed=pid;
	srand(seed);
	int terminating = (rand()%10000) + 1;
	myTag = terminating;	
    for (counter = 0; counter < 10; counter++) {
		//outTag = htonl(myTag);
		outTag = myTag;
		n = write(sockfd, &outTag, sizeof(outTag));
		if (n != sizeof(outTag)){
			printf("Attempted to write %lu, return %d, errno: %d\n", sizeof(outTag), n, errno);
		return;
		};
        n = read(sockfd, &inTag, sizeof(inTag));
		if (n != sizeof(inTag)){
			printf("Attempted to read %lu, return %d, errno: %d\n", sizeof(inTag), n, errno);
		return;
		};
		uint32_t temp;
		//temp = ntohl(inTag);
		temp = inTag;
		
        printf("From Server : %d, %x, counter: %d\n", inTag, inTag, counter);
        printf("Converted Value : %d, %x\n", temp, temp);
		myTag = temp - 1;
    }
	outTag = terminating;
	n = write(sockfd, &outTag, sizeof(outTag));
}
 
int main()
{
    int sockfd;
    struct sockaddr_in servaddr;
	int pauseTime = 5; 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
	printf("Port = %d %x\n", PORT, PORT);
	printf("sin_port = %d %x\n", servaddr.sin_port, servaddr.sin_port);
	
    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr))
        != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    // function for chat
	printf("Sleeping for %ds after connection\n", pauseTime);
	sleep(pauseTime);
	 func(sockfd);
 
    // close the socket
    close(sockfd);
}
