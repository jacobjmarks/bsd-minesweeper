#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "constants.h"
#include "tile.h"

#define NUM_MINES 10

typedef struct {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

void place_mines(Tile*** tiles) {
    for (int i = 0; i < NUM_MINES; i++) {
        int x_pos, y_pos;
        do {
            x_pos = rand() % NUM_TILES_X;
            y_pos = rand() % NUM_TILES_Y;
        } while (tile_contains_mine(x_pos, y_pos));
        Tile tile = *tiles[x_pos][y_pos];
        tile.is_mine = true;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide server port.");
        return 1;
    }

    const int port = stoi(argv[1]);

    // Listen

    return 0;
}