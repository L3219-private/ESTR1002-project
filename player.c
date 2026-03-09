#include "player.h"
#include "maze.h"
#include <ncurses.h>
#include <stdbool.h>

extern GameState CURRENT_STATE;
extern bool LIMITED_SIGHT;

void init_new_game() {
    player.energy = 100;
    player.has_key = false;
    player.powerups = 0;
    player.moves = 0;
    player.world_x = 2;
    player.world_y = 2;
    maze_init();
    maze_generate();
}

void move_player(int dx, int dy) {
    int new_x = player.world_x + dx;
    int new_y = player.world_y + dy;
    TileType target = maze_get(new_x, new_y);
    if (target == TILE_WALL) return;
    if (target == TILE_SPIKE_ON) {
        player.energy = 0;
        return;
    }
    if (target == TILE_KEY) {
        player.has_key = true;
        maze_set(new_x, new_y, TILE_FLOOR);
    }
    if (target == TILE_POWERUP) {
        player.powerups++;
        maze_set(new_x, new_y, TILE_FLOOR);
    }
    if (target == TILE_TELEPORT) {
        // Find another teleport
        for (int i = 0; i < teleport_list.count; i++) {
            if (teleport_list.data[i].x != new_x || teleport_list.data[i].y != new_y) {
                player.world_x = teleport_list.data[i].x;
                player.world_y = teleport_list.data[i].y;
                player.energy--;
                player.moves++;
                return;
            }
        }
    }
    if (target == TILE_EXIT) {
        if (!player.has_key) return; // must have key to win
    }
    player.world_x = new_x;
    player.world_y = new_y;
    player.energy--;
    player.moves++;
}

void handle_input_playing(int ch) {
    if (CURRENT_STATE != STATE_PLAYING) return;
    if (ch == KEY_MOUSE) {
        MEVENT ev;
        if (getmouse(&ev) == OK && (ev.bstate & BUTTON1_CLICKED)) {
            int screen_w, screen_h;
            getmaxyx(stdscr, screen_h, screen_w);
            int view_w_cells = LIMITED_SIGHT ? 11 : screen_w / 2;
            int view_h_cells = LIMITED_SIGHT ? 11 : screen_h - 3;
            int cam_x = player.world_x - (view_w_cells / 2);
            int cam_y = player.world_y - (view_h_cells / 2);
            int cell_x = ev.x / 2;
            int cell_y = ev.y;
            if (cell_x >= 0 && cell_x < view_w_cells && cell_y >= 0 && cell_y < view_h_cells) {
                int target_wx = cam_x + cell_x;
                int target_wy = cam_y + cell_y;
                TileType t = maze_get(target_wx, target_wy);
                if (t == TILE_WALL && player.powerups > 0) {
                    player.powerups--;
                    maze_set(target_wx, target_wy, TILE_FLOOR);
                }
            }
        }
        return;
    }
    switch(ch) {
        case 'w': case 'W': case KEY_UP:    move_player(0, -1); break;
        case 's': case 'S': case KEY_DOWN:  move_player(0, 1); break;
        case 'a': case 'A': case KEY_LEFT:  move_player(-1, 0); break;
        case 'd': case 'D': case KEY_RIGHT: move_player(1, 0); break;
    }
}
