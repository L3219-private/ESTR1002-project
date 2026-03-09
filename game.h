#ifndef GAME_H
#define GAME_H

#include "maze.h"

extern GameState CURRENT_STATE;
extern int MENU_INDEX;
extern bool LIMITED_SIGHT;

void game_update_tick();

#endif
