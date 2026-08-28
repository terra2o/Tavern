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

#define ANIMALS_DEFAULT_CAPACITY 16

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
void simulate_cats(Animals* animals);

/* Recomputes alive_count by scanning cats[]. Only needed after
   bulk-loading cats from a save, where alive_count wasn't tracked
   incrementally. */
void animals_recount_alive(Animals* animals);

#endif
