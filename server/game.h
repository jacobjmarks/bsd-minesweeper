#ifndef GAME_H
#define GAME_H

#include "structs.h"

#define NUM_MINES 10

void play_game(ClientSession_t*);
HighScore_t* get_highscore(char*);
void stream_leaderboard(int);

#endif