#include "render.h"
#include "game.h"
#include "player.h"
#include "maze.h"
#include <ncurses.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
int mvaddnwstr(int y, int x, const wchar_t *wstr, int n);
#ifdef __cplusplus
}
#endif

void render_init() {
    setlocale(LC_ALL, "");
    initscr();
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < MIN_TERM_ROWS || cols < MIN_TERM_COLS) {
        endwin();
        printf("Terminal too small. Please resize to at least 80x24.\n");
        exit(1);
    }
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    mousemask(BUTTON1_CLICKED, NULL);
    timeout(200);
    if (has_colors()) start_color();
}

void render_cleanup() {
    endwin();
}

const wchar_t* get_tile_glyph(TileType t) {
    switch(t) {
        case TILE_WALL: return L"＃";
        case TILE_FLOOR: return L"　";
        case TILE_EXIT: return L"ｘ";
        case TILE_SPIKE_OFF: return L"　";
        case TILE_SPIKE_ON: return L"ｗ";
        case TILE_KEY: return L"Ｋ";
        case TILE_POWERUP: return L"Ｐ";
        case TILE_TELEPORT: return L"＊";
        default: return L"？";
    }
}

void draw_title() {
    const char *items[3] = {
        "Start Game",
        "Limited Sight: ",
        "Exit"
    };
    clear();
    mvprintw(3, 8, "MAZE GAME - Enhanced Version");
    mvprintw(5, 8, "Use W/S to navigate, ENTER to select, ESC to quit");
    for (int i = 0; i < 3; i++) {
        int y = 7 + i * 2;
        if (i == MENU_INDEX) attron(A_REVERSE);
        if (i == 1) {
            mvprintw(y, 8, "%s%s", items[i], LIMITED_SIGHT ? "ON" : "OFF");
        } else {
            mvprintw(y, 8, "%s", items[i]);
        }
        if (i == MENU_INDEX) attroff(A_REVERSE);
    }
}

void draw_game() {
    int screen_w, screen_h;
    getmaxyx(stdscr, screen_h, screen_w);
    int view_w_cells = LIMITED_SIGHT ? 11 : screen_w / 2;
    int view_h_cells = LIMITED_SIGHT ? 11 : screen_h - 3;
    int view_h = view_h_cells;
    int view_w = view_w_cells;
    int cam_x = player.world_x - (view_w / 2);
    int cam_y = player.world_y - (view_h / 2);
    const wchar_t *player_glyph = L"ｏ";
    for(int y = 0; y < view_h; y++) {
        for(int x = 0; x < view_w; x++) {
            int world_x = cam_x + x;
            int world_y = cam_y + y;
            int screen_x = x * 2;
            if(screen_x >= screen_w - 1) continue;
            if(world_x == player.world_x && world_y == player.world_y) {
                attron(A_BOLD);
                mvaddnwstr(y, screen_x, player_glyph, 1);
                attroff(A_BOLD);
            } else {
                TileType t = maze_get(world_x, world_y);
                const wchar_t* glyph = get_tile_glyph(t);
                mvaddnwstr(y, screen_x, glyph, 1);
            }
        }
    }
    mvhline(view_h, 0, '-', screen_w);
    mvprintw(view_h + 1, 0, "Energy: %d  Pos: %d,%d  Moves: %d", player.energy, player.world_x, player.world_y, player.moves);
    int status_y1 = view_h + 1;
    int status_y2 = view_h + 2;
    mvprintw(status_y1, 40, "[KEY: %s]", player.has_key ? "YES" : "NO");
    mvprintw(status_y1, 60, "[P: %d]", player.powerups);
    mvprintw(status_y2, 0, "Traps: %d  Teleports: %d", spike_list.count, teleport_list.count);
}

void draw_win() {
    clear();
    mvprintw(10, 10, "YOU WIN! Press ESC.");
    mvprintw(11, 10, "Moves: %d, Energy left: %d", player.moves, player.energy);
}

void draw_lose() {
    clear();
    mvprintw(10, 10, "GAME OVER. Press ESC.");
}

void render_all() {
    erase();
    switch(CURRENT_STATE) {
        case STATE_TITLE:
            draw_title();
            break;
        case STATE_PLAYING:
            draw_game();
            break;
        case STATE_WIN:
            draw_win();
            break;
        case STATE_LOSE:
            draw_lose();
            break;
    }
    refresh();
}
