/*
*
* kingdom.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdlib.h>
#include "../include/kingdom.h"
#include "../include/sim.h"

void kingdom_towns_init(Kingdom* k, int capacity)
{
    int i;

    /* idempotent: safe to call again to reset the pool */
    for (i = 0; i < k->town_count; i++)
        town_free(&k->towns[i]);
    free(k->towns);
    k->town_capacity = capacity;
    k->town_count = 0;
    k->towns = malloc(capacity * sizeof(Town));
}

void kingdom_towns_free(Kingdom* k)
{
    int i;

    for (i = 0; i < k->town_count; i++)
        town_free(&k->towns[i]);
    free(k->towns);
    k->towns = NULL;
    k->town_count = 0;
    k->town_capacity = 0;
}

int kingdom_add_town(Kingdom* k, Town t)
{
    if (k->town_count >= k->town_capacity) return -1;
    t.id = k->town_count;
    k->towns[k->town_count] = t;
    return k->town_count++;
}

void kingdom_free(Kingdom* k)
{
    kingdom_towns_free(k);
}

void world_kingdoms_init(World* w, int capacity)
{
    int i;

    /* idempotent: safe to call again to reset the pool */
    for (i = 0; i < w->kingdom_count; i++)
        kingdom_free(&w->kingdoms[i]);
    free(w->kingdoms);
    w->kingdom_capacity = capacity;
    w->kingdom_count = 0;
    w->kingdoms = malloc(capacity * sizeof(Kingdom));
}

void world_kingdoms_free(World* w)
{
    int i;

    for (i = 0; i < w->kingdom_count; i++)
        kingdom_free(&w->kingdoms[i]);
    free(w->kingdoms);
    w->kingdoms = NULL;
    w->kingdom_count = 0;
    w->kingdom_capacity = 0;
}

int world_add_kingdom(World* w, Kingdom k)
{
    if (w->kingdom_count >= w->kingdom_capacity) return -1;
    k.id = w->kingdom_count;
    w->kingdoms[w->kingdom_count] = k;
    return w->kingdom_count++;
}

Kingdom* world_player_kingdom(World* w)
{
    return &w->kingdoms[w->player_kingdom_id];
}

Town* world_player_town(World* w)
{
    Kingdom* k = world_player_kingdom(w);
    return &k->towns[k->player_town_id];
}

struct Tavern* world_player_tavern(World* w)
{
    Town* t = world_player_town(w);
    return &t->taverns[t->player_tavern_id];
}
