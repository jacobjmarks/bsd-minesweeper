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

#define ctoi(c) (c-'0')
#define itoc(i) (i+'0')

char field[NUM_TILES_X][NUM_TILES_Y];

int remaining_mines = 10;

void update_tile(int x, int y, const char c)
{
    printf("Attempting to update x:%d y:%d with %c\n", x, y, c);
    field[x][y] = c;
}

void draw_field()
{
    printf("\n\n\nRemaining mines: %d\n\n ", remaining_mines);

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


int reveal_position(int x_pos, int y_pos)
{

}

int flag_position(int x_pos, int y_pos)
{

}

int option(int* x_pos_ref, int* y_pos_ref)
{
    char option, position[2];
    int x_pos, y_pos;
    bool x_valid, y_valid;

    do
    {
        printf("Select option <P, R, Q>: ");
        scanf(" %c", &option);
        if (option == 'Q')
            return QUIT;
    } while (option != 'P' && option != 'R');

    do
    {
        printf("Select position: ");
        scanf(" %s2", position);
        x_pos = ctoi(position[1]) - 1;
        y_pos = ((int) position[0]) - 65;
        x_valid = x_pos >= 0 && x_pos < NUM_TILES_X;
        y_valid = y_pos >= 0 && y_pos < NUM_TILES_Y;
    } while (!x_valid || !y_valid);
    
    *x_pos_ref = x_pos;
    *y_pos_ref = y_pos;
    return option == 'P' ? FLAG_TILE : REVEAL_TILE;
}

int game(int sock)
{

    while(true)
    {
        draw_field();

        int protocol, x_pos, y_pos;
        if ((protocol = option(&x_pos, &y_pos)) == QUIT)
            return QUIT;

        char pos_request[BUFFER_SIZE] = {0};
        pos_request[0] = itoc(x_pos);
        pos_request[1] = itoc(y_pos); 

        spunk(sock, protocol, pos_request);
        
        bool gameover = false;

        char response[BUFFER_SIZE];
        while(true)
        {
            eavesdrop(sock, response);
            if (response[0] == TERMINATOR)
                break;    
            if (response[2] == MINE)
                gameover = true;
            if (protocol == FLAG_TILE)
            {
                remaining_mines = ctoi(response[0]);
                update_tile(x_pos, y_pos, FLAG);
                break;
            }
            else
            {
                update_tile(ctoi(response[0]), ctoi(response[1]), response[2]);  
            }
        }

        if (remaining_mines == 0)
        {
            draw_field();
            return WIN;   
        }
        if (gameover)
        {
            draw_field();
            return LOSE;   
        }
    }
    return QUIT;
}

int eavesdrop(int sock, char* response)
{
    if (read(sock, response, BUFFER_SIZE) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
    printf("Response: %s\n", response);
    return 0;
}

int spunk(int sock, int protocol, const char* message)
{
    char packet[BUFFER_SIZE] = {0};
    packet[0] = itoc(protocol);
    strncat(packet, message, 99);
    printf("Sending '%s'...\n", packet, BUFFER_SIZE);
    
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