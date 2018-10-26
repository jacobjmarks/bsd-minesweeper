/*******************************************************************************
 * game.c
 * 
 * Includes methods to handle the playing of a game of Minesweeper.
 * 
 * Author: Jacob Marks n9188100
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../common.h"
#include "structs.h"
#include "game.h"

HighScore_t* leaderboard;
int leaderboard_size = 0;

/* -------------------------- FORWARD DECLARATIONS -------------------------- */

GameState_t* create_gamestate();
void place_mines(Tile_t[NUM_TILES_X][NUM_TILES_Y]);
void set_adjacent_mines(Tile_t[NUM_TILES_X][NUM_TILES_Y]);
void reveal_tile(int, int, ClientSession_t*);
void reveal_and_traverse(int, int, GameState_t*);
void stream_tiles(ClientSession_t*);
void flag_tile(int, int, ClientSession_t*);
void lose_game(ClientSession_t*);
void send_terminator(int);

/* -------------------------------- PUBLIC ---------------------------------- */

/**
 * Plays a new game of Minesweeper for the given ClientSession.
 */
void play_game(ClientSession_t* session) {
    printf("T%d starts playing...\n", session->tid);
    session->gamestate = create_gamestate();
    session->gamestate->start_time = time(NULL);
    session->score->games_played++;

    printf("Sending initial mine count of %d...\n",
        session->gamestate->mines_remaining);
    send_int(session->fd, session->gamestate->mines_remaining);

    while (!session->gamestate->game_over) {
        char* request;
        if (recv_string(session->fd, &request) <= 0) {
            printf("T%d exiting: Error connecting to client.\n", session->tid);
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

    free(session->gamestate);
}

/**
 * Streams all leaderboard entries to the provided socket.
 */
void stream_leaderboard(int fd) {
    printf("Streaming leaderboard of size %d\n", leaderboard_size);
    send_int(fd, leaderboard_size);

    HighScore_t* score = leaderboard;
    while (score != NULL) {
        if (!score->games_won) {
            score = score->next;
            continue;
        }

        send_string(fd, score->user);
        send_int(fd, score->games_played);
        send_int(fd, score->games_won);
        
        for (int i = 0; i < score->games_won; i++) {
            send_int(fd, score->times[i]);
        }

        score = score->next;
    }
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

    return previous->next = score;
}

/* -------------------------------- PRIVATE --------------------------------- */

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
 * Reveals a Tile at the given position within the GameState Tile matrix of the
 * provided ClientSession. The client will lose the game is the Tile is a mine.
 */
void reveal_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Revealing tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (tile->revealed) return send_terminator(session->fd);
    if (tile->is_mine) return lose_game(session);

    reveal_and_traverse(pos_x, pos_y, session->gamestate);

    stream_tiles(session);
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
 * Streams all revealed Tiles that have not already been sent within the
 * GameState Tile matrix of the provided ClientSession.
 */
void stream_tiles(ClientSession_t* session) {
    // Stream all revealed and unsent tiles
    for (int y = 0; y < NUM_TILES_Y; y++) {
        for (int x = 0; x < NUM_TILES_X; x++) {
            Tile_t* tile = &session->gamestate->tiles[x][y];
            if (!tile->is_mine && tile->revealed && !tile->sent) {
                char response[4];
                sprintf(response, "%d%d%d", x, y, tile->adjacent_mines);
                printf("Responding: %s\n", response);
                send_string(session->fd, response);
                tile->sent = true;
            }
        }
    }

    send_terminator(session->fd);
}

/**
 * Flags a Tile at the given position within the GameState Tile matrix of the
 * provided ClientSession. The client will win the game once all mines have been
 * flagged.
 */
void flag_tile(int pos_x, int pos_y, ClientSession_t* session) {
    printf("Flagging tile %d:%d...\n", pos_x, pos_y);

    Tile_t* tile = &session->gamestate->tiles[pos_x][pos_y];

    if (!tile->is_mine) return send_terminator(session->fd);

    tile->revealed = true;

    send_string(session->fd, "~");

    int new_mine_count = --session->gamestate->mines_remaining;;
    printf("Responding: %d\n", new_mine_count);
    send_int(session->fd, new_mine_count);

    tile->sent = true;
    
    if (!session->gamestate->mines_remaining) {
        session->gamestate->game_over = true;
        session->score->games_won++;
        if (session->score->games_won == 1) leaderboard_size++;
        time_t elapsed = time(NULL) - session->gamestate->start_time;
        session->score->times = realloc(session->score->times, sizeof(int) * session->score->games_won);
        session->score->times[session->score->games_won - 1] = elapsed;
    }
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
                char response[4];
                sprintf(response, "%d%d%c", x, y, '*');
                printf("Responding: %s\n", response);
                send_string(session->fd, response);
                tile->sent = true;
            }
        }
    }

    send_terminator(session->fd);

    session->gamestate->game_over = true;
}

/**
 * Sends a terminator signal to the given socket. Primarily used to indicate the
 * end of a stream of data.
 */
void send_terminator(int fd) {
    char message[2] = { TERMINATOR };
    printf("Sending terminator\n");
    send_string(fd, message);
}
