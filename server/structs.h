#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>
#include "../common.h"

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
    int fd;
    char user[32];
    HighScore_t* score;
    GameState_t* gamestate;
} ClientSession_t;

typedef struct ClientQueue {
    int fd;
    bool waiting;
    struct ClientQueue* next;
} ClientQueue_t;

#endif