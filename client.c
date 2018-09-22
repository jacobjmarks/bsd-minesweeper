#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "structs.h"

char field[NUM_TILES_X][NUM_TILES_Y];

void draw_field()
{
    printf("\n");

    for (int i = 0; i < NUM_TILES_X; i++)
    {
        printf(" %d", i);
    }
    printf("\n");

    for (int i = 0; i < NUM_TILES_X * 2 + 1; i++)
    {
        printf("-");
    }
    printf("\n");

    for (int i = 0; i < NUM_TILES_Y; i++)
    {
        printf("%c|", 65 + i);
        for (int j = 0; j < NUM_TILES_X; j++)
        {
            printf("%c", field[i][j]);   
        }
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    draw_field();
}