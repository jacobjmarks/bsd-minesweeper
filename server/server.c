/*******************************************************************************
 * server.c
 * 
 * Handles the entrypoint of the application including the initialisation of a
 * BSD socket server to listen to incoming client requests.
 * 
 * Author: Jacob Marks n9188100
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>

#include "clients.h"

/* -------------------------- FORWARD DECLARATIONS -------------------------- */

int init_server(int);

/* -------------------------------- PUBLIC ---------------------------------- */

/**
 * Application entrypoint. Takes a single command-line argument specifying the
 * port to use for the BSD server.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./server.o PORT\n");
        return 1;
    }
    
    srand(time(NULL));

    int port = atoi(argv[1]);
    int server_fd = init_server(port);

    int tids[NUM_CLIENT_THREADS];
    pthread_t pthreads[NUM_CLIENT_THREADS];

    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        tids[i] = i;
        pthread_create(&pthreads[i], NULL, handle_client_queue, &tids[i]);
    }

    int socket_fd;

    while (1) {
        printf("Waiting for socket connection...\n");
        if ((socket_fd = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        } else {
            printf("Adding client to queue\n");
            queue_client(socket_fd);
        }
    }

    return 0;
}

/* -------------------------------- PRIVATE --------------------------------- */

/**
 * Initialises a BSD socker server using the given port.
 * 
 * Returns the created server file descriptor.
 */
int init_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}