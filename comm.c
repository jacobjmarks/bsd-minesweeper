#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>

#include "constants.h"

#define ctoi(c) (c-'0')
#define itoc(i) (i+'0')
#define itoascii(i) (((int)i) - 65)


#define DEBUG 1

char* eavesdrop(int sock)
{
    static char response[PACKET_SIZE];
    memset(response, 0, sizeof(response));
    if (read(sock, response, PACKET_SIZE) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
    if (DEBUG)
    {
        printf("Response: '%s' (len %d)\n", response, (int)strlen(response));
    }
    return response;
}

void spunk(int sock, int protocol, const char* message)
{
    char packet[PACKET_SIZE] = {0};
    packet[0] = itoc(protocol);
    strncat(packet, message, 99);

    if (DEBUG)
    {
        printf("Sending '%s'...\n", packet);
    }
    if (write(sock, packet, PACKET_SIZE) < 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
}