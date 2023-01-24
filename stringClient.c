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
    char buff[MAX];
    int n;
	int len;
	pid_t pid = getpid();
	// randomize myTag
    for (counter = 0; counter < 10; counter++) {
		len = sprintf(buff, "hello world %x counter %d\n", pid, counter);
		
		n = write(sockfd, buff, len);
		if (n != len){
			printf("Attempted to write %d, return %d, errno: %d\n", len, n, errno);
			return;
		};
        n = read(sockfd, buff, MAX-1);
		if (n < 0){
			perror("Error on read.\n");
			return;
		};
		buff[n]=0;
        printf("From Server : [%s], counter: %d\n", buff, counter);
    }
	n = write(sockfd, "EXIT", 4);
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
