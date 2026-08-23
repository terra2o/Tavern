/*
*
* collect_game.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef COLLECT_GAME_H
#define COLLECT_GAME_H

#include "fruit.h"

struct Tavern;

#define COLLECT_MAX_FRUITS 5   /* total fruits per game */
#define COLLECT_MAX_ACTIVE 16  /* array size, headroom above what's ever active at once */

typedef struct {
    int x, y;
    FruitType type;
    int active;
} CollectFruit;

typedef struct {
    int player_x, player_y;
    int width, height;          /* interior play area size */
    CollectFruit fruits[COLLECT_MAX_ACTIVE];
    int spawned_total;          /* how many of COLLECT_MAX_FRUITS have appeared so far, also the fruits[] count */
    int collected_count;
    int spawn_timer;
    int resolved;
} CollectState;

/* Set up a fresh game. max_x/max_y are the current terminal size, the
   play area is sized off of them. */
void collect_state_start(CollectState* s, int max_x, int max_y);

/* Called once per frame, spawns fruit over time. */
void collect_tick(CollectState* s);

/* Handle a single keypress: movement, quitting, picking up fruit. */
void collect_handle_input(int ch, CollectState* s, struct Tavern* b);

#endif
