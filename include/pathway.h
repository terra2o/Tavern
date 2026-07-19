#ifndef PATHWAY_H
#define PATHWAY_H

#include "sim.h"

void apply_clean_pathway(Tavern* b, int current_day);

/* Fraction (0.0-1.0) chance a citizen turns back at this tavern's
   entrance because its pathway is too dirty. */
float people_fall_because_pathway_dirty(Tavern* b, int current_day);

#endif
