all: echoStringClient

../container/container.o: ../container/container.c \
						  ../container/container.h
	gcc -g -o ../container/container.o -c ../container/container.c

../eventdispatcher/eventdispatcher.o: ../eventdispatcher/eventdispatcher.c \
						   			../eventdispatcher/eventdispatcher.h
	gcc -g -o ../eventdispatcher/eventdispatcher.o -c ../eventdispatcher/eventdispatcher.c

../eventhandler/eventhandler.o: ../eventhandler/eventhandler.c \
								../eventhandler/client_control_handler.h
	gcc -g -o ../eventhandler/eventhandler.o -c ../eventhandler/eventhandler.c

echoStringClient.o: echoStringClient.c ../eventhandler/client_control_handler.h   \
						../eventdispatcher/eventdispatcher.h \
						 ../container/container.h
	gcc -g -c echoStringClient.c

echoStringClient: echoStringClient.o ../eventhandler/eventhandler.o ../eventdispatcher/eventdispatcher.o ../container/container.o
	gcc -g -o echoStringClient echoStringClient.o 				\
				../eventhandler/eventhandler.o 		\
		   		../eventdispatcher/eventdispatcher.o \
				../container/container.o

clean:
	rm -f echoStringClient echoStringClient.o
	rm -f ../eventhandler/eventhandler.o ../eventdispatcher/eventdispatcher.o 
	rm -f ../container/container.o


