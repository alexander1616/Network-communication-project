#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>
#include <ctype.h>
#include <unistd.h> // read(), write(), close()
#include <errno.h>
#include "../../AlexLib/DebugLib/dumpbuffer/dumpbuffer.h"
#include "../../AlexLib/GenLib/container/container.h"
#include "../../AlexLib/GenLib/eventhandler/client_control_handler.h"
#include "../../AlexLib/GenLib/eventdispatcher/eventdispatcher.h"
#define SA struct sockaddr

//FDElement_t is private Element for container
typedef struct FDElement_t{
	int fd;
	int terminating;
	int readcount;
	int (*onReadEvent)(struct FDElement_t*, Container_t*);
} FDElement_t;

//Helper function to allocate memory for Element
FDElement_t* alloc_FDElement_t(Container_t *p, int newfd, int(*func)(FDElement_t*, Container_t*)){
	FDElement_t *itemp;
	itemp = calloc(1, sizeof(FDElement_t));
	if (itemp == 0){
		printf("No Memory.\n");
		return 0;
	};
	itemp->fd = newfd;
	itemp->onReadEvent = func; 
	container_add(p, itemp); 	
	return itemp;
};

//Helper function to print elements from container
void printElement(FDElement_t* p){
	if (p == 0){
		printf("===================\n");
	} else {
		printf("FD:[%d], Terminating:[%d], Readcount:[%d]\n", p->fd, p->terminating, p->readcount);
	}
};

//Helper function to delete all elements from container
void killElements(FDElement_t* p){
	if (p == 0){
		printf("All elements cleared.\n");
	} else {
		close(p->fd);
		p->fd = 0;
		p->terminating = 0;
		p->readcount = 0;
		p->onReadEvent = 0;
		free(p);
	}
};

//Helper function to cleanup sockfd and container
void cleanup_p_sockfd(Container_t* p, int sockfd){
	container_iterator(p, (void(*)(void*))killElements);
	container_free(p);
};

//Helper function to find FD
int findFD(FDElement_t* p, int value){
	int match;
	match = value - p->fd;	
	printf("Finding FD fd:[%d], value:[%d], match:[%d]\n", p->fd, value, match);
	return match;
};

//Helper function to find Terminating
int findTerminating(FDElement_t* p, int value){
	int match;
	match = value - p->terminating;	
	printf("Finding Terminating fd:[%d], value:[%d], match:[%d]\n", p->fd, value, match);
	return match;
};

//Helper function for qsort - reverse
int revcomp(FDElement_t** p, FDElement_t** q){
	int match;
	match = -((*p)->fd - q[0]->fd);
	return match;
};

//Helper function for server socket creation
int server_sock_create(int port){
    int sockfd;
    struct sockaddr_in servaddr, cli;
   
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
   
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        printf("Socket bind failed...(%d)\n", errno);
		perror("Bind fail");
        exit(0);
    }
    else
        printf("Socket successfully binded to port:[%d]\n", port);
   
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

	return sockfd;
};


//Helper function to read/write integer message
int doReadInt( FDElement_t* ep, Container_t *p ){
	int retFlag = 0;
	int connfd = ep->fd;
	uint32_t myTag;
    uint32_t outTag;
    uint32_t inTag;
    int n;
    
    n = read(connfd, &inTag, sizeof(inTag));
	if (n != sizeof(inTag)){
        printf("Attempted to read %lu, return %d, errno: %d\n", sizeof(inTag), n, errno);
		retFlag = -1; 
    	goto out; 
    };

	uint32_t temp;
	//temp = ntohl(inTag);
	temp = inTag;
    printf("From client: %d, %x \n", inTag, inTag);
	if (ep->readcount++ > 0){
		if(ep->terminating == temp){
			printf("Received a terminate signal from client. \n");
			goto out;
		};
	} else {
		ep->terminating = temp;
	};
	myTag = temp + 2;
	//outTag = htonl(myTag);
	outTag = myTag;
    n = write(connfd, &outTag, sizeof(outTag));
	if (n != sizeof(outTag)){
        printf("Attempted to write %lu, return %d, errno: %d\n", sizeof(outTag), n, errno);
		retFlag = -1;
    	goto out;
    } 
	return 0;
out:	
    close(connfd);
    FDElement_t* answer;    
    answer = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findFD, connfd);
    container_remove(p, (void *)answer);
    free(answer);
    return retFlag;
}

//Helper function to read string
static int doReadString( FDElement_t* ep, Container_t* p){
#define BUFFSIZE 1000       
    char buff[BUFFSIZE+1];
    int connfd = ep->fd;
    int n;

    n = read(connfd, buff, BUFFSIZE);
    if (n == -1){
        perror("Error on read");
        exit(0);
    } else if (n == 0){
        printf("No input \n");
        return 0;
    };
    buff[n]=0; //trick to guarantee a null terminated string
    printf("String: [%s], [%d]\n", buff, n);
    return 0;
};

//Helper function to filter white space
void spacefilter(char* c, int len){
    int x = 0;
    for (int i=0; i<len; i++){
        x = isspace(c[i]);
        if (x != 0){
            c[i]=' ';
        };
    };
    return;
};

//helper function to read string from user
static int doReadUser( FDElement_t* ep, Container_t* p){
#define BUFFSIZE 1000       
    char buff[BUFFSIZE+1];
    int fd = ep->fd;
    int n;
    n = read(fd, buff, BUFFSIZE);
    if (n == -1){
        perror("Error on read");
        exit(0);
    } else if (n == 0){
        printf("No input \n");
        return 0;
    };
    spacefilter(buff, n);
    buff[n]=0; //trick to guarantee a null terminated string
    printf("String: [%s], [%d]\n", buff, n);

    //check for terminating string, "EXIT"
    if (!strncmp(buff, "EXIT", 4)){
        printf("Goodbye. \n");
        exit(0);
    };
    sendBroadcastMessage(p, buff, n, fd);
    return 0;
};


//Helper function to create client
int createClient(Container_t* p, int fd){
    printf("Server accept the client...fd: %d\n", fd);
	FDElement_t * ip;
	ip = alloc_FDElement_t(p, fd, doReadInt);
	if (ip == 0)
		return -1;
	return fd;
};

//Helper function to accept client
int doAccept(FDElement_t* ep, Container_t *p){
    int connfd, len;
    struct sockaddr_in servaddr, cli;
	len = sizeof(cli);
	connfd = accept(ep->fd, (SA*)&cli, &len);
    if (connfd < 0) {
    	printf("Server accept failed... connfd: %d\n", connfd);
    	return (-1);
    };
	return createClient(p, connfd);
};  

//Helper function to construct Pollfd
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
		pfds[i].events = POLLIN;
	};
	return pfds;
};

//Helper function to initiate Poll
void pollEvent(Container_t* p, int uSec){
    int retval;
	struct pollfd* pfds = 0;
    for (int testLoop = 0; testLoop < 20; testLoop++){ //loop 20 times for testing
		printf("\n\n=========================\n");
		printf("==Test loop count: [%d]==\n", testLoop);
		pfds = makePoll(p);
		retval = poll(pfds, p->numElement, uSec/1000);
		printf("Retval: [%d]\n", retval);
		if (retval < 0){
			printf("Failed to poll. \n");
			free(pfds);
			pfds = 0;
			continue;
		} else if (retval == 0){
			printf("We got a timeout. \n");
			free(pfds);
			pfds = 0;
			continue;
		} 
		for (int i=0; retval && i < p->numElement; i++){
			printf("numElement %d\n", p->numElement);
			int revents = pfds[i].revents;
			int fd = pfds[i].fd;
			printf("Fd=[%d] Revents=[%x] [%s][%s][%s]\n", fd, revents,
                  (revents & POLLIN)  ? "POLLIN "  : "",
                  (revents & POLLHUP) ? "POLLHUP " : "",
                  (revents & POLLERR) ? "POLLERR " : "");
			if (revents) {
				FDElement_t* ep;
				ep = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findFD, fd);
				if (revents & POLLIN){
					if (ep->onReadEvent){
						(*ep->onReadEvent)(ep, p);
					} else {
						printf("No underlying read event hander.\n");
					};
				} else {
					printf("Unknown event.\n");
				};
				retval--;
			};
		};
		free(pfds);
		pfds = 0;
	};
};

//Select structure for makeSelect
typedef struct {
	fd_set 	rds; 	//read events we are interested in
	int 	maxFd; 	//maximum size of interested fd
} SelectEvent_t;

//Helper function to create Select Event
SelectEvent_t* makeSelect(Container_t* p){
	int nfds = p->numElement;
	if (nfds == 0){
		printf("No elements in container.\n");
		return 0;
	};
	SelectEvent_t *pfds;
	FDElement_t **np = (FDElement_t **)p->pool;
	pfds = calloc(1, sizeof(SelectEvent_t));
	if (pfds == NULL){
		printf("No memory for pfds.\n");
		return 0;
	};
	int maxFd = 0;
	FD_ZERO(&pfds->rds);
	for (int i = 0; i < nfds; i++){
		FD_SET(np[i]->fd, &(pfds->rds));
		if (maxFd < np[i]->fd){
			maxFd = np[i]->fd;
		};
	};
	pfds->maxFd = maxFd +1;
	return pfds;
};

//Helper function to initiate Select
void selectEvent(Container_t* p, int uSec){
    int retval;
	SelectEvent_t* pfds;
	struct timeval tv;
    for (int testLoop = 0; testLoop < 20; testLoop++){ //loop 20 times for testing
		printf("\n\n=========================\n");
		printf("==Test loop count: [%d]==\n", testLoop);
		//pfds = makePoll(p);
		//retval = poll(pfds, p->numElement, uSec/1000);
		
		tv.tv_sec = uSec/1000000;
		tv.tv_usec = uSec - (tv.tv_sec*1000000);
		pfds = makeSelect(p);
		retval = select(pfds->maxFd, &(pfds->rds), NULL, NULL, &tv);		

		printf("Retval: [%d]\n", retval);
		if (retval < 0){
			printf("Failed to Select. \n");
			free(pfds);
			pfds = 0;
			continue;
		} else if (retval == 0){
			printf("We got a timeout. \n");
			free(pfds);
			pfds = 0;
			continue;
		} 
		for (int i=0; retval && i < pfds->maxFd; i++){
			printf("numElement %d\n", p->numElement);
			//int revents = pfds[i].revents;
			int revents = FD_ISSET(i, &(pfds->rds));
			int fd = i;
			printf("Fd=[%d] Revents=[%x] \n", fd, revents);
			if (revents) {
				FDElement_t* ep;
				ep = (FDElement_t*)container_findIntValue(p, (int(*)(void *, int))findFD, fd);
				//if (revents & POLLIN){
				if (revents){
					if (ep->onReadEvent){
						(*ep->onReadEvent)(ep, p);
					} else {
						printf("No underlying read event hander.\n");
					};
				} else {
					printf("Unknown event.\n");
				};
				retval--;
			};
		};
		free(pfds);
		pfds = 0;
	};
};

//World starts here
int main(int ac, char* av[]){
	int sockfd = server_sock_create(8080);
	Container_t* p;
	p = container_alloc(2);
	FDElement_t * a;
	a = alloc_FDElement_t(p, sockfd, doAccept);
	//pollEvent(p, 5000000);
	selectEvent(p, 5000000);
	cleanup_p_sockfd(p, sockfd);
	return 0;
}
