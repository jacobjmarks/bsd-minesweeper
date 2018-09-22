#include "constants.h"

#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct {
    int adjacent_mines;
    bool revealed;
    bool is_mine;
} Tile;

typedef struct {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

#endif