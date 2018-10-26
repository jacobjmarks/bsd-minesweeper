/*******************************************************************************
 * client.c
 *
 * Client program for playing Minesweeper.
 *
 * Author: Benjamin Saljooghi n9448233
 ******************************************************************************/

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

#define MINE '*'
#define FLAG '+'
#define DELIM ","

typedef struct GameState {
    char field[NUM_TILES_X][NUM_TILES_Y];
    uint32_t remaining_mines;
} GameState_t;

typedef struct HighScore {
    char* name;
    uint32_t game_time;
    uint32_t games_won;
    uint32_t games_played;
} HighScore_t;

/**
 * Reports a connection failure to the user and terminates the client.
 */
void connection_failure() {
    printf("Connection failure.\n");
    exit(1);
}

/**
 * Sends an int to the server. Gracefully terminates the client
 * if no connection could be made.
 *
 * fd: the file descriptor for the socket.
 * message: the int to be sent.
 */
void send_int_check(int fd, uint32_t message) {
    if (send_int(fd, message) <= 0) {
        connection_failure();
    }
}

/**
 * Receives the next int sent by the server. Gracefully terminates the
 * client if no connection could be made.
 *
 * fd: the file descriptor for the socket.
 * response: an int pointer that the response will be written to.
 *
 */
void recv_int_check(int fd, uint32_t* response) {
    if (recv_int(fd, response) <= 0) {
        connection_failure();
    }
}

/**
 * Sends a string prepended by a protocol to the server. Gracefully terminates
 * the client if no connection could be made/
 *
 * fd: the file descriptor for the socket
 * protocol: a single-digit between 1 and 9 inclusive specifying the type of
 *           message that is being sent (see common.h)
 * message: the string to be sent
 */
void send_string_check(int fd, int protocol, char* message) {
    char* packet = calloc(strlen(message) + 1, sizeof(char));
    packet[0] = itoc(protocol);
    strcat(packet, message);
    int bytes_sent = send_string(fd, packet);
    free(packet);
    if (bytes_sent <= 0) {
        connection_failure();
    }
}

/**
 * Receives the next string sent by the server. Gracefully terminates
 * the client if no connection could be made
 *
 * fd: the file descriptor for the socket
 * response: a string pointer that the response will be written to (memory must
 * be freed by the calling function)
 */
void recv_string_check(int fd, char** response) {
    if (recv_string(fd, response) <= 0) {
        connection_failure();
    }
}

/**
 * checks if a coordinate is within the limits of the field
 *
 * x: the x-coordinate
 * y: the y-coordinate
 *
 * returns: 1 if the coordinate was valid, otherwise 0
 */
bool valid_coord(int x, int y) {
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
void update_tile(GameState_t* gs, int x, int y, const char c) {
    if (DEBUG) {
        printf("Attempting to update x:%d y:%d with %c\n", x, y, c);
    }
    if (!valid_coord(x, y)) {
        printf("Invalid field update.\n");
    } else {
        gs->field[x][y] = c;
    }
}

/**
 * draws the field on the terminal
 *
 * gs: the gamestate struct containing the field
 */
void draw_field(GameState_t* gs) {
    printf("\n\n\nRemaining mines: %d\n\n ", gs->remaining_mines);

    for (int x = 0; x < NUM_TILES_X; x++)
        printf(" %d", x + 1);
    printf("\n");

    for (int x = 0; x < NUM_TILES_X * 2 + 1; x++)
        printf("-");
    printf("\n");

    for (int y = 0; y < NUM_TILES_Y; y++) {
        printf("%c|", 65 + y);
        for (int x = 0; x < NUM_TILES_X; x++) {
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
int option(int* x, int* y) {
    char option, position[2];

    // Get option (place flag, reveal flag, or quit)
    do {
        printf("Select option <P, R, Q>: ");
        scanf("%c", &option);
        while (getchar() != '\n');

        if (option == 'Q') {
            return QUIT;
        }
    } while (!(option == 'P' || option == 'R') || isalpha(option) == 0);

    // Get position request if user didn't quit
    do {
        printf("Select position: ");
        scanf("%2s", position);
        while (getchar() != '\n')
            ;
        *x = ctoi(position[1]) - 1;
        *y = itoascii(position[0]);
    } while (!valid_coord(*x, *y));

    return option == 'P' ? FLAG_TILE : REVEAL_TILE;
}

/**
 * plays a game of Minesweeper
 *
 * fd: file descriptor to the socket for server communication
 *
 * returns: the result of the game (QUIT, WIN, or LOSE)
 */
int game(int fd) {
    GameState_t gs = {0};

    // Request to play game
    send_int_check(fd, PLAY);
    // Get the initial mine count
    recv_int_check(fd, &gs.remaining_mines);

    int protocol, x_pos, y_pos;
    bool gameover = false;
    char* response;
    draw_field(&gs);

    // Query next move until user quits
    while ((protocol = option(&x_pos, &y_pos)) != QUIT) {
        // Tell the server the user's move
        char pos_request[] = {itoc(x_pos), itoc(y_pos)};
        send_string_check(fd, protocol, pos_request);

        // Get the first response from the server. Interpret the
        // response differently based on whether the user flagged
        // or revealed.
        recv_string_check(fd, &response);
        switch (protocol) {
        case FLAG_TILE:
            // There was no mine
            if (response[0] == TERMINATOR) {
                printf("\nNo mine at that position!\n");
            }
            // User correctly flagged a mine
            else {
                recv_int_check(fd, &gs.remaining_mines);
                update_tile(&gs, x_pos, y_pos, FLAG);
            }
            break;
        case REVEAL_TILE:
            // Reveal all tiles the server sends back
            while (response[0] != TERMINATOR) {
                char response_x = ctoi(response[0]);
                char response_y = ctoi(response[1]);
                char response_char = response[2];
                // Game is over if the server sends a mine.
                if (response_char == MINE) {
                    gameover = true;
                }
                update_tile(&gs, response_x, response_y, response_char);
                recv_string_check(fd, &response);
            }

            break;
        default:
            printf("Client protocol error! Consult programmers!\n");
            break;
        }
        draw_field(&gs);
        if (gs.remaining_mines == 0) {
            uint32_t game_time;
            recv_int_check(fd, &game_time);
            printf("You won in %d seconds!!!\n", game_time);
            return WIN;
        }
        if (gameover) {
            printf("Game over!!!\n");
            return LOSE;
        }
    }
    send_string_check(fd, QUIT, "");
    return QUIT;
}

/**
 * Compares two high scores to determine their relative position with respect
 * to the following ordering (from the spec):
 *
 * Highs scores are displayed in descending order of the number of seconds each
 * successful game took to complete. If two or more games have the same number
 * of seconds then the game played by the player with the highest number of
 * games won are displayed last. If two or more games were won in the
 * same number of seconds by players with the same number of games won then
 * display those games by the names of their players in alphabetical order.
 *
 * highscore_a: A pointer to a HighScore_t, to be compared against highscore_b.
 * highscore_b: A pointer to a HighScore_t, to be compared against highscore_a.
 *
 * returns: an int representing the position of highscore_a relative to
 * highscore_b, with respect to the ordering described above.
 *
 */
int compare_highscores(const void* highscore_a, const void* highscore_b) {
    HighScore_t* a = (HighScore_t*)highscore_a;
    HighScore_t* b = (HighScore_t*)highscore_b;
    if (a->game_time > b->game_time) {
        return -1;
    } else if (a->game_time < b->game_time) {
        return 1;
    }

    if (a->games_won < b->games_won) {
        return -1;
    } else if (a->games_won > b->games_won) {
        return 1;
    }

    return strcmp(a->name, b->name);
}

/**
 * Gets and prints the server leaderboard
 *
 * fd: file descriptor to the socket for server communication
 *
 */
void leaderboard(int fd) {
    // Request leaderboard from server
    send_int_check(fd, LEADERBOARD);

    // Get leaderboard size from server
    uint32_t num_users;
    recv_int_check(fd, &num_users);

    // If leaderboard is empty, tell the user
    if (num_users == 0) {
        printf(
            "============================================================\n"
            "There is no information currently stored in the leaderboard."
            "Try again later.\n"
            "============================================================\n");
        return;
    }


    char* names[num_users];
    uint32_t games_played[num_users];
    uint32_t games_won[num_users];
    uint32_t* game_times[num_users];

    uint32_t num_rows = 0;
    for (uint32_t i = 0; i < num_users; i++) {
        recv_string_check(fd, names + i);
        recv_int_check(fd, games_played + i);
        recv_int_check(fd, games_won + i);
 
        num_rows += games_won[i];
        game_times[i] = malloc(games_won[i] * sizeof(uint32_t));
        
        for (uint32_t t = 0; t < games_won[i]; t++) {
            recv_int_check(fd, game_times[i] + t);
        }
    }

    HighScore_t* highscores = malloc(sizeof(HighScore_t) * num_rows);

    int row_index = 0;
    for (uint32_t user_index = 0; user_index < num_users; user_index++) {
        for (uint32_t time_index = 0; time_index < games_won[user_index]; time_index++) {
            printf("Creating struct at row_index %d\n", row_index);
            highscores[row_index].name = names[user_index];
            highscores[row_index].games_played = games_played[user_index];
            highscores[row_index].games_won = games_won[user_index];
            highscores[row_index].game_time = game_times[user_index][time_index];
            row_index++;
        }
    }

    // Sort the highscores
    qsort(highscores, num_rows, sizeof(*highscores), &compare_highscores);

    // Display the ordered high scores
    printf("============================================================\n");
    for (uint32_t i = 0; i < num_rows; i++) {
        printf("%-10s \t %10d seconds \t %d games won, %d games played\n",
               highscores[i].name, highscores[i].game_time,
               highscores[i].games_won, highscores[i].games_played);
    }
    printf("============================================================\n");

    // Cleanup
    free(highscores);
}

/**
 * Logs in to a Minesweeper server
 *
 * ip: a string representing the IP address of the server
 * port: an int representing the port of the server
 *
 * returns: a file descriptor to the socket for server communication
 */
int login(const char* ip, int port) {
    printf("\nConnecting to %s:%d...\n", ip, port);

    struct sockaddr_in serv_addr;
    int fd;

    // Setup a socket to be used for connection
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error.\n");
        exit(1);
    }

    // Construct server information based on ip and port
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address or address not supported.\n");
        exit(1);
    }

    // Connect to the server
    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection failed.\n");
        exit(1);
    }

    printf("Connection established.\n");

    // Wait in queue until server is not busy
    uint32_t play_response;
    recv_int_check(fd, &play_response);
    if (play_response != PLAY) {
        printf("Server is at capacity. You have been placed into a queue...\n");
        recv_int_check(fd, &play_response);
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
    sprintf(credentials, "%s:%s", username, password);

    // Send login credentials to server
    send_string_check(fd, LOGIN, credentials);

    // Continue to main menu if server authenticated the credentials
    uint32_t response;
    recv_int_check(fd, &response);
    if (response) {
        return fd;
    }

    printf("Login failure.\n");
    exit(1);
}

/**
 * Program entrypoint. Takes the Minesweeper server IP address and port as
 * arguments
 *
 * Usage: ./client.o ip port
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid usage! Should be: ./client.o [ip] [port]\n");
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);
    int fd = login(ip, port);

    int selection;
    // Present main menu to client until they quit (menu option 3)
    do {
        printf("\nWelcome to the Minesweeper gaming system.\n\n"
               "Please enter a selection:\n"
               "<1> Play Minesweeper\n"
               "<2> Show Leaderboard\n"
               "<3> Quit\n\n"
               "Selection option (1-3): ");

        scanf("%d", &selection);
        while (getchar() != '\n');

        switch (selection) {
            // Play (menu option 1)
            case 1:
                game(fd);
                break;
            // Leaderboard (menu option 2)
            case 2:
                leaderboard(fd);
                break;
        }
    } while (selection != 3);

    // Tell user the client chose to quit
    send_int_check(fd, QUIT);
    printf("\nThanks for playing!\n");
    return 0;
}