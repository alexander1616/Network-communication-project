#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <ctype.h>
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <errno.h>
#include "../dumpbuffer/dumpbuffer.h"
#include "../container/container.h"
#include "../eventhandler/client_control_handler.h"
#include "../eventdispatcher/eventdispatcher.h"

		/*******************************************
 			Callback functions are defined here:
		********************************************/

//Helper function to broadcast message
static int sendBroadcastMessage(Container_t* p, char* msg, int msglen, int restrictfd){
    FDElement_t **ep = (FDElement_t **)p->pool;
    int n;
    int x=0;
    for (int i = 0; i< p->numElement; i++){
        if((ep[i]->allowbroadcast == 1) && (ep[i]->fd != restrictfd)){
            n = write(ep[i]->fd, msg, msglen);
            if (n != msglen){
                printf("Attempted to write at fd:[%d] with [%d], return %d, errno: %d\n", ep[i]->fd, msglen, n, errno);
            } else {
                x++;
            };
        };
    };
    return x;
};

//Helper function to read string from Server
static int doReadServer( FDElement_t* ep, Container_t* p){
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

//helper function to handle timeout
static int doTimeout(Eventdispatcher_t* edp){
    printf("User inactive goodbye.\n");
    exit(0);
};

//Helper function to cleanup sockfd and container
static void cleanup(Container_t* p){
    container_iterator(p, (void(*)(void*))FDElement_kill);
    container_free(p);
};

//World starts here
int main(int ac, char* av[]){
	CCH_data_t* ep;
	ep = CCH_create("127.0.0.1", 8080);
	/******************************************
 		we have ep->fd for communication 
	******************************************/
    Container_t*      	p; 
    Eventdispatcher_t*  edp;
	FDElement_t* 		ip;

	p = container_alloc(2);
	edp = eventdispatcher_create();
	edp->containerP = p;
	edp->onTimeoutEvent = doTimeout;
	ip = FDElement_alloc(p, ep->fd, doReadServer);
	ip->allowbroadcast = 1;
	ip = FDElement_alloc(p, 0, doReadUser);
	eventdispatcher_loop(edp, 60000000);
    eventdispatcher_free(edp);
    cleanup(p);
	return 0;
}
