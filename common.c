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


int send_int(int fd, int message)
{
    message = htons(message);
    return write(fd, &message, sizeof(int));
}

int recv_int(int fd, int* response)
{
    if (read(fd, response, sizeof(int)) <= 0)
    {
        return 0;
    }
    *response = ntohs(*response);
    return 1;
}

int send_string(int fd, char* message)
{
    send_int(fd, strlen(message));
    return write(fd, message, strlen(message));
}

int recv_string(int fd, char** response)
{
    int length;
    if (recv_int(fd, &length) <= 0)
    {
        return 0;
    }
    *response = calloc(length, sizeof(char));
    return read(fd, *response, length);
}