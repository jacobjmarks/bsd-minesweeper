#ifndef CONSTANTS_H
#define CONSTANTS_H

#define NUM_TILES_X 9
#define NUM_TILES_Y 9

// GUI icons
#define MINE '*'
#define FLAG '+'

// Responses
#define TERMINATOR 'T'

#define DELIM ","

#define PACKET_SIZE 100

// enum { QUIT, LOSE, WIN, REVEAL_TILE, FLAG_TILE, LOGIN, LEADERBOARD, PLAY};
enum { LOGIN, PLAY, LEADERBOARD, QUIT, LOSE, WIN, REVEAL_TILE, FLAG_TILE};


#endif