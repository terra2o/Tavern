/*
*
* pathway.c for Tavern
*
* Copyright 2026 terra2o
*
*
*/

#include "../include/sim.h"
#include <stdio.h>

int lost_customers = 0;

void apply_clean_pathway(World* w, Tavern* b, int current_day)
{
    b->last_pathway_clean_day = current_day;
}

int people_fall_because_pathway_dirty(
    World* w,
    Tavern* b,
    int current_day,
    int customers
)
{
    if (current_day - b->last_pathway_clean_day >= 7) {
        int lost = customers / 4;

        char buf[128];
        snprintf(buf, sizeof(buf),
            "%d customers left because the pathway was dirty.", lost);
        log_message(&w->log, buf, LOG_INFO);

        return customers - lost;
    }

    return customers;
}

