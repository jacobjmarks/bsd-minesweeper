#ifndef TILE_H
#define TILE_H

typedef struct {
    int adjacent_mines;
    bool revealed;
    bool is_mine;
} Tile;

#endif