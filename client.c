#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "tile.h"

char field[NUM_TILES_X][NUM_TILES_Y];

void update_tile(int x, int y, char c)
{
    field[x][y] = c;
}

void draw_field()
{
    printf("\n\n\nRemaining mines: %d\n\n ", 10);

    for (int x = 0; x < NUM_TILES_X; x++)
        printf(" %d", x+1);
    printf("\n");

    for (int x = 0; x < NUM_TILES_X * 2 + 1; x++)
        printf("-");
    printf("\n");

    for (int y = 0; y < NUM_TILES_Y; y++)
    {
        printf("%c|", 65 + y);
        for (int x = 0; x < NUM_TILES_X; x++)
            printf("%c ", field[x][y]);   
        printf("\n");
    }
    printf("\n");
}

void game()
{
    char option;
    char position[2];

    while(1)
    {
        draw_field();

        printf("Select option <P, R, Q>: ");
        scanf(" %c", &option);

        if(option == 'Q')
            return;

        printf("Select position: ");
        scanf(" %s2", position);

        int x_pos = position[1] - '0' - 1;
        int y_pos = ((int) position[0]) - 65;

        update_tile(x_pos, y_pos, '+');
    }
}

void login()
{
    printf
    (
        "Welcome to the online Minesweeper gaming system\n\n"
        "             . . .                         \n"
        "              \\|/                          \n"
        "            `--+--'                        \n"
        "              /|\\                          \n"
        "             ' | '                         \n"
        "               |                           \n"
        "               |                           \n"
        "           ,--'#`--.                       \n"
        "           |#######|                       \n"
        "        _.-'#######`-._                    \n"
        "     ,-'###############`-.                 \n"
        "   ,'#####################`,               \n"
        "  /#########################\\              \n"
        " |###########################|             \n"
        "|#############################|            \n"
        "|#############################|            \n"
        "|#############################|            \n"
        "|#############################|            \n"
        " |###########################|             \n"
        "  \\#########################/              \n"
        "   `.#####################,'               \n"
        "     `._###############_,'                 \n"
        "        `--..#####..--'\n\n"
        "You are required to login with your registered username and password.\n\n"
    );

    char username[10];
    char password[10];

    printf("Enter your username: ");
    scanf(" %10s", username);  
    printf("Enter your password: ");
    scanf(" %10s", password);
}

int menu()
{
    printf("\nWelcome to the Minesweeper gaming system.\n\n");
    printf("Please enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n\n");

    int selection;

    do
    {
        printf("Selection option (1-3): ");
        scanf("%d", &selection);
    } while (selection < 1 || selection > 3);
    
    return selection;
}

int main(int argc, char* argv[])
{
    char* ip_address = argv[1];
    int port = atoi(argv[2]);

    printf("\nConnecting to %s:%d...\n\n", ip_address, port);
    
    login();
    int selection = menu();

    switch (selection)
    {
        case 1:
            game();
            break;

        case 2:
            return 0;
            break;

        case 3:
            return 0;
            break;
    }

    printf("\nThanks for playing!\n");

    return 0;
}