CC = gcc
CFLAGS = -Wall

all: server.o client.o

server.o: server.c
	$(CC) $(CFLAGS) -pthread server.c -o server.o

client.o: client.c
	$(CC) $(CFLAGS) client.c -o client.o

clean:
	rm -f *.o