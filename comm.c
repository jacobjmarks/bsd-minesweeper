#include "constants.h"
#include "comm.h"

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

    if (protocol)
    {
        packet[0] = itoc(protocol);
    }

    strncat(packet, message, protocol ? 99 : 100);

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
