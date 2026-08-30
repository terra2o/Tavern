/*
*
* animals.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef ANIMALS_H
#define ANIMALS_H

#include "log.h"

/* count (total ever spawned, dead or alive) never shrinks and never
   reallocates past this, so it needs real headroom now that cats
   reproduce instead of trickling in one at a time. */
#define ANIMALS_DEFAULT_CAPACITY 64

typedef struct Cat {
    int age;
    float thirst;
    float curiosity;
    int drunk;           /* bool */
    float health;
    int alive;           /* bool */
    int male;            /* bool */
} Cat;

/* i know, humans are animals too. but you get it, right? */
typedef struct Animals {
    Cat* cats;
    int count;        /* total ever spawned; also the valid range of animals[] */
    int alive_count;  /* currently alive, use this for anything population-size-facing */
    int capacity;
} Animals;

void animals_init(Animals* animals, int capacity);
void animals_free(Animals* animals);
void cat_spawn(Animals* animals);

/* One day of cat lifecycle: aging/thirst growth/sobering up/old-age
   death for every cat already alive, plus a chance of new kittens from
   every mature male/female pair in the pool. Doesn't know about taverns
   at all - same split as population_tick vs market_simulate_all, see
   cats_visit_taverns() (static, in sim.c) for the tavern-visiting half. */
void cats_tick(Animals* animals, MessageLog* log);

/* Recomputes alive_count by scanning cats[]. Only needed after
   bulk-loading cats from a save, where alive_count wasn't tracked
   incrementally. */
void animals_recount_alive(Animals* animals);

/* Counts of alive/drunk cats, for the left status panel. */
void animals_stats(const Animals* animals, int* alive_count, int* drunk_count);

#endif
