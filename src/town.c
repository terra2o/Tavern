/*
*
* town.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdlib.h>
#include "../include/town.h"
#include "../include/sim.h"
#include "../include/merchant.h"

void town_taverns_init(Town* t, int capacity)
{
    /* idempotent: safe to call again to reset the pool, so free whatever
       was already there (including each tavern's own animals) first */
    for (int i = 0; i < t->tavern_count; i++)
        animals_free(&t->taverns[i].cats);
    free(t->taverns);
    t->tavern_capacity = capacity;
    t->tavern_count = 0;
    t->taverns = malloc(capacity * sizeof(Tavern));
}

void town_taverns_free(Town* t)
{
    for (int i = 0; i < t->tavern_count; i++)
        animals_free(&t->taverns[i].cats);
    free(t->taverns);
    t->taverns = NULL;
    t->tavern_count = 0;
    t->tavern_capacity = 0;
}

int town_add_tavern(Town* t, Tavern b)
{
    if (t->tavern_count >= t->tavern_capacity) return -1;
    b.id = t->tavern_count;
    t->taverns[t->tavern_count] = b;
    return t->tavern_count++;
}

void town_merchants_init(Town* t, int capacity)
{
    free(t->merchants); /* idempotent: safe to call again to reset the pool */
    t->merchant_capacity = capacity;
    t->merchant_count = 0;
    t->merchants = malloc(capacity * sizeof(Merchant));
}

void town_merchants_free(Town* t)
{
    free(t->merchants);
    t->merchants = NULL;
    t->merchant_count = 0;
    t->merchant_capacity = 0;
}

int town_add_merchant(Town* t, Merchant m)
{
    if (t->merchant_count >= t->merchant_capacity) return -1;
    t->merchants[t->merchant_count] = m;
    return t->merchant_count++;
}

void town_relink_suppliers(Town* t)
{
    int i;
    Tavern* b;

    for (i = 0; i < t->tavern_count; i++) {
        b = &t->taverns[i];
        b->supplier = (b->supplier_id >= 0 && b->supplier_id < t->merchant_count)
            ? &t->merchants[b->supplier_id]
            : NULL;
    }
}

void town_free(Town* t)
{
    population_free(&t->population);
    town_taverns_free(t);
    town_merchants_free(t);
}
