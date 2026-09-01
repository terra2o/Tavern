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
#include <stdio.h>
#include "../include/sim_random.h"
#include "../include/animals.h"
#include "../include/sim.h" /* for the CLAMP macro */

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
    Cat* c;

    if (animals->count >= animals->capacity) return;

    c = &animals->cats[animals->count];
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
    int i;

    for (i = 0; i < animals->count; i++)
        if (animals->cats[i].alive) alive++;
    animals->alive_count = alive;
}

void animals_stats(const Animals* animals, int* alive_count, int* drunk_count)
{
    int alive = 0;
    int drunk = 0;
    int i;

    for (i = 0; i < animals->count; i++) {
        const Cat* c = &animals->cats[i];
        if (!c->alive) continue;
        alive++;
        if (c->drunk) drunk++;
    }

    *alive_count = alive;
    *drunk_count = drunk;
}

#define CAT_THIRST_GROWTH_PER_DAY 0.10f
#define CAT_SOBER_UP_CHANCE 0.1f        /* daily chance a drunk cat sobers back up */
#define CAT_OLD_AGE_THRESHOLD_DAYS 2000
#define CAT_OLD_AGE_DEATH_CHANCE_PER_DAY 0.001f
/* A cat can't have kittens until it's grown up a bit. */
#define CAT_MATURITY_AGE_DAYS 90
/* Rolled once per mature male/female pair in town, so more cats means
   more (not faster) litters - growth naturally slows once cat_spawn
   starts hitting Animals.capacity. */
#define CAT_LITTER_CHANCE_PER_DAY 0.05f

void cats_tick(Animals* a, MessageLog* log)
{
    int deaths = 0;
    int mature_males = 0;
    int mature_females = 0;
    int fertile_pairs;
    int births = 0;
    int i;

    for (i = 0; i < a->count; i++) {
        Cat* c = &a->cats[i];
        if (!c->alive) continue;
        c->age++;

        if (c->drunk && frand() < CAT_SOBER_UP_CHANCE) c->drunk = 0;

        c->thirst = CLAMP(c->thirst + CAT_THIRST_GROWTH_PER_DAY, 0.0f, 1.0f);

        if (c->age > CAT_OLD_AGE_THRESHOLD_DAYS) {
            float death_chance = CLAMP((c->age - CAT_OLD_AGE_THRESHOLD_DAYS) * CAT_OLD_AGE_DEATH_CHANCE_PER_DAY, 0.0f, 1.0f);
            if (frand() < death_chance) {
                c->alive = 0;
                a->alive_count--;
                deaths++;
                continue;
            }
        }

        if (c->age >= CAT_MATURITY_AGE_DAYS) {
            if (c->male) mature_males++;
            else mature_females++;
        }
    }

    /* One shot at a litter per fertile pair - if there are 3 mature
       toms and 5 mature queens, that's 3 pairs, 3 independent rolls. */
    fertile_pairs = mature_males < mature_females ? mature_males : mature_females;
    for (i = 0; i < fertile_pairs; i++) {
        if (frand() < CAT_LITTER_CHANCE_PER_DAY) {
            cat_spawn(a);
            births++;
        }
    }

    if (births > 0) {
        char buf[128];
        tavern_snprintf(buf, sizeof(buf), "%d new kitten%s born.", births, births == 1 ? " was" : "s were");
        log_message(log, buf, LOG_INFO);
    }
    if (deaths > 0) {
        char buf[128];
        tavern_snprintf(buf, sizeof(buf), "%d cats died of old age.", deaths);
        log_message(log, buf, LOG_INFO);
    }
}
