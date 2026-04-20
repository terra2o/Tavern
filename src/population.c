#include "../include/sim_random.h"
#include "../include/game_state.h"
#include "../include/log.h"
#include <stdio.h>

void populate(World* w)
{
    int random_int = (int)(frand() * 5.0f) + 1;
    w->population += random_int;
    char populate_message[64];
    char population_message[64];
    snprintf(populate_message, sizeof(populate_message), "Population increased by %d", random_int);
    snprintf(population_message, sizeof(population_message), "Population: %d", w->population);
    log_message(&w->log, populate_message, LOG_INFO);
    log_message(&w->log, population_message, LOG_INFO);
}

