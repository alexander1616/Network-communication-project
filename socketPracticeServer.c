#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>
#include <unistd.h> // read(), write(), close()
#include <errno.h>
#include "../dumpbuffer/dumpbuffer.h"
#define MAX 80
#define PORT 8080
#define SA struct sockaddr

int doAccept(int fd){
    int connfd, len;
    struct sockaddr_in servaddr, cli;
	len = sizeof(cli);
	printf("Going into accept, current connection: \n");
	connfd = accept(fd, (SA*)&cli, &len);
    if (connfd < 0) {
    	printf("server accept failed... connfd: %d\n", connfd);
    	exit(0);
    };
    printf("server accept the client...connfd: %d\n", connfd);
	return connfd;
};  

uint32_t firstseq[1024] = {0};

int newRead(int connfd){
	char buff[MAX];
	int n;
	read(connfd, buff, sizeof(buff));
	printf("From Client: %s\n", buff);
	return 0;
};

int newWrite(int connfd){
	char buff[MAX];
	int n;
	bzero(buff, sizeof(buff));
	n = 0;
	printf("Please enter string for client: \n");
	while ((buff[n++] = getchar()) !='\n')
		;
	write(connfd, buff, n);
	return 0;
};

int doRead(int connfd) //return 0 when reach terminating sequence, return 1 when back to client
{
	int counter = 0;
	uint32_t myTag;
    uint32_t outTag;
    uint32_t inTag;
    char buff[MAX];
    int n;
    
    // read the message from client and copy it in buffer
    n = read(connfd, &inTag, sizeof(inTag));
	if (n != sizeof(inTag)){
        printf("Attempted to read %lu, return %d, errno: %d\n", sizeof(inTag), n, errno);    
    	return -1; 
    };

	uint32_t temp;
	//temp = ntohl(inTag);
	temp = inTag;
    // print buffer which contains the client contents
    printf("From client: %d, %x counter: %d\n", inTag, inTag, counter);
    printf("Converted value: %d, %x\n", temp, temp);
	if(temp == firstseq[connfd]){
		printf("Received a terminate signal from client. \n");
		firstseq[connfd] = 0;
		return 0;
	};

	if (firstseq[connfd] == 0){
		firstseq[connfd] = temp;
	};	
	
    // copy server message in the buffer
	myTag = temp + 2;
	//outTag = htonl(myTag);
	outTag = myTag;
    n = write(connfd, &outTag, sizeof(outTag));
	if (n != sizeof(outTag)){
        printf("Attempted to write %lu, return %d, errno: %d\n", sizeof(outTag), n, errno);
    	return -1;
    };
	return 1;
}

struct x{
	fd_set t;
	int max;
};

void makeSelect(Container_t* p, struct x* yyy){
	struct x dad;
	yyy = &dad;	
	int maxFd = p->fd;
	yyy->max = maxFd;
};
   
// Driver function
int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;
   
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Socket bind failed...(%d)\n", errno);
		perror("Bind fail");
        exit(0);
    }
    else
        printf("Socket successfully binded to port:[%d]\n", PORT);
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

    // Accept the data packet from client and verification
	int maxFd; //current maximum fd detected
    fd_set allfds;
	FD_ZERO(&allfds);	
	FD_SET(sockfd, &allfds);
	maxFd = sockfd + 1;	
    struct timeval tv;
    int retval;

    while(1){
		fd_set tempfds;
        tv.tv_sec = 60;
        tv.tv_usec = 0;
		tempfds = allfds;
		struct x yyy;
		makeSelect(p, &yyy);
		printf("\n\nGoing into select for connection: \n");
		retval = select(maxFd, &tempfds, NULL, NULL, &tv);
		if (retval < 0){
			printf("Failed to Select. \n");
			perror("alex");
		} else if (retval == 0){
			printf("We got a timeout. \n");
		} else {
			if (FD_ISSET(sockfd, &tempfds)){
				printf("We got a read condition on sockfd.\n");
				connfd=doAccept(sockfd);
				FD_SET(connfd, &allfds);
				if (connfd >= maxFd){
					maxFd = connfd + 1;
				};
				retval--;
			};
			while (retval){
				for (int i=3; i<maxFd; i++){
					if (FD_ISSET(i, &tempfds)){
						int n = doRead(i);
						printf("Attempting to write on fd: %d\n", i);
						if(n < 0){
							printf("There is an issue on read. Current FD: %d\n", i);
							exit(0);
						} else if (n == 0){
							printf("We reached terminating sequence for Current FD: %d\n", i);
							close(i);
							FD_CLR(i, &allfds);
						};	
						retval--;
					};
				};
			};
		};
	}; 
    // After chatting close the socket
    close(sockfd);
}
