#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"

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
    int mines_remaining;
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
    bool waiting;
    struct ClientQueue* next;
} ClientQueue_t;

ClientQueue_t* client_queue;
int busy_threads = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t is_new_client = PTHREAD_COND_INITIALIZER;

HighScore_t* leaderboard;

/**
 * Place a set amount of mines at random locations within the given Tile matrix.
 */
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

/**
 * Sets the adjacent mine count for all Tiles in the given matrix.
 */
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

/**
 * Initialises a BSD socker server using the given port.
 * 
 * Returns the created server file descriptor.
 */
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

/**
 * Initialise a new GameState.
 * 
 * Returns a pointer to the created struct.
 */
GameState_t* create_gamestate() {
    GameState_t* gs = malloc(sizeof(GameState_t));
    memset(gs, 0, sizeof(GameState_t));
    place_mines(gs->tiles);
    set_adjacent_mines(gs->tiles);
    gs->game_over = false;
    gs->mines_remaining = NUM_MINES;
    return gs;
}

/**
 * Reveals the Tile at the given location within the Tile matrix of the provided
 * Gamestate. Recursively continues the process if necessary, based on the
 * rules of Minesweeper.
 */
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

/**
 * Authenticates a given user and password against the defined credentials
 * within the external authentication tsv file.
 * 
 * Returns true if the user and password is successfully found.
 */
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

/**
 * Finds the existing or creates a new HighScore for the given user.
 * 
 * Returns a pointer the struct.
 */
HighScore_t* get_highscore(char* user) {
    // Create new leaderboard if none exists
    if (!leaderboard) {
        leaderboard = calloc(1, sizeof(HighScore_t));
        strcpy(leaderboard->user, user);
        leaderboard->best_time = 999;
        return leaderboard;
    }

    // Search leaderboard for user
    HighScore_t* previous;
    HighScore_t* score = leaderboard;
    while (score) {
        if (strcmp(score->user, user) == 0) {
            return score;
        }
        previous = score;
        score = score->next;
    }

    // Create new entry if user not found
    score = calloc(1, sizeof(HighScore_t));
    strcpy(score->user, user);
    score->best_time = 999;

    return previous->next = score;
}

/**
 * Attempts to authenticate an incoming client. If successful, the logged in
 * user is stored in the provided pointer.
 * 
 * Returns 1 if there was an issue completing the process, 0 otherwise.
 */
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

/**
 * Sends a terminator signal to the given socket. Primarily used to indicate the
 * end of a stream of data.
 */
void send_terminator(int sock) {
    char message[PACKET_SIZE] = {0};
    message[0] = TERMINATOR;
    printf("Sending terminator\n");
    send(sock, &message, PACKET_SIZE, 0);
}

/**
 * Completes the neccessary actions when losing a game of Minesweeper. Including
 * streaming all mine positions and setting the gameover state of the given
 * ClientSession.
 */
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

/**
 * Streams all revealed Tiles that have not already been sent within the
 * GameState Tile matrix of the provided ClientSession.
 */
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

/**
 * Reveals a Tile at the given position within the GameState Tile matrix of the
 * provided ClientSession. The client will lose the game is the Tile is a mine.
 */
void reveal_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Revealing tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (tile->revealed) return send_terminator(session->sock);
    if (tile->is_mine) return lose_game(session);

    reveal_and_traverse(pos_x, pos_y, session->gamestate);

    stream_tiles(session);
}
/**
 * Flags a Tile at the given position within the GameState Tile matrix of the
 * provided ClientSession. The client will win the game once all mines have been
 * flagged.
 */
void flag_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Flagging tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (!tile->is_mine) return send_terminator(session->sock);

    tile->revealed = true;

    char response[PACKET_SIZE] = {0};
    response[0] = itoc(--session->gamestate->mines_remaining);
    printf("Responding: %s\n", response);
    send(session->sock, &response, PACKET_SIZE, 0);

    tile->sent = true;
    
    if (!session->gamestate->mines_remaining) {
        session->gamestate->game_over = true;
        session->score->games_won++;
        time_t elapsed = time(NULL) - session->gamestate->start_time;
        if (elapsed < session->score->best_time) {
            session->score->best_time = elapsed;
        }
    }
}

/**
 * Plays a new game of Minesweeper for the given ClientSession.
 */
void play_game(ClientSession_t* session) {
    printf("T%d starts playing...\n", session->tid);
    session->gamestate = create_gamestate();
    session->gamestate->start_time = time(NULL);
    session->score->games_played++;

    char message[PACKET_SIZE] = {0};
    sprintf(message, "%d", session->gamestate->mines_remaining);
    printf("Indicating initial mine count of %s...\n", message);
    send(session->sock, &message, PACKET_SIZE, 0);

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
            case REVEAL_TILE:
                reveal_tile(ctoi(request[1]), ctoi(request[2]), session);
                break;
            case FLAG_TILE:
                flag_tile(ctoi(request[1]), ctoi(request[2]), session);
                break;
            case QUIT:
                session->gamestate->game_over = true;
                break;
            default:
                break;
        }
    }
}

/**
 * Streams all leaderboard entries to the provided socket.
 */
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

/**
 * Initialises a new ClientSession with the given thread ID and socket.
 * 
 * Returns a pointer to the created struct.
 */
ClientSession_t* create_client_session(int tid, int sock) {
    char user[32];

    if (client_login(sock, user) != 0) return NULL;

    ClientSession_t* session = calloc(1, sizeof(ClientSession_t));
    strcpy(session->user, user);
    session->tid = tid;
    session->sock = sock;
    session->score = get_highscore(session->user);

    return session;
}

/**
 * Serves the new client specified in the given ClientSession.
 */
void serve_client(ClientSession_t* session) {
    printf("T%d Listening...\n", session->tid);

    while (true) {
        char request[PACKET_SIZE];
        if (read(session->sock, request, PACKET_SIZE) <= 0) {
            printf("T%d exiting: Error connecting to client.\n", session->tid);
            break;
        }
        int menu_selection = ctoi(request[0]);

        switch(menu_selection) {
            case PLAY:
                play_game(session);
                break;
            case LEADERBOARD:
                stream_leaderboard(session->sock);
                break;
            case QUIT:
                return;
            default:
                break;
        }
    }
}

/**
 * Retreives the next client in the waiting queue in a FIFO manor.
 * 
 * Returns the socket of the client, or 0 if no clients are in the queue.
 */
int get_client() {
    if (client_queue) {
        ClientQueue_t* client = client_queue;
        client_queue = client_queue->next;
        int sock = client->sock;

        if (client->waiting) {
            // Notify client no longer needs to wait
            char message[PACKET_SIZE] = {0};
            message[0] = itoc(PLAY);
            printf("Notifying waiting client...\n");
            send(sock, &message, PACKET_SIZE, 0);
        }

        free(client);
        return sock;
    }

    return 0;
}

/**
 * Thread function to listen to the client waiting queue, serving queued clients
 * where available.
 */
void* handle_client_queue(void* data) {
    int tid = *(int*)data;
    printf("TID: %d\n", tid);

    pthread_mutex_lock(&mutex);

    while(true) {
        if (client_queue) {
            busy_threads++;
            int sock = get_client();
            ClientSession_t* session = create_client_session(tid, sock);

            pthread_mutex_unlock(&mutex);
            if (session) serve_client(session);
            pthread_mutex_lock(&mutex);
            
            busy_threads--;
        } else {
            pthread_cond_wait(&is_new_client, &mutex);
        }
    }
}

/**
 * Adds a new entry in the ClientQueue with the given socket. If all threads are
 * busy, the client will be marked as waiting.
 */
void queue_client(int sock) {
    ClientQueue_t* new_client;

    if (!client_queue) {
        client_queue = calloc(1, sizeof(ClientQueue_t));
        client_queue->sock = sock;
        new_client = client_queue;
    } else {
        ClientQueue_t* client = client_queue;
        while (client->next) {
            client = client->next;
        }
        client->next = calloc(1, sizeof(ClientQueue_t));
        client->next->sock = sock;
        new_client = client->next;
    }

    bool needs_to_wait = busy_threads == NUM_CLIENT_THREADS;
    if (needs_to_wait) new_client->waiting = true;

    char message[PACKET_SIZE] = {0};
    message[0] = itoc(needs_to_wait ? QUEUED : PLAY);
    printf("Thread available: %s\n", needs_to_wait ? "NO" : "YES");
    send(sock, &message, PACKET_SIZE, 0);

    pthread_cond_signal(&is_new_client);
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