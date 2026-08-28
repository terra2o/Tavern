/*
*
* animals.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdlib.h>
#include "../include/sim_random.h"
#include "../include/animals.h"

void animals_init(Animals* animals, int capacity)
{
    animals->capacity = capacity;
    animals->count = 0;
    animals->alive_count = 0;
    animals->cats = malloc(capacity * sizeof(Cat));
}

void animals_free(Animals* animals)
{
    free(animals->cats);
    animals->cats = NULL;
    animals->count = 0;
    animals->alive_count = 0;
    animals->capacity = 0;
}

void cat_spawn(Animals* animals)
{
    if (animals->count >= animals->capacity) return;

    Cat* c = &animals->cats[animals->count];
    c->age = 0;
    c->thirst = frand();
    c->curiosity = 0.5f;
    c->drunk = 0;
    c->health = 1.0f;
    c->alive = 1;
    if (rand() % 2 == 0)
        c->male = 0;
    else
        c->male = 1;

    animals->count++;
    animals->alive_count++;
}

void animals_recount_alive(Animals* animals)
{
    int alive = 0;
    for (int i = 0; i < animals->count; i++)
        if (animals->cats[i].alive) alive++;
    animals->alive_count = alive;
}

void simulate_cats(Animals* animals)
{
    int i = 0;
    for (i = 0; i <= animals->alive_count; i++) {
    }
}
