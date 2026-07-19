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

#define PATHWAY_DIRTY_THRESHOLD_DAYS 7
#define PATHWAY_BASE_LOSS_FRACTION 0.25
#define PATHWAY_LOSS_GROWTH_PER_DAY 0.025
#define PATHWAY_LOSS_MULTIPLIER_CAP 3.0

void apply_clean_pathway(Tavern* b, int current_day)
{
    b->last_pathway_clean_day = current_day;
}

float people_fall_because_pathway_dirty(Tavern* b, int current_day)
{
    int days_dirty = current_day - b->last_pathway_clean_day;
    if (days_dirty < PATHWAY_DIRTY_THRESHOLD_DAYS) return 0.0f;

    double loss_multiplier = 1.0 + (days_dirty - PATHWAY_DIRTY_THRESHOLD_DAYS) * PATHWAY_LOSS_GROWTH_PER_DAY;
    if (loss_multiplier > PATHWAY_LOSS_MULTIPLIER_CAP) loss_multiplier = PATHWAY_LOSS_MULTIPLIER_CAP;

    return (float)CLAMP(PATHWAY_BASE_LOSS_FRACTION * loss_multiplier, 0.0, 1.0);
}
