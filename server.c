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
#include "tile.h"
#include "protocol.h"

#define ctoi(char)(char - '0')
#define itoc(int)(int + '0')

#define NUM_MINES 10

int port;

typedef struct {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

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
                    if (x+i < 0 || x+i > NUM_TILES_X) continue;
                    if (y+j < 0 || y+j > NUM_TILES_Y) continue;
                    if (tiles[x+i][y+j].is_mine) adjacent_mines++;
                }
            }
            tile->adjacent_mines = adjacent_mines;
        }
    }
}

int create_socket() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
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

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    return new_socket;
}

GameState* create_gamestate() {
    GameState* gs = malloc(sizeof(GameState));
    place_mines(gs->tiles);
    set_adjacent_mines(gs->tiles);
    return gs;
}

void* client_thread(void* data) {
    int tid = pthread_self();

    int sock = *(int*)data;
    free(data);

    GameState* gs = create_gamestate();

    printf("T%x: Listening...\n", tid);
    while (true) {
        char request[100];
        if (read(sock, request, 100) <= 0) {
            printf("T%x exiting: Error connecting to client.\n", tid);
            pthread_exit(NULL);
        }
        int protocol = ctoi(request[0]);

        printf("T%x serving {\n", tid);
        printf("    Protocol: %d\n", protocol);
        printf("    Message:  %s\n", request + 1);
        printf("}\n");

        switch (protocol) {
            case REVEAL_TILE:;

                break;
            default:;
                break;
        }
    }
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

    port = atoi(argv[1]);

    bool create_new_client = true;
    int sock;

    while (true) {
        if (create_new_client) {
            printf("Waiting for socket connection...\n");
            sock = create_socket();
            printf("New client listening...\n");
        }
        create_new_client = false;

        char request[100];
        if (read(sock, request, 100) <= 0) {
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
                char credentials[100];
                strncpy(credentials, request + 1, strlen(request));

                char* user = strtok(credentials, ":");
                char* pass = strtok(NULL, "\n");

                if (user == NULL || pass == NULL) {
                    printf("Error parsing credentials.\n");
                    char response[100] = {0};
                    strcat(response, "0");
                    printf("Responding: %s\n", response);
                    send(sock, &response, 100, 0);
                    break;
                }

                printf("Authenticating %s:%s...", user, pass);
                bool authenticated = authenticate(user, pass);

                if (authenticated) {
                    printf("Granted\n");
                    printf("Creating new client thread...");
                    int* data = malloc(sizeof(*data));
                    *data = sock;
                    pthread_t pid;
                    pthread_create(&pid, NULL, client_thread, data);
                    printf("Done (%x)\n", (int)pid);
                    create_new_client = true;
                } else {
                    printf("Denied\n");
                }

                char response[100] = {0};
                strcat(response, authenticated ? "1" : "0");
                printf("Responding: %s\n", response);
                send(sock, &response, 100, 0);
                break;
            default:;
                break;
        }
    }

    return 0;
}