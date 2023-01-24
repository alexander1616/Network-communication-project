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
#include "../container/container.h"
#define MAX 80
#define SA struct sockaddr

typedef struct{
	int fd;
	int terminating;
	int readcount;
} FDElement_t;

void printElement(FDElement_t* p){
	if (p == 0){
		printf("==========\n");
	} else {
		printf("FD:[%d], Terminating:[%d], Readcount:[%d]\n", p->fd, p->terminating, p->readcount);
	}
};

int findFD(FDElement_t* p, int value){
	int match;
	match = value - p->fd;	
	printf("Find FD fd:[%d], value:[%d], match:[%d]\n", p->fd, value, match);
	return match;
};

int findTerminating(FDElement_t* p, int value){
	int match;
	match = value - p->terminating;	
	printf("Find Terminating fd:[%d], value:[%d], match:[%d]\n", p->fd, value, match);
	return match;
};

int revcomp(FDElement_t** p, FDElement_t** q){
//int revcomp(const void* a, const void* b){
	int match;
	/*
	FDElement_t** p;
	FDElement_t** q;
	p = (FDElement_t**)a;
	q = (FDElement_t**)b;
	*/
	match = -((*p)->fd - q[0]->fd);
	return match;
};

int server_sock_create(int port){
    int sockfd;
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
    servaddr.sin_port = htons(port);
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Socket bind failed...(%d)\n", errno);
		perror("Bind fail");
        exit(0);
    }
    else
        printf("Socket successfully binded to port:[%d]\n", port);
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

	return sockfd;
};

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

FDElement_t* alloc_FDElement_t(Container_t *p, int newfd){
	FDElement_t *itemp;
	
	itemp = calloc(1, sizeof(FDElement_t));
	if (itemp == 0){
		printf("No Memory.\n");
		return 0;
	};
	itemp->fd = newfd;
	//itemp->terminating = 0; initalized to 0 by calloc
	//itemp->readcount = 0;
	container_add(p, itemp); 	
	return itemp;
};

struct pollfd* makePoll(Container_t* p){
	int nfds = p->numElement;
	if (nfds == 0){
		printf("No elements in container.\n");
		return 0;
	};
	struct pollfd *pfds;
	FDElement_t **np = (FDElement_t **)p->pool;
	pfds = calloc(nfds, sizeof(struct pollfd));
	if (pfds == NULL){
		printf("No memory for pfds.\n");
		return 0;
	};
	for (int i = 0; i < nfds; i++){
		pfds[i].fd = np[i]->fd;
		//pfds[i].fd = np[i][0].fd; same as above statement
		pfds[i].events = POLLIN;
	};
	return pfds;
};
void test123(){
	Container_t* p;
	p = container_alloc(2);
	struct pollfd* pfds2 = makePoll(p);
	printf("pfds2: %p\n", pfds2);
	FDElement_t * a, *b, *c, *d, *e;
	a = alloc_FDElement_t(p, 1);
	b = alloc_FDElement_t(p, 20);
	c = alloc_FDElement_t(p, 5);
	d = alloc_FDElement_t(p, 88);
	e = alloc_FDElement_t(p, 14);

	a->terminating = 111;
	a->readcount = 10;
	b->terminating = 222;
	b->readcount = 20;
	c->terminating = 333;
	c->readcount = 30;
	d->terminating = 444;
	d->readcount = 40;
	e->terminating = 555;
	e->readcount = 50;
	
	//alloc will add to container
	/*	
	container_add(p, a);
	container_add(p, b);
	container_add(p, c);
	container_add(p, d);
	container_add(p, e);
	*/
	container_iterator(p, (void(*)(void*))printElement);
	FDElement_t* answer;
	answer = (FDElement_t*)container_findValue(p, (int(*)(void *, void *))findFD, (void *) 3);
	answer = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findTerminating, 444);
	printf("Fd:[%d], Terminating:[%d], readcount[%d]\n", answer->fd, answer->terminating, answer->readcount);		

	qsort(p->pool, p->numElement, sizeof(FDElement_t*), (int(*)(const void*, const void*))revcomp);
	printf("After quicksort....\n");
	container_iterator(p, (void(*)(void*))printElement);
	struct pollfd* pfds = makePoll(p);
	for (int i = 0; i < p->numElement; i++){
		printf("pfds[%d].fd = %d\n", i, pfds[i].fd);
	};
	
};
   
// Driver function
int main(int ac, char* av[]){
	//test123();
	//return 0;
	int sockfd = server_sock_create(8080);
	Container_t* p;
	p = container_alloc(2);
	FDElement_t * a;
	a = alloc_FDElement_t(p, sockfd);
	
    int retval;
	int connfd;
	struct pollfd* pfds = 0;
	
    while(1){
		printf("\n====================\nGoing into poll for connection: \n");
		container_iterator(p, (void(*)(void*))printElement);
		pfds = makePoll(p);

		retval = poll(pfds, p->numElement, 60000);
		printf("retval: [%d]\n", retval);
		if (retval < 0){
			printf("Failed to poll. \n");
			perror("alex");
		} else if (retval == 0){
			printf("We got a timeout. \n");
		} else {
			for (int i=0; retval && i < p->numElement; i++){
				printf("retval = [%d], i = [%d], numElement = [%d]\n", retval, i, p->numElement);
				//pfds[i] check events for this poll struct
				int revents = pfds[i].revents;
				int fd = pfds[i].fd;
				printf("  fd=%d; events: %x %s%s%s\n", fd, revents,
                      (revents & POLLIN)  ? "POLLIN "  : "",
                      (revents & POLLHUP) ? "POLLHUP " : "",
                      (revents & POLLERR) ? "POLLERR " : "");
				if (revents) {
					if (revents & POLLIN){
						//process input event
						if (fd == sockfd){
							connfd = doAccept(sockfd);
							FDElement_t * ip;
							ip = alloc_FDElement_t(p, connfd);
							printf("+++++++++++++++++++++++++++++\n");
							container_iterator(p, (void(*)(void*))printElement);
						
						} else {
							int n = doRead(fd);
							printf("Attempting to write on fd: %d\n", fd);
							if(n < 0){
								printf("Issue on read. Current FD: %d\n", fd);
								exit(0);
							} else if (n == 0){
								printf("Terminating sequence for Current FD: %d\n", fd);
								close(fd);
								FDElement_t* answer;	
								answer = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findFD, fd);
								container_remove(p, (void *)answer);
								free(answer);
							};	
						};
					} else {
						//process hangup or error events
						printf("Unknown event.\n");
					};
					retval--;
				}
			};
		}; 
	};
    close(sockfd);
	FDElement_t* answer;	
	answer = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findFD, sockfd);
	container_remove(p, answer);
	free(answer);
	container_free(p);
	return 0;
}
