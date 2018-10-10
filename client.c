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

int sock;

typedef struct GameState {
    char field[NUM_TILES_X][NUM_TILES_Y];
    int remaining_mines;
} GameState_t;

char* eavesdrop()
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

void spunk(int protocol, const char* message)
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

void update_tile(GameState_t* gs, int x, int y, const char c)
{
    if (DEBUG)
    {
        printf("Attempting to update x:%d y:%d with %c\n", x, y, c);
    }
    
    if (x < 0 || x > 9 || y < 0 || y > 9)
    {
        printf("jhapisugapisfyghas\n");
    }
    else
    {
        // f(x, y) = c;
        gs->field[x][y] = c;
    }
        
}

void draw_field(GameState_t* gs)
{
    system("clear");
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
            // char c = f(x, y);
            char c = gs->field[x][y];
            printf("%c ", c ? c : ' ');
        }
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
        y_pos = itoascii(position[0]);
        x_valid = x_pos >= 0 && x_pos < NUM_TILES_X;
        y_valid = y_pos >= 0 && y_pos < NUM_TILES_Y;
    } while (!x_valid || !y_valid);
    
    *x_pos_ref = x_pos;
    *y_pos_ref = y_pos;
    return option == 'P' ? FLAG_TILE : REVEAL_TILE;
}

int game()
{
    GameState_t gs;
    memset(&gs, 0, sizeof(GameState_t));

    spunk(PLAY, "");
    gs.remaining_mines = 10;
    // gs.remaining_mines = eavesdrop();

    int protocol, x_pos, y_pos;
    int gameover = 0;

    draw_field(&gs);
    while((protocol = option(&x_pos, &y_pos)) != QUIT)
    {
        char* response;
        char pos_request[PACKET_SIZE] = {0};
        
        pos_request[0] = itoc(x_pos);
        pos_request[1] = itoc(y_pos); 

        spunk(protocol, pos_request);
        switch (protocol)
        {
            case FLAG_TILE:
                if((response = eavesdrop(sock))[0] == TERMINATOR)
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
                while((response = eavesdrop(sock))[0] != TERMINATOR)
                {
                    if(response[2] == MINE)
                    {
                        gameover = true;
                    }
                    update_tile(&gs, ctoi(response[0]), ctoi(response[1]), response[2]);    
                    draw_field(&gs);
                    usleep(1000 * 10);
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
    return QUIT;
}


int login(const char* ip, int port)
{
    printf("\nConnecting to %s:%d...\n", ip, port);
    
    struct sockaddr_in address;
    struct sockaddr_in serv_addr; 

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
    printf("You are required to login with your registered username and password.\n\n");

    char username[10];
    char password[10];

    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);

    char credentials[PACKET_SIZE];
    sprintf(credentials,"%s:%s", username, password);
    
    spunk(LOGIN, credentials);
    char* response = eavesdrop();

    if (atoi(response))
    {
        return sock;
    }
    
    printf("Login failure.\n");
    exit(1);
}

void leaderboard()
{
    printf("==========================================================================\n");

    spunk(LEADERBOARD, "");   
    char* response;
    if ((response = eavesdrop())[0] == TERMINATOR)
        printf("There is no information currently stored in the leaderboard. Try again later.\n");

    while(response[0] != TERMINATOR)
    {
        char *name = strtok(response, DELIM);
        char *seconds = strtok(NULL, DELIM);
        char *games_won = strtok(NULL, DELIM);
        char *games_played = strtok(NULL, DELIM);
        
        printf("%s \t %s seconds \t %s games won, %s games played\n", name, seconds, games_won, games_played);
        response = eavesdrop();
    }            

    printf("==========================================================================\n");
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Invalid usage! Should be: client.o [ip] [port]\n");
        return 1;
    }
 
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    sock = login(ip, port);
       
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
        switch (selection)
        {
            case PLAY:
                game();
                break;
            case LEADERBOARD:
                leaderboard();
                break;
        }
    } while (selection != QUIT);
    
    printf("\nThanks for playing!\n");

    return 0;
}