/*
*
* town.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef TOWN_H
#define TOWN_H

#include "population.h"

typedef struct Town {
    /* Index of this town in Kingdom.towns[]; assigned at insertion, same
       pattern as Tavern.id (see town_add_tavern). */
    int id;

    Population population;

    struct Tavern* taverns;
    int tavern_count;
    int tavern_capacity;
    int player_tavern_id; /* only meaningful when this is the player's town */

    struct Merchant* merchants;
    int merchant_count;
    int merchant_capacity;

    int last_advertised_day;
} Town;

void town_taverns_init(Town* t, int capacity);
void town_taverns_free(Town* t);
/* Copies b into the pool and returns its index, or -1 if the pool is full */
int town_add_tavern(Town* t, struct Tavern b);

void town_merchants_init(Town* t, int capacity);
void town_merchants_free(Town* t);
/* Copies m into the pool and returns its index, or -1 if the pool is full */
int town_add_merchant(Town* t, struct Merchant m);

/* Re-point every tavern's supplier pointer at t->merchants[supplier_id].
   Call after loading a save, since the merchant pool is freshly malloc'd. */
void town_relink_suppliers(Town* t);

/* Frees population + taverns + merchants */
void town_free(Town* t);

#endif /* TOWN_H */
