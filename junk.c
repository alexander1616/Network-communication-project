
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
