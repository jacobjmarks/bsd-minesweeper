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
    if (DEBUG)
        printf("send int: %d\n", message);
    int thing = htons(message);
    return write(fd, &thing, sizeof(int));
}

int recv_int(int fd, int* response)
{
    if (read(fd, response, sizeof(response)) <= 0)
    {
        return 0;
    }
    else
    {
        *response = ntohs(*response);
        if (DEBUG)
            printf("recv int: %d\n", *response);
        return 1;
    }
}

int send_string(int fd, char* message)
{
    // return send_int(fd, strlen(message)) && write(fd, message, strlen(message));
    send_int(fd, strlen(message));
     if (DEBUG)
        printf("send string: '%s'\n", message);
    return write(fd, message, strlen(message));
}

int recv_string(int fd, char** response)
{
    int length;
    if (recv_int(fd, &length))
    {
        *response = calloc(length, sizeof(char));
        read(fd, *response, sizeof(*response));
        if (DEBUG)
            printf("recv string: '%s' (len %d)\n", *response, (int)strlen(*response));
        return 1;
    }
    // return strdup("");
    return 0;
}