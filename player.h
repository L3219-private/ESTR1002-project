#ifndef PLAYER_H
#define PLAYER_H

#include "maze.h"

extern Player player;

void init_new_game();
void move_player(int dx, int dy);
void handle_input_playing(int ch);

#endif
