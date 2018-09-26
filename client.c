#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <stdbool.h>
#include <unistd.h>

#include "constants.h"
#include "tile.h"
#include "protocol.h"

#define ctoi(my_char) (my_char-'0')
#define itoc(my_int)  (my_int+'0')

char field[NUM_TILES_X][NUM_TILES_Y];

void update_tile(int x, int y, const char c)
{
    printf("Attempting to update x:%d y:%d with %c\n", x, y, c);
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

void game(int sock)
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

        int protocol = option == 'P' ? FLAG_TILE : REVEAL_TILE;
        
        int x_pos = ctoi(position[1]) - 1;
        int y_pos = ((int) position[0]) - 65;

        char pos_request[BUFFER_SIZE];
        pos_request[0] = x_pos;
        pos_request[1] = y_pos; 

        spunk(sock, protocol, pos_request);
        
        while(1)
        {
            char response[BUFFER_SIZE];
            eavesdrop(sock, response);
            if (response[0] == 'T')
                break;
            update_tile(ctoi(response[0]), ctoi(response[1]), response[3]);
        }
        
    }
}

int eavesdrop(int sock, char* response)
{
    if (read(sock, response, BUFFER_SIZE) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
    return 0;
}

int spunk(int sock, int protocol, const char* message)
{
    char packet[BUFFER_SIZE];
    packet[0] = itoc(protocol);
    strncat(packet, message, 99);
    printf("Sending '%s' with size %d... ", packet, BUFFER_SIZE);
    
    if (write(sock, packet, BUFFER_SIZE) < 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }

    return 0;
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
    printf("You are required to login with your registered username and password.\n\n");

    char username[10];
    char password[10];

    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);

    char credentials[BUFFER_SIZE];
    sprintf(credentials,"%s:%s", username, password);
    
    spunk(sock, LOGIN, credentials);
    char response[BUFFER_SIZE];
    eavesdrop(sock, response);

    if (response[0])
    {
        return sock;
    }
    
    printf("Login failure.\n");

    exit(1);
}

int menu()
{
    printf
        (
            "\nWelcome to the Minesweeper gaming system.\n\n"
            "Please enter a selection:\n"
            "<1> Play Minesweeper\n"
            "<2> Show Leaderboard\n"
            "<3> Quit\n\n"
        );
           
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
    char* ip = argv[1];
    int port = atoi(argv[2]);
    int sock = login(ip, port);
 
    switch (menu())
    {
        case 1:
            game(sock);
            break;

        case 2:
            return 0;
            break;
    }

    printf("\nThanks for playing!\n");

    return 0;
}