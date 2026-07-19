#ifndef POPULATION_H
#define POPULATION_H

#include "log.h"
#include "drink.h"

typedef struct Citizen {
    int age;
    float thirst;
    float wealth;
    float addiction;     /* 0.0 to 1.0 */
    float income;        /* earned per day, replenishes wealth */
    float loyalty;       /* 0.0 to 1.0, attachment to favorite_tavern_id */
    int last_drink_day;  /* day of last visit, -1 if never */
    int favorite_tavern_id; /* index into World.taverns, -1 if none yet */
    float drink_preference[DRINK_COUNT]; /* affinity per drink */
    float health;        /* 0.0 to 1.0 */
    int homeless; /* bool */
    int alive;    /* bool */
} Citizen;

typedef struct Population {
    Citizen* citizens;
    int count;        /* total ever spawned; also the valid range of citizens[] */
    int alive_count;  /* currently alive, use this for anything population-size-facing */
    int capacity;
} Population;

struct World;

void population_init(Population* pop, int capacity);
void population_free(Population* pop);
void population_tick(struct World* w);
void citizen_spawn(Population* pop);

/* Recomputes alive_count by scanning citizens[]. Only needed after
   bulk-loading citizens from a save, where alive_count wasn't tracked
   incrementally. */
void population_recount_alive(Population* pop);

/* Averages thirst and addiction across every alive citizen. Both are
   left at 0.0 if nobody's alive. */
void population_stats(const Population* pop, float* avg_thirst, float* avg_addiction);

#endif
