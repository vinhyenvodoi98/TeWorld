CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm

all: client server

client: cchess-client.c linklist.c checkinput.c
	${CC} cchess-client.c linklist.c checkinput.c -o client -pthread

server: cchess-server.c linklist.c checkinput.c board.c
	${CC} cchess-server.c linklist.c checkinput.c board.c -o server -pthread

clean:
	rm -f *.o *~
