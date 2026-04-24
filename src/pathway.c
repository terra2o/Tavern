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

void apply_clean_pathway(Tavern* b, int current_day)
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
        int days_dirty = current_day - b->last_pathway_clean_day;
        
        double loss_multiplier = 1.0 + (days_dirty - 7) * 0.025;
        if (loss_multiplier > 3.0) loss_multiplier = 3.0;
        
        int lost = (int)(customers * 0.25 * loss_multiplier);
        char buf[128];
        snprintf(buf, sizeof(buf),
            "%d customers because pathway is too dirty.", lost);
        log_message(&w->log, buf, LOG_WARN);
        
        return lost;
    }
    
    return 0;
}
