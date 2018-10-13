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

enum { LOGIN, PLAY, LEADERBOARD, QUIT, LOSE, WIN, REVEAL_TILE, FLAG_TILE, QUEUED };

// 0 or 1
int send_string(int fd, char* message);
// 0 or 1
int recv_string(int fd, char** response);
// 0 or 1
int send_int(int fd, int message);
// 0 or 1
int recv_int(int fd, int* response);

#endif