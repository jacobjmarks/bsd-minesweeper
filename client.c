#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "tile.h"

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

void login()
{
    printf("====================\n");
    printf("Welcome to the online Minesweeper gaming system\n");
    printf("====================\n\n");
    printf("You are required to login with your registered username and password.\n\n");
    
    char username[255];
    char password[255];

    printf("Enter your username: ");
    scanf("%255s", username);  
    printf("Enter your password: ");
    scanf("%255s", password);
}

int menu()
{
    printf("Welcome to the Minesweeper gaming system.\n\n");
    printf("Please enter a selection:\n");
    printf("<1> Play Minesweeper\n");
    printf("<2> Show Leaderboard\n");
    printf("<3> Quit\n\n");

    int selection;

    do
    {
        printf("Selection option (1-3):");
        scanf("%d", &selection);
    } while (selection < 1 || selection > 3);
    
    return selection;
}

int main(int argc, char* argv[])
{
    printf("\n");

    char* ip_address = argv[1];
    int port = atoi(argv[2]);

    printf("ip_address: %s\n", ip_address);
    printf("port: %d\n", port);

    login();
    int selection = menu();

    switch (selection)
    {
        case 1:
            draw_field();
            break;

        case 2:
            draw_field();
            break;

        case 3:
            return 0;
            break;
    }

    return 0;
}