#include <stdlib.h>
#include "../include/sim_random.h"
#include "../include/game_state.h"

void population_init(Population* pop, int capacity)
{
    pop->capacity = capacity;
    pop->count = 0;
    pop->citizens = malloc(capacity * sizeof(Citizen));
}

void population_free(Population* pop)
{
    free(pop->citizens);
    pop->citizens = NULL;
    pop->count = 0;
    pop->capacity = 0;
}

void citizen_spawn(Population* pop)
{
    if (pop->count >= pop->capacity) return;

    Citizen* c = &pop->citizens[pop->count];
    c->age = 0;
    c->thirst = frand();
    c->wealth = frand() * 100.0f;
    c->homeless = 0;
    c->alive = 1;

    pop->count++;
}

void population_tick(struct World* w)
{
    Population* pop = &w->population;
    for (int i = 0; i < pop->count; i++) {
        Citizen* c = &pop->citizens[i];
        if (!c->alive) continue;
        c->age++;
    }
}

