#ifndef POPULATION_H
#define POPULATION_H

#include "log.h"

typedef struct Citizen {
    int age;
    float thirst;
    float wealth;
    int homeless; /* bool */
    int alive;    /* bool */
} Citizen;

typedef struct Population {
    Citizen* citizens;
    int count;
    int capacity;
} Population;

struct World;

void population_init(Population* pop, int capacity);
void population_free(Population* pop);
void population_tick(struct World* w);
void citizen_spawn(Population* pop);

#endif
