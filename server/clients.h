#ifndef CLIENTS_H
#define CLIENTS_H

#define NUM_CLIENT_THREADS 10

void queue_client(int);
void* handle_client_queue(void*);
void stop_listening();

#endif