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


char* recv_string(int fd)
{
    int bslength;
    read(fd, &bslength, sizeof(bslength));
    char* response = calloc(bslength, sizeof(char));
    read(fd, response, sizeof(response));
    if (DEBUG)
        printf("Response: '%s' (len %d)\n", response, (int)strlen(response));
    return response;
}

int send_string(int fd, char* message)
{
    if (DEBUG)
        printf("Sending '%s'...\n", message);
    return write(fd, message, strlen(message));
}