#include "game.h"   // game state and tick
#include "render.h" // rendering
#include "player.h" // player logic
#include "maze.h"   // maze logic
#include <ncurses.h>
#include <stdlib.h>

// The current state of the game
GameState CURRENT_STATE = STATE_TITLE;
Player player; // the player struct

// Go back to title screen
static void reset_to_title() {
    CURRENT_STATE = STATE_TITLE;
}

// Start a new game
static void start_new_game() {
    init_new_game();
    CURRENT_STATE = STATE_PLAYING;
}

// Handle input in the title menu
void handle_input_title(int ch) {
    if (ch == 27) { // ESC to quit
        render_cleanup();
        exit(0);
    } else if (ch == 'w' || ch == 'W' || ch == KEY_UP) {
        MENU_INDEX = (MENU_INDEX + 2) % 3; // up
    } else if (ch == 's' || ch == 'S' || ch == KEY_DOWN) {
        MENU_INDEX = (MENU_INDEX + 1) % 3; // down
    } else if (ch == '\n' || ch == KEY_ENTER || ch == 10) {
        if (MENU_INDEX == 0) {
            start_new_game();
        } else if (MENU_INDEX == 1) {
            LIMITED_SIGHT = !LIMITED_SIGHT; // toggle sight
        } else if (MENU_INDEX == 2) {
            render_cleanup();
            exit(0);
        }
    }
}

// Main game tick: handle input and state
void game_update_tick() {
    int ch = getch(); // get key
    switch (CURRENT_STATE) {
        case STATE_TITLE:
            handle_input_title(ch);
            break;
        case STATE_PLAYING:
            if (ch == 27) { // ESC to title
                reset_to_title();
                break;
            } else if (ch == 'q' || ch == 'Q') {
                render_cleanup();
                exit(0);
            }
            handle_input_playing(ch); // move player etc
            if (player.energy <= 0) {
                CURRENT_STATE = STATE_LOSE;
            } else if (maze_get(player.world_x, player.world_y) == TILE_EXIT) {
                CURRENT_STATE = STATE_WIN;
            }
            break;
        case STATE_WIN:
        case STATE_LOSE:
            if (ch == 27) {
                reset_to_title();
            } else if (ch == '\n' || ch == KEY_ENTER) {
                start_new_game();
            } else if (ch == 'q' || ch == 'Q') {
                render_cleanup();
                exit(0);
            }
            break;
        default:
            reset_to_title();
            break;
    }
}
