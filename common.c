/**
 * commonc.c
 * 
 * Common functions used for server-client communication.
 *  
 * Author: Benjamin Saljooghi n9448233
 */


#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "common.h"

/**
 * Sends an int via the socket at the given file descriptor. Gurantees
 * network byte order.
 * 
 * fd: the file descriptor for the socket
 * message: an int to be sent
 */ 
int send_int(int fd, uint32_t message)
{
    message = htonl(message);
    return write(fd, &message, sizeof(uint32_t));
}

/**
 * Receives the next int via the socket at the given file descriptor. The int
 * received is translated from network byte order to host byte order.
 * 
 * fd: the file descriptor for the socket
 * response: an int pointer that the response will be written to.
 */ 
int recv_int(int fd, uint32_t* response)
{
    if (read(fd, response, sizeof(uint32_t)) <= 0)
    {
        return 0;
    }
    *response = ntohl(*response);
    return 1;
}

/**
 * Sends a string via the socket at the given file descriptor. The string is
 * sent by first sending an int representing the byte length of the string,
 * followed by the string itself.
 * 
 * fd: the file descriptor for the socket
 * message: a string to be sent
 * 
 */ 
int send_string(int fd, char* message)
{
    return send_int(fd, strlen(message)) && write(fd, message, strlen(message));
}

/**
 * Receives a string via the socket at the given file descriptor. The string is
 * received by first receiving an int representing the byte length of the
 * incoming string, followed by the string itself.
 * 
 * fd: the file descriptor for the socket
 * response: a string pointer that the response will be written to (memory must
 * be freed by the calling function)
 * 
 */ 
int recv_string(int fd, char** response)
{
    uint32_t length;
    if (recv_int(fd, &length) <= 0)
    {
        return 0;
    }
    *response = calloc(length, sizeof(char));
    return read(fd, *response, length);
}