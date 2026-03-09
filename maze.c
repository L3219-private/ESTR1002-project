#include "maze.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

Maze maze;
SpikeList spike_list = {0};
TeleportList teleport_list = {0};
PowerupList powerup_list = {0};
long long last_spike_update = 0;

void* safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Fatal Error: Memory allocation failed.\n");
        exit(1);
    }
    return ptr;
}

void* safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "Fatal Error: Memory allocation failed.\n");
        exit(1);
    }
    return new_ptr;
}

long long current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

static unsigned int mix_chunk_seed(int cx, int cy, long long base_seed) {
    return (unsigned int)(base_seed ^ (cx * 73856093u) ^ (cy * 19349663u) ^ (cx * cy * 83492791u));
}

static unsigned int prng_next(unsigned int *state) {
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static int prng_range(unsigned int *state, int min, int max) {
    unsigned int r = prng_next(state);
    return min + (int)(r % (unsigned int)(max - min + 1));
}

static void world_to_chunk(int wx, int wy, int *cx, int *cy, int *lx, int *ly) {
    int mod_x = wx % CHUNK_W;
    if (mod_x < 0) mod_x += CHUNK_W;
    int mod_y = wy % CHUNK_H;
    if (mod_y < 0) mod_y += CHUNK_H;
    *lx = mod_x;
    *ly = mod_y;
    *cx = (wx - mod_x) / CHUNK_W;
    *cy = (wy - mod_y) / CHUNK_H;
}

static Chunk* find_chunk(int cx, int cy) {
    Chunk *cur = maze.chunks;
    while (cur) {
        if (cur->cx == cx && cur->cy == cy) return cur;
        cur = cur->next;
    }
    return NULL;
}

static Chunk* ensure_chunk(int cx, int cy) {
    Chunk *existing = find_chunk(cx, cy);
    if (existing) return existing;
    Chunk *chunk = safe_malloc(sizeof(Chunk));
    chunk->cx = cx;
    chunk->cy = cy;
    chunk->generated = false;
    chunk->next = maze.chunks;
    maze.chunks = chunk;
    return chunk;
}

static void ensure_spike_capacity(int extra) {
    int needed = spike_list.count + extra;
    if (spike_list.capacity >= needed) return;
    int new_cap = spike_list.capacity ? spike_list.capacity * 2 : 16;
    while (new_cap < needed) new_cap *= 2;
    spike_list.data = safe_realloc(spike_list.data, new_cap * sizeof(Spike));
    spike_list.capacity = new_cap;
}

static void ensure_teleport_capacity(int extra) {
    int needed = teleport_list.count + extra;
    if (teleport_list.capacity >= needed) return;
    int new_cap = teleport_list.capacity ? teleport_list.capacity * 2 : 8;
    while (new_cap < needed) new_cap *= 2;
    teleport_list.data = safe_realloc(teleport_list.data, new_cap * sizeof(Teleport));
    teleport_list.capacity = new_cap;
}

static void ensure_powerup_capacity(int extra) {
    int needed = powerup_list.count + extra;
    if (powerup_list.capacity >= needed) return;
    int new_cap = powerup_list.capacity ? powerup_list.capacity * 2 : 8;
    while (new_cap < needed) new_cap *= 2;
    powerup_list.data = safe_realloc(powerup_list.data, new_cap * sizeof(Powerup));
    powerup_list.capacity = new_cap;
}

static bool walkable_for_exit(TileType t) {
    return t != TILE_WALL && t != TILE_SPIKE_ON;
}

static void generate_chunk(Chunk *chunk) {
    if (chunk->generated) return;
    unsigned int state = mix_chunk_seed(chunk->cx, chunk->cy, maze.base_seed);
    int gw = CHUNK_W + 2;
    int gh = CHUNK_H + 2;
    TileType temp[CHUNK_H + 2][CHUNK_W + 2];
    for (int y = 0; y < gh; y++) for (int x = 0; x < gw; x++) temp[y][x] = TILE_WALL;
    int stack_size = gw * gh;
    int *stack_x = safe_malloc(stack_size * sizeof(int));
    int *stack_y = safe_malloc(stack_size * sizeof(int));
    int stack_top = 0;
    int directions[4][2] = {{0, -2}, {2, 0}, {0, 2}, {-2, 0}};
    bool visited[CHUNK_H + 2][CHUNK_W + 2];
    for (int y = 0; y < gh; y++) for (int x = 0; x < gw; x++) visited[y][x] = false;
    int start_x = 1, start_y = 1;
    temp[start_y][start_x] = TILE_FLOOR;
    visited[start_y][start_x] = true;
    stack_x[stack_top] = start_x;
    stack_y[stack_top] = start_y;
    stack_top++;
    while (stack_top > 0) {
        stack_top--;
        int cx = stack_x[stack_top];
        int cy = stack_y[stack_top];
        int dir_order[4] = {0, 1, 2, 3};
        for (int i = 0; i < 4; i++) {
            int j = prng_range(&state, 0, 3);
            int tmp = dir_order[i];
            dir_order[i] = dir_order[j];
            dir_order[j] = tmp;
        }
        bool carved = false;
        for (int d = 0; d < 4; d++) {
            int dir = dir_order[d];
            int nx = cx + directions[dir][0];
            int ny = cy + directions[dir][1];
            if (nx >= 1 && nx < gw - 1 && ny >= 1 && ny < gh - 1) {
                if (!visited[ny][nx]) {
                    int wall_x = cx + directions[dir][0] / 2;
                    int wall_y = cy + directions[dir][1] / 2;
                    temp[wall_y][wall_x] = TILE_FLOOR;
                    temp[ny][nx] = TILE_FLOOR;
                    visited[ny][nx] = true;
                    visited[wall_y][wall_x] = true;
                    stack_x[stack_top] = nx;
                    stack_y[stack_top] = ny;
                    stack_top++;
                    stack_x[stack_top] = cx;
                    stack_y[stack_top] = cy;
                    stack_top++;
                    carved = true;
                    break;
                }
            }
        }
        if (!carved) continue;
    }
    free(stack_x);
    free(stack_y);
    int center_x = CHUNK_W / 2;
    int center_y = CHUNK_H / 2;
    temp[center_y + 1][center_x + 1] = TILE_FLOOR;
    for (int y = 0; y < CHUNK_H; y++) for (int x = 0; x < CHUNK_W; x++) chunk->cells[y][x] = temp[y + 1][x + 1];
    int spawn_x = 2, spawn_y = 2;
    int exit_x = -1, exit_y = -1;
    if (!maze.exit_set && chunk->cx == maze.exit_cx && chunk->cy == maze.exit_cy) {
        for (int y = 0; y < CHUNK_H; y++) for (int x = 0; x < CHUNK_W; x++) {
            int dx = x - spawn_x, dy = y - spawn_y;
            if (dx*dx + dy*dy <= 25 && chunk->cells[y][x] == TILE_FLOOR) { exit_x = x; exit_y = y; break; }
        }
        if (exit_x == -1) { exit_x = spawn_x; exit_y = spawn_y + 1; }
        chunk->cells[exit_y][exit_x] = TILE_EXIT;
        maze.exit_set = true;
        maze.exit_x = chunk->cx * CHUNK_W + exit_x;
        maze.exit_y = chunk->cy * CHUNK_H + exit_y;
    }
    if (chunk->cx == 0 && chunk->cy == 0) {
        int key_x = spawn_x, key_y = spawn_y;
        for (int attempt = 0; attempt < 200; attempt++) {
            int tx = prng_range(&state, 0, CHUNK_W - 1);
            int ty = prng_range(&state, 0, CHUNK_H - 1);
            int dx = tx - spawn_x, dy = ty - spawn_y;
            if (dx*dx + dy*dy > 25) continue;
            if (chunk->cells[ty][tx] != TILE_FLOOR) continue;
            if (maze.exit_set && chunk->cx == maze.exit_cx && chunk->cy == maze.exit_cy && tx == exit_x && ty == exit_y) continue;
            key_x = tx; key_y = ty; break;
        }
        chunk->cells[key_y][key_x] = TILE_KEY;
    }
    int local_teleports = prng_range(&state, 2, 4);
    ensure_teleport_capacity(local_teleports);
    for (int i = 0; i < local_teleports; i++) {
        int tx, ty, tries = 0;
        do { tx = prng_range(&state, 1, CHUNK_W - 2); ty = prng_range(&state, 1, CHUNK_H - 2); tries++; } while (tries < 100 && chunk->cells[ty][tx] != TILE_FLOOR);
        chunk->cells[ty][tx] = TILE_TELEPORT;
        Teleport tp = { .x = chunk->cx * CHUNK_W + tx, .y = chunk->cy * CHUNK_H + ty };
        teleport_list.data[teleport_list.count++] = tp;
    }
    int local_spikes = prng_range(&state, 5, 10);
    ensure_spike_capacity(local_spikes);
    for (int i = 0; i < local_spikes; i++) {
        int sx, sy, tries = 0;
        do { sx = prng_range(&state, 1, CHUNK_W - 2); sy = prng_range(&state, 1, CHUNK_H - 2); tries++; } while (tries < 100 && chunk->cells[sy][sx] != TILE_FLOOR);
        chunk->cells[sy][sx] = TILE_SPIKE_OFF;
        Spike sp = { .x = chunk->cx * CHUNK_W + sx, .y = chunk->cy * CHUNK_H + sy, .on_interval = 2000 + prng_range(&state, 0, 3000), .off_interval = 1000 + prng_range(&state, 0, 2000), .timer = 0, .is_active = false };
        spike_list.data[spike_list.count++] = sp;
    }
    int local_powerups = prng_range(&state, 0, 2);
    ensure_powerup_capacity(local_powerups);
    for (int i = 0; i < local_powerups; i++) {
        int px, py, tries = 0;
        do { px = prng_range(&state, 1, CHUNK_W - 2); py = prng_range(&state, 1, CHUNK_H - 2); tries++; } while (tries < 100 && chunk->cells[py][px] != TILE_FLOOR);
        if (chunk->cells[py][px] != TILE_FLOOR) continue;
        chunk->cells[py][px] = TILE_POWERUP;
        Powerup pu = { .x = chunk->cx * CHUNK_W + px, .y = chunk->cy * CHUNK_H + py };
        powerup_list.data[powerup_list.count++] = pu;
    }
    chunk->generated = true;
}

void maze_init() {
    maze.infinite = true;
    maze.chunks = NULL;
    maze.base_seed = current_time_ms();
    maze.exit_set = false;
    maze.exit_cx = 0;
    maze.exit_cy = 0;
}

void maze_free() {
    Chunk *cur = maze.chunks;
    while (cur) {
        Chunk *next = cur->next;
        free(cur);
        cur = next;
    }
    maze.chunks = NULL;
    if (spike_list.data) { free(spike_list.data); spike_list.data = NULL; }
    spike_list.count = spike_list.capacity = 0;
    if (teleport_list.data) { free(teleport_list.data); teleport_list.data = NULL; }
    teleport_list.count = teleport_list.capacity = 0;
    if (powerup_list.data) { free(powerup_list.data); powerup_list.data = NULL; }
    powerup_list.count = powerup_list.capacity = 0;
}

TileType maze_get(int wx, int wy) {
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *chunk = ensure_chunk(cx, cy);
    generate_chunk(chunk);
    return chunk->cells[ly][lx];
}

void maze_set(int wx, int wy, TileType type) {
    int cx, cy, lx, ly;
    world_to_chunk(wx, wy, &cx, &cy, &lx, &ly);
    Chunk *chunk = ensure_chunk(cx, cy);
    generate_chunk(chunk);
    chunk->cells[ly][lx] = type;
}

void maze_generate() {
    Chunk *origin = ensure_chunk(0, 0);
    generate_chunk(origin);
    last_spike_update = current_time_ms();
}

