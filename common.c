#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "common.h"


int send_int(int fd, uint32_t message)
{
    message = htonl(message);
    return write(fd, &message, sizeof(uint32_t));
}

int recv_int(int fd, uint32_t* response)
{
    if (read(fd, response, sizeof(uint32_t)) <= 0)
    {
        return 0;
    }
    *response = ntohl(*response);
    return 1;
}

int send_string(int fd, char* message)
{
    return send_int(fd, strlen(message)) && write(fd, message, strlen(message));
}

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