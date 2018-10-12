CC = gcc
CFLAGS = -Wall

all: server client

server: server/server.c server/clients.c server/auth.c server/game.c
	$(CC) $(CFLAGS) -pthread \
		server/server.c \
		server/clients.c \
		server/auth.c \
		server/game.c \
		-o server.o

client: client.c
	$(CC) $(CFLAGS) client.c -o client.o

clean:
	rm -f *.o