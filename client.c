/**
 * client.c
 * 
 * Client program for playing Minesweeper.
 * 
 * Author: Benjamin Saljooghi n9448233
 */


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

#define MINE '*'
#define FLAG '+'
#define DELIM ","

typedef struct GameState {
    char field[NUM_TILES_X][NUM_TILES_Y];
    int remaining_mines;
} GameState_t;

void eavesdrop(int fd, char** response)
{
    printf("Attempting to eavesdrop...\n");
    if (recv_string(fd, response))
    {
        printf("Writing response: '%s'\n", *response);
        return;
    }
    printf("Connection failure.\n");
    exit(1);
}

void spunk(int sock, int protocol, char* message)
{
    printf("Attempting to send protocol %d with message '%s'\n", protocol, message);
    char* packet = calloc(strlen(message) + 1, sizeof(char));
    packet[0] = itoc(protocol);
    strcat(packet, message);
    if (send_string(sock, packet) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
    free(packet);
}

bool valid_coord(int x, int y)
{
    return x >= 0 && x < NUM_TILES_X && y >= 0 && y < NUM_TILES_Y;
}

void update_tile(GameState_t* gs, int x, int y, const char c)
{
    if (DEBUG)
    {
        printf("Attempting to update x:%d y:%d with %c\n", x, y, c);
    }
    if (!valid_coord(x, y))
    {
        printf("Invalid field update.\n");
    }
    else
    {
        gs->field[x][y] = c;
    }   
}

void draw_field(GameState_t* gs)
{
    // system("clear");
    printf("\n\n\nRemaining mines: %d\n\n ", gs->remaining_mines);

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
        {
            char c = gs->field[x][y];
            printf("%c ", c ? c : ' ');
        }
        printf("\n");
    }
    printf("\n");
}

int option(int* x_pos_ref, int* y_pos_ref)
{
    char option, position[2];

    do
    {
        printf("Select option <P, R, Q>: ");
        scanf("%c", &option);
        while(getchar() != '\n');

        if (option == 'Q')
        {
            return QUIT;
        }
    } while (!(option == 'P' || option == 'R') || isalpha(option) == 0);

    do
    {
        printf("Select position: ");
        scanf("%2s", position);
        while(getchar() != '\n');
        *x_pos_ref = ctoi(position[1]) - 1;
        *y_pos_ref = itoascii(position[0]);
    } while (!valid_coord(*x_pos_ref, *y_pos_ref));
    
    return option == 'P' ? FLAG_TILE : REVEAL_TILE;
}

int game(int sock)
{
    GameState_t gs;
    memset(&gs, 0, sizeof(GameState_t));

    spunk(sock, PLAY, "");
    char* remaining_mines;
    eavesdrop(sock, &remaining_mines);
    gs.remaining_mines = atoi(remaining_mines);
    free(remaining_mines);

    int protocol, x_pos, y_pos;
    int gameover = 0;

    draw_field(&gs);
    while((protocol = option(&x_pos, &y_pos)) != QUIT)
    {
        char* response;
        char pos_request[50] = {0};
        pos_request[0] = itoc(x_pos);
        pos_request[1] = itoc(y_pos);
        printf("Pos request is %s\n", pos_request);
        spunk(sock, protocol, pos_request);
        eavesdrop(sock, &response);
        switch (protocol)
        {
            case FLAG_TILE:
                if(response[0] == TERMINATOR)
                {
                    printf("\nNo mine at that position!\n");
                }
                else
                {
                    gs.remaining_mines = ctoi(response[0]);
                    update_tile(&gs, x_pos, y_pos, FLAG);
                }
                break;
            case REVEAL_TILE:
                while(response[0] != TERMINATOR)
                {
                    char response_x = ctoi(response[0]);
                    char response_y = ctoi(response[1]);
                    char response_char = response[2];
                    if(response_char == MINE)
                    {
                        gameover = true;
                    }
                    update_tile(&gs, response_x, response_y, response_char);
                    free(response);    
                    eavesdrop(sock, &response);
                }
                break;
            case QUIT:
                printf("Client protocol error! Consult programmers!");
                break;
            default:
                printf("Client protocol error! Consult programmers!");
                break;
        }
        draw_field(&gs);
        if (gs.remaining_mines == 0)
        {
            printf("You won!!!\n");
            return WIN;
        }
        if (gameover == 1)
        {
            printf("Game over!!!\n");
            return LOSE;
        }
    }
    spunk(sock, QUIT, "");
    return QUIT;
}


void leaderboard(int sock)
{
    printf("============================================================\n");

    spunk(sock, LEADERBOARD, "");   
    char* response;
    eavesdrop(sock, &response);
    if (response[0] == TERMINATOR)
    {
        printf(
            "There is no information currently stored in the leaderboard."
            "Try again later.\n"
        );
    }
        
    while(response[0] != TERMINATOR)
    {
        char *name = strtok(response, DELIM);
        char *seconds = strtok(NULL, DELIM);
        char *games_won = strtok(NULL, DELIM);
        char *games_played = strtok(NULL, DELIM);
        
        printf("%s \t %s seconds \t %s games won, %s games played\n",
                name,
                seconds,
                games_won,
                games_played);
        eavesdrop(sock, &response);
    }            

    free(response);

    printf("============================================================\n");
}

int login(const char* ip, int port)
{
    printf("\nConnecting to %s:%d...\n", ip, port);
    
    struct sockaddr_in serv_addr; 
    int sock;

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

    printf("Connection established.\n");

    char* play_response;
    eavesdrop(sock, &play_response);
    if (ctoi(play_response[0]) != PLAY)
    {
        printf("Server is at capacity. You have been placed into a queue...\n");
        eavesdrop(sock, &play_response);
    }
    // free(play_response);

    printf("Please login.\n");

    char username[10];
    char password[10];

    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);

    char credentials[22];
    sprintf(credentials,"%s:%s", username, password);
    
    spunk(sock, LOGIN, credentials);
    
    // free(username);
    // free(password);
    // free(credentials);

    char* response;
    eavesdrop(sock, &response);

    if (atoi(response))
    {
        free(response);
        return sock;
    }
    
    free(response);
    printf("Login failure.\n");
    exit(1);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Invalid usage! Should be: client.o [ip] [port]\n");
        return 1;
    }
 
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int sock = login(ip, port);
       
    int selection;
    do
    {
        printf
        (
            "\nWelcome to the Minesweeper gaming system.\n\n"
            "Please enter a selection:\n"
            "<1> Play Minesweeper\n"
            "<2> Show Leaderboard\n"
            "<3> Quit\n\n"
            "Selection option (1-3): "
        );

        scanf("%d", &selection);
        while(getchar() != '\n');

        switch (selection)
        {
            case 1: // Play
                game(sock);
                break;
            case 2: // Leaderboard
                leaderboard(sock);
                break;
        }
    } while (selection != 3); // Quit
    
    printf("\nThanks for playing!\n");

    return 0;
}