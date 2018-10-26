#ifndef COMMON_H
#define COMMON_H

#define NUM_TILES_X 9
#define NUM_TILES_Y 9

#define TERMINATOR 'T'

#define ctoi(c) (c-'0')
#define itoc(i) (i+'0')
#define itoascii(i) (((int)i) - 65)

#define DEBUG 0

enum { LOGIN=1, PLAY, LEADERBOARD, QUIT, LOSE, WIN, REVEAL_TILE, FLAG_TILE, QUEUED };

int send_int(int fd, uint32_t message);
int recv_int(int fd, uint32_t* response);
int send_string(int fd, char* message);
int recv_string(int fd, char** response);

#endif