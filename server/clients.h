#ifndef CLIENTS_H
#define CLIENTS_H

#define NUM_CLIENT_THREADS 10

void* handle_client_queue(void*);
void queue_client(int);

#endif