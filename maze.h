#ifndef MAZE_H
#define MAZE_H

#include <stdbool.h>
#include <stdint.h>

#define CHUNK_W 31
#define CHUNK_H 15
#define VIEWPORT_W 21
#define VIEWPORT_H 11
#define EXIT_BFS_RADIUS 20
#define MIN_TERM_ROWS 24
#define MIN_TERM_COLS 80

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_WIN,
    STATE_LOSE
} GameState;

typedef enum {
    TILE_WALL,
    TILE_FLOOR,
    TILE_EXIT,
    TILE_SPIKE_OFF,
    TILE_SPIKE_ON,
    TILE_KEY,
    TILE_POWERUP,
    TILE_TELEPORT
} TileType;

typedef struct {
    int world_x;
    int world_y;
    int energy;
    bool has_key;
    int powerups;
    int moves;
} Player;

typedef struct {
    int x;
    int y;
    int on_interval;
    int off_interval;
    int timer;
    bool is_active;
} Spike;

typedef struct {
    int x;
    int y;
} Teleport;

typedef struct Chunk {
    int cx;
    int cy;
    TileType cells[CHUNK_H][CHUNK_W];
    bool generated;
    struct Chunk *next;
} Chunk;

typedef struct {
    bool infinite;
    Chunk *chunks;
    long long base_seed;
    bool exit_set;
    int exit_x;
    int exit_y;
    int exit_cx;
    int exit_cy;
} Maze;

typedef struct {
    int top_left_x;
    int top_left_y;
} Camera;

typedef struct {
    Spike *data;
    int count;
    int capacity;
} SpikeList;

typedef struct {
    Teleport *data;
    int count;
    int capacity;
} TeleportList;

typedef struct {
    int x;
    int y;
} Powerup;

typedef struct {
    Powerup *data;
    int count;
    int capacity;
} PowerupList;

extern Maze maze;
extern SpikeList spike_list;
extern TeleportList teleport_list;
extern PowerupList powerup_list;

void maze_init();
void maze_free();
TileType maze_get(int wx, int wy);
void maze_set(int wx, int wy, TileType type);
void maze_generate();
void update_spikes();

#endif
