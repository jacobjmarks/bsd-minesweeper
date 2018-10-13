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
        printf("Sending '%d'...\n", message);
    message = htons(message);
    return write(fd, &message, sizeof(int));
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
            printf("Response: '%d'", *response);
        return 1;
    }
}

int send_string(int fd, char* message)
{
    if (DEBUG)
        printf("Sending '%s'...\n", message);
    return send_int(fd, strlen(message)) && write(fd, message, strlen(message));
}

char* recv_string(int fd)
{
    int length;
    if (recv_int(fd, &length))
    {
        char* response = calloc(length, sizeof(char));
        read(fd, response, sizeof(response));
        if (DEBUG)
            printf("Response: '%s' (len %d)\n", response, (int)strlen(response));
        return response;
    }
    return strdup("");
}