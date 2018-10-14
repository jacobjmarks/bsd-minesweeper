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
 * 
 * returns: number of bytes sent or -1 if error
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
 * 
 * returns: number of bytes received or -1 if error
 */ 
int recv_int(int fd, uint32_t* response)
{
    int bytes_read = read(fd, response, sizeof(uint32_t));
    *response = ntohl(*response);
    return bytes_read;
}

/**
 * Sends a string via the socket at the given file descriptor. The string is
 * sent by first sending an int representing the byte length of the string,
 * followed by the string itself.
 * 
 * fd: the file descriptor for the socket
 * message: a string to be sent
 * 
 * returns: number of bytes sent or -1 if error
 */ 
int send_string(int fd, char* message)
{
    int bytes_sent = send_int(fd, strlen(message));
    if (bytes_sent <= 0)
    {
        return bytes_sent;
    }
    return write(fd, message, strlen(message));
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
 * returns: number of bytes received or -1 if error 
 */ 
int recv_string(int fd, char** response)
{
    uint32_t length;
    int bytes_read = recv_int(fd, &length);
    if (bytes_read <= 0)
    {
        return bytes_read;
    }
    *response = calloc(length, sizeof(char));
    return read(fd, *response, length);
}