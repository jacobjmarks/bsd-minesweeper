#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "constants.h"
#include "tile.h"

#define NUM_MINES 10

typedef struct {
    Tile tiles[NUM_TILES_X][NUM_TILES_Y];
} GameState;

void print_tile_state(Tile tiles[NUM_TILES_X][NUM_TILES_Y]) {
    printf("[adjacent_mines revealed is_mine]\n");
    for (int x = 0; x < NUM_TILES_X; x++) {
        for (int y = 0; y < NUM_TILES_Y; y++) {
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

void serve_client() {
    GameState* gs = malloc(sizeof(GameState));

    place_mines(gs->tiles);

    print_tile_state(gs->tiles);

    while (true) {
        // Listen
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please provide server port.");
        return 1;
    }

    srand(time(NULL));

    const int port = atoi(argv[1]);

    // Listen
    serve_client();

    return 0;
}