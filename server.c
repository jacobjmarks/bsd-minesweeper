#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "constants.h"
#include "tile.h"

#define NUM_MINES 10

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

void serve_client() {
    GameState* gs = malloc(sizeof(GameState));

    place_mines(gs->tiles);
    set_adjacent_mines(gs->tiles);

    print_tile_state(gs->tiles);

    while (true) {
        // Listen
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