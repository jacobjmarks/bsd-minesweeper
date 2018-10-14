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
    uint32_t remaining_mines;
} GameState_t;

/**
 * Receives the next string sent by the server. Gracefully terminates
 * the client if no connection could be made
 * 
 * fd: the file descriptor for the socket
 * response: a string pointer that the response will be written to (memory must
 * be freed by the calling function)
 */
void eavesdrop(int fd, char** response)
{
    if (recv_string(fd, response) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
}

/**
 * Sends a string prepended by a protocol to the server. Gracefully terminates
 * the client if no connection could be made
 * 
 * fd: the file descriptor for the socket
 * protocol: a single-digit between 1 and 9 inclusive specifying the type of
 *           message that is being sent (see common.h)
 * message: the string to be sent
 */
void spunk(int fd, int protocol, char* message)
{
    char* packet = calloc(strlen(message) + 1, sizeof(char));
    packet[0] = itoc(protocol);
    strcat(packet, message);
    if (send_string(fd, packet) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
    free(packet);
}

/**
 * checks if a coordinate is within the limits of the field
 * 
 * x: the x-coordinate
 * y: the y-coordinate
 * 
 * returns: 1 if the coordinate was valid, otherwise 0
 */ 
bool valid_coord(int x, int y)
{
    return x >= 0 && x < NUM_TILES_X && y >= 0 && y < NUM_TILES_Y;
}

/**
 * updates a field tile at a coordinate with a new character
 * 
 * gs: The gamestate struct containing the field
 * x: the x-coordinate
 * y: the y-coordinate
 * c: the new character
 */ 
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

/**
 * draws the field on the terminal
 * 
 * gs: the gamestate struct containing the field
 */ 
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

/**
 * gets the next move from the user
 * 
 * x: a pointer to an x-coordinate, updated by the function
 * y: a pointer to a y-coordinate, updated by the function
 * 
 * returns: an int representing the option selected (either QUIT,
 *          FLAG_TILE, or REVEAL_TILE)
 */ 
int option(int* x, int* y)
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
        *x = ctoi(position[1]) - 1;
        *y = itoascii(position[0]);
    } while (!valid_coord(*x, *y));
    
    return option == 'P' ? FLAG_TILE : REVEAL_TILE;
}

int game(int fd)
{
    GameState_t gs; memset(&gs, 0, sizeof(GameState_t));

    spunk(fd, PLAY, "");
    recv_int(fd, &gs.remaining_mines);

    int protocol, x_pos, y_pos;
    int gameover = 0;

    draw_field(&gs);
    while((protocol = option(&x_pos, &y_pos)) != QUIT)
    {
        char* response;
        char pos_request[50] = {0};
        pos_request[0] = itoc(x_pos);
        pos_request[1] = itoc(y_pos);
        spunk(fd, protocol, pos_request);
        eavesdrop(fd, &response);
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
                    eavesdrop(fd, &response);

                }

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
    spunk(fd, QUIT, "");
    return QUIT;
}


void leaderboard(int fd)
{
    printf("============================================================\n");

    spunk(fd, LEADERBOARD, "");   
    char* response;
    eavesdrop(fd, &response);
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
        eavesdrop(fd, &response);
    }            

    free(response);

    printf("============================================================\n");
}

int login(const char* ip, int port)
{
    printf("\nConnecting to %s:%d...\n", ip, port);
    
    struct sockaddr_in serv_addr; 
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
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

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        printf("\nConnection failed.\n");
        exit(1);
    }

    printf("Connection established.\n");

    char* play_response;
    eavesdrop(fd, &play_response);
    if (ctoi(play_response[0]) != PLAY)
    {
        printf("Server is at capacity. You have been placed into a queue...\n");
        eavesdrop(fd, &play_response);
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
    
    spunk(fd, LOGIN, credentials);

    char* response;
    eavesdrop(fd, &response);

    if (atoi(response))
    {
        free(response);
        return fd;
    }
    
    free(response);
    printf("Login failure.\n");
    exit(1);
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Invalid usage! Should be: ./client.o [ip] [port]\n");
        return 1;
    }
 
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int fd = login(ip, port);
       
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
            case 1:
                game(fd);
                break;
            case 2:
                leaderboard(fd);
                break;
        }
    } while (selection != 3);
    
    printf("\nThanks for playing!\n");

    return 0;
}