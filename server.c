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
#define NUM_CLIENT_THREADS 10

typedef struct Tile {
    int adjacent_mines;
    bool revealed;
    bool is_mine;
    bool sent;
} Tile_t;

typedef struct GameState {
    Tile_t tiles[NUM_TILES_X][NUM_TILES_Y];
    time_t start_time;
    bool game_over;
} GameState_t;

typedef struct HighScore {
    char user[32];
    int best_time;
    int games_won;
    int games_played;
    struct HighScore* next;
} HighScore_t;

typedef struct ClientSession {
    int tid;
    int sock;
    char user[32];
    HighScore_t* score;
    GameState_t* gamestate;
} ClientSession_t;

typedef struct ClientQueue {
    int sock;
    struct ClientQueue* next;
} ClientQueue_t;

ClientQueue_t* clients;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_client = PTHREAD_COND_INITIALIZER;

HighScore_t* leaderboard;

void print_tile_state(Tile_t tiles[NUM_TILES_X][NUM_TILES_Y]) {
    printf("[adjacent_mines revealed is_mine]\n");
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t tile = tiles[x][y];
            printf("[%d %d %d] ", tile.adjacent_mines, tile.revealed, tile.is_mine);
        }
        printf("\n");
    }
}

void place_mines(Tile_t tiles[NUM_TILES_X][NUM_TILES_Y]) {
    for (int i = 0; i < NUM_MINES; i++) {
        int x_pos, y_pos;
        Tile_t* tile;

        do {
            x_pos = rand() % NUM_TILES_X;
            y_pos = rand() % NUM_TILES_Y;
            tile = &tiles[x_pos][y_pos];
        } while(tile->is_mine);

        tile->is_mine = true;
    }
}

void set_adjacent_mines(Tile_t tiles[NUM_TILES_X][NUM_TILES_Y]) {
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t* tile = &tiles[x][y];
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

GameState_t* create_gamestate() {
    GameState_t* gs = malloc(sizeof(GameState_t));
    memset(gs, 0, sizeof(GameState_t));
    place_mines(gs->tiles);
    set_adjacent_mines(gs->tiles);
    gs->game_over = false;
    return gs;
}

void reveal_and_traverse(int x, int y, GameState_t* gs) {
    printf("RAT %d %d\n", x, y);
    Tile_t* self = &gs->tiles[x][y];
    self->revealed = true;

    if (self->adjacent_mines != 0) return;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue;
            if (x+i < 0 || x+i >= NUM_TILES_X) continue;
            if (y+j < 0 || y+j >= NUM_TILES_Y) continue;

            Tile_t* tile = &gs->tiles[x+i][y+j];
            if (tile->revealed) continue;

            if (tile->adjacent_mines == 0) {
                reveal_and_traverse(x+i, y+j, gs);
            } else {
                if (!tile->is_mine) tile->revealed = true;
            }
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

HighScore_t* get_highscore(char* user) {
    printf("Getting highscore for user: %s\n", user);
    if (!leaderboard) {
        printf("  Creating new leaderboard\n");
        leaderboard = calloc(1, sizeof(HighScore_t));
        memset(leaderboard, 0, sizeof(HighScore_t));
        strcpy(leaderboard->user, user);
        leaderboard->best_time = 999;
        printf("    User: %s\n", leaderboard->user);
        return leaderboard;
    }

    printf("  Searching leaderboard\n");
    HighScore_t* previous;
    HighScore_t* score = leaderboard;
    while (score) {
        printf("    %s %s\n", score->user, user);
        if (strcmp(score->user, user) == 0) {
            printf("      FOUND\n");
            return score;
        }
        previous = score;
        score = score->next;
    }

    printf("  Creating new entry\n");
    score = calloc(1, sizeof(HighScore_t));
    memset(score, 0, sizeof(HighScore_t));
    strcpy(score->user, user);
    score->best_time = 999;
    printf("    User: %s\n", score->user);

    return previous->next = score;
}

int client_login(int sock, char* user) {
    bool authenticated = false;

    while (!authenticated) {
        char request[PACKET_SIZE];
        if (read(sock, request, PACKET_SIZE) <= 0) {
            printf("Closing connection: Error connecting to client.\n");
            return 1;
        }
        int protocol = ctoi(request[0]);

        if (protocol != LOGIN) continue;

        printf("Serving {\n");
        printf("    Protocol: %d\n", protocol);
        printf("    Message:  %s\n", request + 1);
        printf("}\n");

        char credentials[PACKET_SIZE];
        strncpy(credentials, request + 1, strlen(request));

        char* input_user = strtok(credentials, ":");
        char* input_pass = strtok(NULL, "\n");

        if (input_user != NULL && input_pass != NULL) {
            printf("Authenticating %s:%s...", input_user, input_pass);
            if ((authenticated = authenticate(input_user, input_pass))) {
                printf("  Granted");
                strcpy(user, input_user);
            } else {
                printf("  Denied");
            }
        } else {
            printf("Error parsing credentials.\n");
        }
        
        char response[PACKET_SIZE] = {0};
        strcat(response, authenticated ? "1" : "0");
        printf("Responding: %s\n", response);
        send(sock, &response, PACKET_SIZE, 0);
    }

    return strlen(user) ? 0 : 1;
}

void send_terminator(int sock) {
    char message[PACKET_SIZE] = {0};
    message[0] = TERMINATOR;
    printf("Sending terminator\n");
    send(sock, &message, PACKET_SIZE, 0);
}

void lose_game(ClientSession_t* session) {
    // Stream all mine positions
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t* tile = &session->gamestate->tiles[x][y];
            if (tile->is_mine) {
                char response[PACKET_SIZE] = {0};
                response[0] = itoc(x);
                response[1] = itoc(y);
                response[2] = '*';
                printf("Responding: %s\n", response);
                send(session->sock, &response, PACKET_SIZE, 0);
                tile->sent = true;
            }
        }
    }

    send_terminator(session->sock);

    session->gamestate->game_over = true;
}

void stream_tiles(ClientSession_t* session) {
    // Stream all revealed and unsent tiles
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t* tile = &session->gamestate->tiles[x][y];
            if (!tile->is_mine && tile->revealed && !tile->sent) {
                char response[PACKET_SIZE] = {0};
                response[0] = itoc(x);
                response[1] = itoc(y);
                response[2] = itoc(tile->adjacent_mines);
                printf("Responding: %s\n", response);
                send(session->sock, &response, PACKET_SIZE, 0);
                tile->sent = true;
            }
        }
    }

    send_terminator(session->sock);
}

void reveal_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Revealing tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (tile->revealed) return send_terminator(session->sock);
    if (tile->is_mine) return lose_game(session);

    reveal_and_traverse(pos_x, pos_y, session->gamestate);

    stream_tiles(session);
}

void flag_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Flagging tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (!tile->is_mine) return send_terminator(session->sock);

    // Flag and return remaining number of mines...

    tile->revealed = true;
    tile->sent = true;

    int mines_remaining = 0;

    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t* tile = &session->gamestate->tiles[x][y];
            if (tile->is_mine && !tile->revealed) {
                mines_remaining++;
            }
        }
    }
    
    char response[PACKET_SIZE] = {0};
    response[0] = itoc(mines_remaining);
    printf("Responding: %s\n", response);
    send(session->sock, &response, PACKET_SIZE, 0);
    
    if (!mines_remaining) {
        session->gamestate->game_over = true;
        session->score->games_won++;
        time_t elapsed = time(NULL) - session->gamestate->start_time;
        if (elapsed < session->score->best_time) {
            session->score->best_time = elapsed;
        }
    }
}

void play_game(ClientSession_t* session) {
    printf("T%d starts playing...\n", session->tid);
    session->gamestate = create_gamestate();
    session->gamestate->start_time = time(NULL);
    session->score->games_played++;

    while (!session->gamestate->game_over) {
        char request[PACKET_SIZE];
        if (read(session->sock, request, PACKET_SIZE) <= 0) {
            printf("T%d exiting: Error connecting to client.\n", session->tid);
            free(session->gamestate);
            break;
        }
        int protocol = ctoi(request[0]);

        printf("T%d serving {\n", session->tid);
        printf("    Protocol: %d\n", protocol);
        printf("    Message:  %s\n", request + 1);
        printf("}\n");

        switch (protocol) {
            case REVEAL_TILE: {
                int pos_x = ctoi(request[1]);
                int pos_y = ctoi(request[2]);
                reveal_tile(pos_x, pos_y, session);
                break;
            }
            case FLAG_TILE: {
                int pos_x = ctoi(request[1]);
                int pos_y = ctoi(request[2]);
                flag_tile(pos_x, pos_y, session);
                break;
            }
            case QUIT: {
                session->gamestate->game_over = true;
                break;
            }
            default: break;
        }
    }
}

void stream_leaderboard(int sock) {
    HighScore_t* score = leaderboard;

    while (score != NULL) {
        char response[PACKET_SIZE] = {0};
        sprintf(response, "%s,%d,%d,%d",
            score->user,
            score->best_time,
            score->games_won,
            score->games_played
        );

        printf("Responding: %s\n", response);
        send(sock, &response, PACKET_SIZE, 0);
        score = score->next;
    }

    send_terminator(sock);
}

ClientSession_t* create_client_session(int tid, int sock) {
    ClientSession_t* session = calloc(1, sizeof(ClientSession_t));

    if (client_login(sock, session->user) != 0) {
        free(session);
        return NULL;
    }

    session->tid = tid;
    session->sock = sock;
    session->score = get_highscore(session->user);

    return session;
}

void serve_client(ClientSession_t* session) {
    if (!session) return;

    printf("T%d Listening...\n", session->tid);

    int menu_selection;

    do {
        char request[PACKET_SIZE];
        if (read(session->sock, request, PACKET_SIZE) <= 0) {
            printf("T%d exiting: Error connecting to client.\n", session->tid);
            break;
        }
        menu_selection = ctoi(request[0]);

        switch(menu_selection) {
            case PLAY: {
                play_game(session);
                break;
            }
            case LEADERBOARD: {
                stream_leaderboard(session->sock);
                break;
            }
            case QUIT: {
                return;
            }
            default: break;
        }
    } while (true);
}

int get_client() {
    if (clients) {
        ClientQueue_t* client = clients;
        clients = clients->next;
        int sock = client->sock;
        free(client);
        return sock;
    }

    return 0;
}

void* handle_client_queue(void* data) {
    int tid = *(int*)data;
    printf("TID: %d\n", tid);

    pthread_mutex_lock(&mutex);

    while(true) {
        if (clients) {
            int sock = get_client();
            pthread_mutex_unlock(&mutex);
            serve_client(create_client_session(tid, sock));
            pthread_mutex_lock(&mutex);
        } else {
            pthread_cond_wait(&new_client, &mutex);
        }
    }
}

void queue_client(int sock) {
    if (!clients) {
        clients = calloc(1, sizeof(ClientQueue_t));
        clients->sock = sock;
    } else {
        ClientQueue_t* client = clients;
        while (client->next) {
            client = client->next;
        }
        client->next = calloc(1, sizeof(ClientQueue_t));
        client->next->sock = sock;
    }
    pthread_cond_signal(&new_client);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ./server.o PORT\n");
        return 1;
    }
    
    srand(time(NULL));

    int port = atoi(argv[1]);
    int server_fd = init_server(port);

    int tids[NUM_CLIENT_THREADS];
    pthread_t pthreads[NUM_CLIENT_THREADS];

    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        tids[i] = i;
        pthread_create(&pthreads[i], NULL, handle_client_queue, &tids[i]);
    }

    int sock;

    while (true) {
        printf("Waiting for socket connection...\n");
        if ((sock = accept(server_fd, NULL, NULL)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        } else {
            printf("Adding client to queue\n");
            queue_client(sock);
        }
    }

    return 0;
}