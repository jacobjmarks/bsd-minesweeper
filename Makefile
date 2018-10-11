CC = gcc
CFLAGS = -Wall

all: server client

server: server.c
	$(CC) $(CFLAGS) -pthread server.c -o server.o

client: client.c comm.c
	$(CC) $(CFLAGS) client.c comm.c -o client.o

clean:
	rm -f *.o