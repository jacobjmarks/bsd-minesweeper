#include <stdio.h>
#include <stdlib.h>

#define WIDTH 10
#define HEIGHT 10
char field[WIDTH][HEIGHT];

void draw_field()
{
    printf(" ");
    for (int i = 0; i < WIDTH; i++) printf(" %d", i);
    printf("\n");
    for (int i = 0; i < WIDTH * 2 + 1; i++) printf("-");
    printf("\n");

    for (int i = 0; i < HEIGHT; i++) {
        printf("%c|", 65 + i);
        for (int j = 0; j < WIDTH; j++) printf("%c", field[i][j]);
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    draw_field();
}