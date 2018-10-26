CC = gcc
CFLAGS = -Wall

.PHONY: server client

all: server client

server: server/server.c server/clients.c server/auth.c server/game.c common.c
	$(CC) $(CFLAGS) -pthread \
		server/server.c \
		server/clients.c \
		server/auth.c \
		server/game.c \
		common.c \
		-o server.o

client: client.c common.c
	$(CC) $(CFLAGS) client.c common.c -o client.o

clean:
	rm -f *.o