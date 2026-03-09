#include "render.h" // for rendering functions
#include "game.h"    // for game logic

// These are for menu and sight mode
int MENU_INDEX = 0;
bool LIMITED_SIGHT = true;

int main() {
    // Initialize the screen
    render_init();
    // Main game loop
    while (1) {
        game_update_tick(); // update game state
        render_all();      // draw everything
    }
    // Clean up ncurses
    render_cleanup();
    return 0; // program finished
}
