/*
*
* collect_game.c for "Tavern"
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <curses.h>
#include <string.h>

#include "../include/collect_game.h"
#include "../include/sim.h"
#include "../include/sim_random.h"

#define COLLECT_MIN_WIDTH 10
#define COLLECT_MIN_HEIGHT 6

/* Fruit doesn't show up right away, and not too often either. */
#define SPAWN_DELAY_MIN 60
#define SPAWN_DELAY_MAX 150

static int rand_range(int min, int max)
{
    return min + (int)(frand() * (max - min + 1));
}

void collect_state_start(CollectState* s, int max_x, int max_y)
{
    memset(s, 0, sizeof(*s));

    s->width = max_x / 2 - 2;
    s->height = max_y / 2 - 2;
    if (s->width < COLLECT_MIN_WIDTH) s->width = COLLECT_MIN_WIDTH;
    if (s->height < COLLECT_MIN_HEIGHT) s->height = COLLECT_MIN_HEIGHT;

    s->player_x = s->width / 2;
    s->player_y = s->height / 2;
    s->spawn_timer = rand_range(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
}

static int cell_is_free(const CollectState* s, int x, int y)
{
    if (x == s->player_x && y == s->player_y)
        return 0;

    for (int i = 0; i < s->spawned_total; i++) {
        if (s->fruits[i].active && s->fruits[i].x == x && s->fruits[i].y == y)
            return 0;
    }
    return 1;
}

static void spawn_fruit(CollectState* s)
{
    int x, y;
    int tries = 0;
    do {
        x = rand_range(0, s->width - 1);
        y = rand_range(0, s->height - 1);
        tries++;
    } while (!cell_is_free(s, x, y) && tries < 50);

    CollectFruit* f = &s->fruits[s->spawned_total];
    f->x = x;
    f->y = y;
    f->type = rand_range(0, FRUIT_COUNT - 1);
    f->active = 1;

    s->spawned_total++;
}

void collect_tick(CollectState* s)
{
    if (s->resolved)
        return;

    if (s->spawned_total >= COLLECT_MAX_FRUITS)
        return;

    s->spawn_timer--;
    if (s->spawn_timer <= 0) {
        spawn_fruit(s);
        s->spawn_timer = rand_range(SPAWN_DELAY_MIN, SPAWN_DELAY_MAX);
    }
}

static void try_collect(CollectState* s, Tavern* b)
{
    for (int i = 0; i < s->spawned_total; i++) {
        CollectFruit* f = &s->fruits[i];
        if (!f->active) continue;
        if (f->x != s->player_x || f->y != s->player_y) continue;

        f->active = 0;
        b->fruits[f->type].inventory.amount++;
        s->collected_count++;

        if (s->collected_count >= COLLECT_MAX_FRUITS)
            s->resolved = 1;
        break;
    }
}

void collect_handle_input(int ch, CollectState* s, Tavern* b)
{
    if (s->resolved)
        return;

    switch (ch) {
        case 'q':
        case 'Q':
        case 27: /* ESC */
            s->resolved = 1;
            return;

        case 'w':
        case 'W':
        case 'K':
        case 'k':
        case KEY_UP:
            if (s->player_y > 0) s->player_y--;
            break;

        case 's':
        case 'S':
        case 'j':
        case 'J':
        case KEY_DOWN:
            if (s->player_y < s->height - 1) s->player_y++;
            break;

        case 'a':
        case 'A':
        case 'h':
        case 'H':
        case KEY_LEFT:
            if (s->player_x > 0) s->player_x--;
            break;

        case 'd':
        case 'D':
        case 'L':
        case 'l':
        case KEY_RIGHT:
            if (s->player_x < s->width - 1) s->player_x++;
            break;

        default:
            return;
    }

    try_collect(s, b);
}
