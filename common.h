#ifndef COMMON_H
#define COMMON_H

#define NUM_TILES_X 9
#define NUM_TILES_Y 9

#define PACKET_SIZE 100
#define TERMINATOR 'T'

#define ctoi(c) (c-'0')
#define itoc(i) (i+'0')
#define itoascii(i) (((int)i) - 65)

#define DEBUG 1

enum { PLAY=1, LEADERBOARD, QUIT, LOGIN, LOSE, WIN, REVEAL_TILE, FLAG_TILE };

#endif