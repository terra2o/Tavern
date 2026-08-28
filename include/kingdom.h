/*
*
* kingdom.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef KINGDOM_H
#define KINGDOM_H

#include "town.h"
#include "game_state.h"

typedef struct Kingdom {
    /* Index of this kingdom in World.kingdoms[]; assigned at insertion,
       same pattern as Town.id/Tavern.id. */
    int id;

    int at_war;
    int our_kingdom_attack;
    int war_end_day;

    float inflation_rate;
    float money_supply_prev;

    Town* towns;
    int town_count;
    int town_capacity;
    int player_town_id; /* only meaningful when this is the player's kingdom */
} Kingdom;

void kingdom_towns_init(Kingdom* k, int capacity);
void kingdom_towns_free(Kingdom* k);
/* Copies t into the pool and returns its index, or -1 if the pool is full */
int kingdom_add_town(Kingdom* k, Town t);

/* Frees every town in the pool, then the pool itself */
void kingdom_free(Kingdom* k);

void world_kingdoms_init(World* w, int capacity);
void world_kingdoms_free(World* w);
/* Copies k into the pool and returns its index, or -1 if the pool is full */
int world_add_kingdom(World* w, Kingdom k);

/* Convenience lookups, since the player's tavern is now 4 hops deep
   (world -> kingdom -> town -> tavern). */
Kingdom* world_player_kingdom(World* w);
Town* world_player_town(World* w);
struct Tavern* world_player_tavern(World* w);

#endif /* KINGDOM_H */
