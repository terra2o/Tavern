#include <stdlib.h>
#include <stdio.h>
#include "../include/sim_random.h"
#include "../include/game_state.h"
#include "../include/sim.h"

void population_init(Population* pop, int capacity)
{
    pop->capacity = capacity;
    pop->count = 0;
    pop->alive_count = 0;
    pop->citizens = malloc(capacity * sizeof(Citizen));
}

void population_free(Population* pop)
{
    free(pop->citizens);
    pop->citizens = NULL;
    pop->count = 0;
    pop->alive_count = 0;
    pop->capacity = 0;
}

void population_recount_alive(Population* pop)
{
    int alive = 0;
    for (int i = 0; i < pop->count; i++)
        if (pop->citizens[i].alive) alive++;
    pop->alive_count = alive;
}

void population_stats(const Population* pop, float* avg_thirst, float* avg_addiction)
{
    float thirst_sum = 0.0f;
    float addiction_sum = 0.0f;
    int alive = 0;

    for (int i = 0; i < pop->count; i++) {
        const Citizen* c = &pop->citizens[i];
        if (!c->alive) continue;
        thirst_sum += c->thirst;
        addiction_sum += c->addiction;
        alive++;
    }

    *avg_thirst = alive > 0 ? thirst_sum / alive : 0.0f;
    *avg_addiction = alive > 0 ? addiction_sum / alive : 0.0f;
}

/* Most citizens are working-class, but a minority spawn well-off, with
   both more starting wealth and a higher income. Without this tail,
   nobody in town could ever plausibly afford something like wine. */
#define WEALTHY_SPAWN_CHANCE 0.1f
#define WEALTHY_WEALTH_BONUS_MAX 400.0f
#define WEALTHY_INCOME_BONUS_MAX 20.0f

void citizen_spawn(Population* pop)
{
    if (pop->count >= pop->capacity) return;

    Citizen* c = &pop->citizens[pop->count];
    c->age = 0;
    c->thirst = frand();
    c->wealth = frand() * 100.0f;
    c->addiction = 0.0f;
    c->income = frand() * 10.0f;
    if (frand() < WEALTHY_SPAWN_CHANCE) {
        c->wealth += frand() * WEALTHY_WEALTH_BONUS_MAX;
        c->income += frand() * WEALTHY_INCOME_BONUS_MAX;
    }
    c->loyalty = 0.0f;
    c->last_drink_day = -1;
    c->favorite_tavern_id = -1;
    for (int d = 0; d < DRINK_COUNT; d++)
        c->drink_preference[d] = frand();
    c->health = 1.0f;
    c->homeless = 0;
    c->alive = 1;

    pop->count++;
    pop->alive_count++;
}

#define THIRST_GROWTH_PER_DAY 0.08f
#define ADDICTION_THIRST_BOOST 0.05f /* extra daily thirst growth at max addiction */

/* Health erodes for anyone drinking heavily, recovers slowly otherwise */
#define HEALTHY_ADDICTION_THRESHOLD 0.1f
#define HEALTH_DECAY_PER_ADDICTION 0.01f
#define HEALTH_RECOVERY_PER_DAY 0.005f

/* Past this many days old, a small and growing chance of natural death kicks in */
#define OLD_AGE_THRESHOLD_DAYS 3000
#define OLD_AGE_DEATH_CHANCE_PER_DAY 0.001f

void population_tick(struct World* w)
{
    Population* pop = &w->population;
    int deaths_health = 0;
    int deaths_age = 0;

    for (int i = 0; i < pop->count; i++) {
        Citizen* c = &pop->citizens[i];
        if (!c->alive) continue;
        c->age++;

        /* Thirst builds up daily; the market pass resets it for
           whoever actually visits a tavern that day. */
        float growth = THIRST_GROWTH_PER_DAY + c->addiction * ADDICTION_THIRST_BOOST;
        c->thirst = CLAMP(c->thirst + growth, 0.0f, 1.0f);

        c->wealth += c->income;

        if (c->addiction > HEALTHY_ADDICTION_THRESHOLD)
            c->health = CLAMP(c->health - c->addiction * HEALTH_DECAY_PER_ADDICTION, 0.0f, 1.0f);
        else
            c->health = CLAMP(c->health + HEALTH_RECOVERY_PER_DAY, 0.0f, 1.0f);

        if (c->health <= 0.0f) {
            c->alive = 0;
            pop->alive_count--;
            deaths_health++;
            continue;
        }

        if (c->age > OLD_AGE_THRESHOLD_DAYS) {
            float death_chance = CLAMP((c->age - OLD_AGE_THRESHOLD_DAYS) * OLD_AGE_DEATH_CHANCE_PER_DAY, 0.0f, 1.0f);
            if (frand() < death_chance) {
                c->alive = 0;
                pop->alive_count--;
                deaths_age++;
            }
        }
    }

    if (deaths_health > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%d townsfolk drank themselves to death", deaths_health);
        log_message(&w->log, buf, LOG_WARN);
    }
    if (deaths_age > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%d elderly townsfolk passed away", deaths_age);
        log_message(&w->log, buf, LOG_INFO);
    }
}

