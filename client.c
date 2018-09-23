#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdbool.h>

#include "constants.h"
#include "tile.h"
#include "protocol.h"

char field[NUM_TILES_X][NUM_TILES_Y];

void update_tile(int x, int y, char c)
{
    field[x][y] = c;
}

void draw_field()
{
    printf("\n\n\nRemaining mines: %d\n\n ", 10);

    for (int x = 0; x < NUM_TILES_X; x++)
        printf(" %d", x+1);
    printf("\n");

    for (int x = 0; x < NUM_TILES_X * 2 + 1; x++)
        printf("-");
    printf("\n");

    for (int y = 0; y < NUM_TILES_Y; y++)
    {
        printf("%c|", 65 + y);
        for (int x = 0; x < NUM_TILES_X; x++)
            printf("%c ", field[x][y]);   
        printf("\n");
    }
    printf("\n");
}

void game()
{
    char option;
    char position[2];

    while(1)
    {
        draw_field();

        printf("Select option <P, R, Q>: ");
        scanf(" %c", &option);

        if(option == 'Q')
            return;

        printf("Select position: ");
        scanf(" %s2", position);

        int x_pos = position[1] - '0' - 1;
        int y_pos = ((int) position[0]) - 65;

        update_tile(x_pos, y_pos, '+');
    }
}

int spunk(int sock, int protocol, char* message, char* response)
{
    char packet[BUFFER_SIZE];
    packet[0] = protocol + '0';
    strncat(packet, message, 50);
    printf("Sending %s with size %d on sock %d\n", packet, BUFFER_SIZE, sock);
    write(sock, packet, BUFFER_SIZE);
    read(sock, &response, BUFFER_SIZE);
    int status = 0;
    return status;
}

int login(char* ip, int port)
{
    printf("\nConnecting to %s:%d...\n", ip, port);
    
    struct sockaddr_in address;
    struct sockaddr_in serv_addr; 
    
    int sock = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\nSocket creation error.\n"); 
        exit(1);
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
       
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)  
    { 
        printf("\nInvalid address or address not supported.\n"); 
        exit(1); 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nConnection failed.\n"); 
        exit(1); 
    }

    printf(" Connection established.\n");

    printf
    (
        "You are required to login with your registered username and password.\n\n"
    );

    char username[10];
    char password[10];
    char message[100];

    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);

    strncat(message, username, 10);
    strncat(message, "\t", 2);
    strncat(message, password, 10);

    char buffer[BUFFER_SIZE];
    spunk(sock, LOGIN, message, buffer);
    printf("Response: %s", buffer);

    return sock;
}

int menu()
{
    printf("\nWelcome to the Minesweeper gaming system.\n\n");
    printf("Please enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n\n");

    int selection;
    do
    {
        printf("Selection option (1-3): ");
        scanf("%d", &selection);
    } while (selection < 1 || selection > 3);
    return selection;
}

int main(int argc, char* argv[])
{

    // char response[100];
    // printf("%d\n", strlen(response));

    char* ip = argv[1];
    int port = atoi(argv[2]);
    int sock = login(ip, port);
 
    switch (menu())
    {
        case 1:
            game(&sock);
            break;

        case 2:
            return 0;
            break;
    }

    printf("\nThanks for playing!\n");

    return 0;
}