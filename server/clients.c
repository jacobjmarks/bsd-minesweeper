/*******************************************************************************
 * clients.c
 * 
 * Includes methods to handle the queueing and serving of incoming clients.
 * 
 * Author: Jacob Marks n9188100
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../common.h"
#include "structs.h"
#include "auth.h"
#include "game.h"
#include "clients.h"

ClientQueue_t* client_queue;
int busy_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_new_client = PTHREAD_COND_INITIALIZER;

/* -------------------------- FORWARD DECLARATIONS -------------------------- */

int get_client();
ClientSession_t* create_client_session(int, int);
void serve_client(ClientSession_t*);

/* -------------------------------- PUBLIC ---------------------------------- */

/**
 * Adds a new entry in the ClientQueue with the given socket. If all threads are
 * busy, the client will be marked as waiting.
 */
void queue_client(int fd) {
    ClientQueue_t* new_client;

    if (!client_queue) {
        client_queue = calloc(1, sizeof(ClientQueue_t));
        client_queue->fd = fd;
        new_client = client_queue;
    } else {
        ClientQueue_t* client = client_queue;
        while (client->next) {
            client = client->next;
        }
        client->next = calloc(1, sizeof(ClientQueue_t));
        client->next->fd = fd;
        new_client = client->next;
    }

    bool needs_to_wait = busy_threads == NUM_CLIENT_THREADS;
    if (needs_to_wait) new_client->waiting = true;

    char message[2] = { itoc(needs_to_wait ? QUEUED : PLAY) };
    printf("Thread available: %s\n", needs_to_wait ? "NO" : "YES");
    send_string(fd, message);

    pthread_cond_signal(&is_new_client);
}

/**
 * Thread function to listen to the client waiting queue, serving queued clients
 * where available.
 */
void* handle_client_queue(void* data) {
    int tid = *(int*)data;
    printf("TID: %d\n", tid);

    pthread_mutex_lock(&mutex);

    while(true) {
        if (client_queue) {
            busy_threads++;
            int fd = get_client();
            ClientSession_t* session = create_client_session(tid, fd);

            pthread_mutex_unlock(&mutex);
            if (session) serve_client(session);
            pthread_mutex_lock(&mutex);
            
            busy_threads--;
        } else {
            pthread_cond_wait(&is_new_client, &mutex);
        }
    }
}

/* -------------------------------- PRIVATE --------------------------------- */

/**
 * Retreives the next client in the waiting queue in a FIFO manor.
 * 
 * Returns the socket of the client, or 0 if no clients are in the queue.
 */
int get_client() {
    if (client_queue) {
        ClientQueue_t* client = client_queue;
        client_queue = client_queue->next;
        int fd = client->fd;

        if (client->waiting) {
            // Notify client no longer needs to wait
            char message[2] = { itoc(PLAY) };
            printf("Notifying waiting client...\n");
            send_string(fd, message);
        }

        free(client);
        return fd;
    }

    return 0;
}

/**
 * Initialises a new ClientSession with the given thread ID and socket.
 * 
 * Returns a pointer to the created struct.
 */
ClientSession_t* create_client_session(int tid, int fd) {
    char user[32];

    if (client_login(fd, user) != 0) return NULL;

    ClientSession_t* session = calloc(1, sizeof(ClientSession_t));
    strcpy(session->user, user);
    session->tid = tid;
    session->fd = fd;
    session->score = get_highscore(session->user);

    return session;
}

/**
 * Serves the new client specified in the given ClientSession.
 */
void serve_client(ClientSession_t* session) {
    

    printf("T%d Listening...\n", session->tid);

    while (true) {
        char* request;
        if (recv_string(session->fd, &request) <= 0) {
            printf("T%d exiting: Error connecting to client.\n", session->tid);
            break;
        }
        
        int menu_selection = ctoi(request[0]);

        switch(menu_selection) {
            case PLAY:
                play_game(session);
                break;
            case LEADERBOARD:
                stream_leaderboard(session->fd);
                break;
            case QUIT:
                return;
            default:
                break;
        }
    }
}
