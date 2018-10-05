#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "constants.h"

#define ctoi(char)(char - '0')
#define itoc(int)(int + '0')

#define NUM_MINES 10

typedef struct {
    int adjacent_mines;
    bool revealed;
    bool is_mine;
    bool sent;
} Tile;

typedef struct {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

typedef struct {
    char* user;
    int sock;
} ThreadData;

void print_tile_state(Tile tiles[NUM_TILES_X][NUM_TILES_Y]) {
    printf("[adjacent_mines revealed is_mine]\n");
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile tile = tiles[x][y];
            printf("[%d %d %d] ", tile.adjacent_mines, tile.revealed, tile.is_mine);
        }
        printf("\n");
    }
}

void place_mines(Tile tiles[NUM_TILES_X][NUM_TILES_Y]) {
    for (int i = 0; i < NUM_MINES; i++) {
        int x_pos, y_pos;
        Tile* tile;

        do {
            x_pos = rand() % NUM_TILES_X;
            y_pos = rand() % NUM_TILES_Y;
            tile = &tiles[x_pos][y_pos];
        } while(tile->is_mine);

        tile->is_mine = true;
    }
}

void set_adjacent_mines(Tile tiles[NUM_TILES_X][NUM_TILES_Y]) {
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile* tile = &tiles[x][y];
            int adjacent_mines = 0;
            for (int i = -1; i < 2; i++) {
                for (int j = -1; j < 2; j++) {
                    if (x+i < 0 || x+i >= NUM_TILES_X) continue;
                    if (y+j < 0 || y+j >= NUM_TILES_Y) continue;
                    if (tiles[x+i][y+j].is_mine) adjacent_mines++;
                }
            }
            tile->adjacent_mines = adjacent_mines;
        }
    }
}

int init_server(int port) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

GameState* create_gamestate() {
    GameState* gs = malloc(sizeof(GameState));
    memset(gs, 0, sizeof(GameState));
    place_mines(gs->tiles);
    set_adjacent_mines(gs->tiles);
    return gs;
}

void reveal_and_traverse(int x, int y, GameState* gs) {
    printf("RAT %d %d\n", x, y);
    Tile* self = &gs->tiles[x][y];
    self->revealed = true;

    if (self->adjacent_mines != 0) return;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            if (x+i < 0 || x+i >= NUM_TILES_X) continue;
            if (y+j < 0 || y+j >= NUM_TILES_Y) continue;

            Tile* tile = &gs->tiles[x+i][y+j];
            if (tile->revealed) continue;

            if (tile->adjacent_mines == 0) {
                reveal_and_traverse(x+i, y+j, gs);
            } else {
                if (!tile->is_mine) tile->revealed = true;
            }
        }
    }
}

void* client_thread(void* data) {
    int tid = pthread_self();

    ThreadData td = *(ThreadData*)data;

    printf("T%x: Listening...\n", tid);

    int menu_selection;

    do {
        char request[PACKET_SIZE];
        if (read(td.sock, request, PACKET_SIZE) <= 0) {
            printf("T%x exiting: Error connecting to client.\n", tid);
            break;
        }
        menu_selection = ctoi(request[0]);

        switch(menu_selection) {
            case PLAY: {
                printf("T%x starts playing...\n", tid);
                GameState* gs = create_gamestate();
                time_t start_time = time(NULL);

                while (true) {
                    char request[PACKET_SIZE];
                    if (read(td.sock, request, PACKET_SIZE) <= 0) {
                        printf("T%x exiting: Error connecting to client.\n", tid);
                        free(gs);
                        break;
                    }
                    int protocol = ctoi(request[0]);

                    printf("T%x serving {\n", tid);
                    printf("    Protocol: %d\n", protocol);
                    printf("    Message:  %s\n", request + 1);
                    printf("}\n");

                    switch (protocol) {
                        case REVEAL_TILE: {
                            int pos_x = ctoi(request[1]);
                            int pos_y = ctoi(request[2]);

                            printf("Revealing tile %d:%d...\n", pos_x, pos_y);

                            Tile* tile = &gs->tiles[pos_x][pos_y];

                            if (tile->revealed) {
                                char response[PACKET_SIZE] = {0};
                                response[0] = 'T';
                                printf("Responding: %s\n", response);
                                send(td.sock, &response, PACKET_SIZE, 0);
                                break;
                            }

                            if (tile->is_mine) {
                                // Stream all mine positions
                                for (int y = 0; y < NUM_TILES_Y; y++) {
                                    for (int x = 0; x < NUM_TILES_X; x++) {
                                        if (gs->tiles[x][y].is_mine) {
                                            char response[PACKET_SIZE] = {0};
                                            response[0] = itoc(x);
                                            response[1] = itoc(y);
                                            response[2] = '*';
                                            printf("Responding: %s\n", response);
                                            send(td.sock, &response, PACKET_SIZE, 0);
                                            tile->sent = true;
                                        }
                                    }
                                }

                                char terminate[PACKET_SIZE] = {0};
                                terminate[0] = 'T';
                                printf("Responding: %s\n", terminate);
                                send(td.sock, &terminate, PACKET_SIZE, 0);

                                break;
                            }

                            reveal_and_traverse(pos_x, pos_y, gs);

                            // Stream all revealed tiles
                            for (int y = 0; y < NUM_TILES_Y; y++) {
                                for (int x = 0; x < NUM_TILES_X; x++) {
                                    Tile* tile = &gs->tiles[x][y];
                                    if (!tile->is_mine && tile->revealed && !tile->sent) {
                                        char response[PACKET_SIZE] = {0};
                                        response[0] = itoc(x);
                                        response[1] = itoc(y);
                                        response[2] = itoc(tile->adjacent_mines);
                                        printf("Responding: %s\n", response);
                                        send(td.sock, &response, PACKET_SIZE, 0);
                                        tile->sent = true;
                                    }
                                }
                            }

                            char terminate[PACKET_SIZE] = {0};
                            terminate[0] = 'T';
                            printf("Responding: %s\n", terminate);
                            send(td.sock, &terminate, PACKET_SIZE, 0);

                            break;
                        }
                        case FLAG_TILE: {
                            int pos_x = ctoi(request[1]);
                            int pos_y = ctoi(request[2]);

                            printf("Flagging tile %d:%d...\n", pos_x, pos_y);

                            Tile* tile = &gs->tiles[pos_x][pos_y];

                            if (!tile->is_mine) {
                                char response[PACKET_SIZE] = {0};
                                response[0] = 'T';
                                printf("Responding: %s\n", response);
                                send(td.sock, &response, PACKET_SIZE, 0);
                                break;
                            }

                            // Flag and return remaining number of mines...

                            tile->revealed = true;
                            tile->sent = true;

                            int mines_remaining = 0;

                            for (int y = 0; y < NUM_TILES_Y; y++) {
                                for (int x = 0; x < NUM_TILES_X; x++) {
                                    Tile* tile = &gs->tiles[x][y];
                                    if (tile->is_mine && !tile->revealed) {
                                        mines_remaining++;
                                    }
                                }
                            }
                            
                            char response[PACKET_SIZE] = {0};
                            response[0] = itoc(mines_remaining);
                            printf("Responding: %s\n", response);
                            send(td.sock, &response, PACKET_SIZE, 0);

                            break;
                        }
                        default: break;
                    }
                }
            }
            case LEADERBOARD: {

            }
            default: break;
        }
    } while (menu_selection == LEADERBOARD);

    return NULL;
}

bool authenticate(char* user, char* pass) {
    bool authenticated = false;

    FILE* auth_file = fopen("authentication.tsv", "r");

    char line[255];
    fgets(line, sizeof(line), auth_file); // Skip first line
    while (fgets(line, sizeof(line), auth_file) != NULL) {
        bool user_match = strcmp(user, strtok(line, "\t")) == 0;
        bool pass_match = strcmp(pass, strtok(NULL, "\n")) == 0;
        if (user_match && pass_match) {
            authenticated = true;
            break;
        }
    }
    fclose(auth_file);

    return authenticated;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./server.o PORT\n");
        return 1;
    }
    
    srand(time(NULL));

    int port = atoi(argv[1]);
    int server_fd = init_server(port);

    bool create_new_client = true;
    int sock;

    while (true) {
        if (create_new_client) {
            printf("Waiting for socket connection...\n");
            if ((sock = accept(server_fd, NULL, NULL)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("New client listening...\n");
        }
        create_new_client = false;

        char request[PACKET_SIZE];
        if (read(sock, request, PACKET_SIZE) <= 0) {
            printf("Closing connection: Error connecting to client.\n");
            create_new_client = true;
            continue;
        }
        int protocol = ctoi(request[0]);

        printf("Serving {\n");
        printf("    Protocol: %d\n", protocol);
        printf("    Message:  %s\n", request + 1);
        printf("}\n");
        
        switch (protocol) {
            case LOGIN:;
                char credentials[PACKET_SIZE];
                strncpy(credentials, request + 1, strlen(request));

                char* user = strtok(credentials, ":");
                char* pass = strtok(NULL, "\n");

                if (user == NULL || pass == NULL) {
                    printf("Error parsing credentials.\n");
                    char response[PACKET_SIZE] = {0};
                    strcat(response, "0");
                    printf("Responding: %s\n", response);
                    send(sock, &response, PACKET_SIZE, 0);
                    break;
                }

                printf("Authenticating %s:%s...", user, pass);
                bool authenticated = authenticate(user, pass);

                if (authenticated) {
                    printf("Granted\n");
                    printf("Creating new client thread...");
                    ThreadData data;
                    data.user = user;
                    data.sock = sock;
                    pthread_t pid;
                    pthread_create(&pid, NULL, client_thread, &data);
                    printf("Done (%x)\n", (int)pid);
                    create_new_client = true;
                } else {
                    printf("Denied\n");
                }

                char response[PACKET_SIZE] = {0};
                strcat(response, authenticated ? "1" : "0");
                printf("Responding: %s\n", response);
                send(sock, &response, PACKET_SIZE, 0);
                break;
            default:;
                break;
        }
    }

    return 0;
}