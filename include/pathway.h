#ifndef PATHWAY_H
#define PATHWAY_H

#include "sim.h"

void apply_clean_pathway(World* w, Tavern* b, int current_day);
void people_fall_because_pathway_dirty(World* w,
    Tavern* b,
    int current_day,
    int customers);

#endif