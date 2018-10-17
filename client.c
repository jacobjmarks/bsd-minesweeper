/*******************************************************************************
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
 * socket_fd: the file descriptor for the socket
 * response: a string pointer that the response will be written to (memory must
 * be freed by the calling function)
 */
void eavesdrop(int socket_fd, char** response)
{
    if (recv_string(socket_fd, response) <= 0)
    {
        printf("Connection failure.\n");
        exit(1);
    }
}

/**
 * Sends a string prepended by a protocol to the server. Gracefully terminates
 * the client if no connection could be made
 * 
 * socket_fd: the file descriptor for the socket
 * protocol: a single-digit between 1 and 9 inclusive specifying the type of
 *           message that is being sent (see common.h)
 * message: the string to be sent
 */
void spunk(int socket_fd, int protocol, char* message)
{
    char* packet = calloc(strlen(message) + 1, sizeof(char));
    packet[0] = itoc(protocol);
    strcat(packet, message);
    if (send_string(socket_fd, packet) <= 0)
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

    // Get option (place flag, reveal flag, or quit)
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

    // Get position request if user didn't quit
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

/**
 * plays a game of Minesweeper
 * 
 * socket_fd: file descriptor to the socket for server communication
 * 
 * returns: the result of the game (QUIT, WIN, or LOSE)
 */
int game(int socket_fd)
{
    GameState_t gs = {0};

    // Tell the server the user has requested to play
    spunk(socket_fd, PLAY, "");
    // Get the initial mine count
    recv_int(socket_fd, &gs.remaining_mines);

    int protocol, x_pos, y_pos;
    bool gameover = false;
    char* response;
    draw_field(&gs);
    
    // Query next move until user quits
    while((protocol = option(&x_pos, &y_pos)) != QUIT)
    {
        // Tell the server the user's move
        char pos_request[] = {itoc(x_pos), itoc(y_pos)};
        spunk(socket_fd, protocol, pos_request);
        
        // Get the first response from the server. Interpret the
        // response differently based on whether the user flagged
        // or revealed.
        eavesdrop(socket_fd, &response);
        switch (protocol)
        {
            case FLAG_TILE:
                // There was no mine 
                if(response[0] == TERMINATOR)
                {
                    printf("\nNo mine at that position!\n");
                }
                // User correctly flagged a mine
                else
                {
                    gs.remaining_mines = ctoi(response[0]);
                    update_tile(&gs, x_pos, y_pos, FLAG);
                }
                break;
            case REVEAL_TILE:
                // Reveal all tiles the server sends back
                while(response[0] != TERMINATOR)
                {
                    char response_x = ctoi(response[0]);
                    char response_y = ctoi(response[1]);
                    char response_char = response[2];
                    // Game is over if the server sends a mine.
                    if(response_char == MINE)
                    {
                        gameover = true;
                    }
                    update_tile(&gs, response_x, response_y, response_char);
                    eavesdrop(socket_fd, &response);
                }

                break;
            default:
                printf("Client protocol error! Consult programmers!\n");
                break;
        }
        draw_field(&gs);
        if (gs.remaining_mines == 0)
        {
            printf("You won!!!\n");
            return WIN;
        }
        if (gameover)
        {
            printf("Game over!!!\n");
            return LOSE;
        }
    }
    spunk(socket_fd, QUIT, "");
    return QUIT;
}

/**
 * Gets and prints the server leaderboard
 * 
 * socket_fd: file descriptor to the socket for server communication
 * 
 */ 
void leaderboard(int socket_fd)
{
    printf("============================================================\n");

    spunk(socket_fd, LEADERBOARD, "");   
    char* response;
    eavesdrop(socket_fd, &response);
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
        eavesdrop(socket_fd, &response);
    }            

    free(response);

    printf("============================================================\n");
}

/**
 * Logs in to a Minesweeper server
 * 
 * ip: a string representing the IP address of the server
 * port: an int representing the port of the server
 * 
 * returns: a file descriptor to the socket for server communication
 */ 
int login(const char* ip, int port)
{
    printf("\nConnecting to %s:%d...\n", ip, port);
    
    struct sockaddr_in serv_addr; 
    int socket_fd;

    // Setup a socket to be used for connection
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        printf("\nSocket creation error.\n");
        exit(1);
    }

    // Construct server information based on ip and port
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)  
    {
        printf("\nInvalid address or address not supported.\n"); 
        exit(1);
    }

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    {
        printf("\nConnection failed.\n");
        exit(1);
    }

    printf("Connection established.\n");

    // Wait in queue until server is not busy
    char* play_response;
    eavesdrop(socket_fd, &play_response);
    if (ctoi(play_response[0]) != PLAY)
    {
        printf("Server is at capacity. You have been placed into a queue...\n");
        eavesdrop(socket_fd, &play_response);
    }

    // Request login credentials from user
    printf("Please login.\n");
    char username[10];
    char password[10];
    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);
    char credentials[22];
    sprintf(credentials,"%s:%s", username, password);
    
    // Send login credentials to server
    spunk(socket_fd, LOGIN, credentials);

    // Continue to main menu if server authenticated the credentials
    char* response;
    eavesdrop(socket_fd, &response);
    if (atoi(response))
    {
        free(response);
        return socket_fd;
    }
    
    free(response);
    printf("Login failure.\n");
    exit(1);
}

/**
 * Program entrypoint. Takes the Minesweeper server IP address and port as
 * arguments
 * 
 * Usage: ./client.o ip port
 */
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Invalid usage! Should be: ./client.o [ip] [port]\n");
        return 1;
    }
 
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int socket_fd = login(ip, port);
       
    int selection;
    // Present main menu to client until they quit (menu option 3)
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
            // Play (menu option 1)
            case 1:
                game(socket_fd);
                break;
            // Leaderboard (menu option 2)
            case 2:
                leaderboard(socket_fd);
                break;
        }
    } while (selection != 3);
    
    printf("\nThanks for playing!\n");
    return 0;
}