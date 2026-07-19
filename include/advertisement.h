#ifndef ADVERTISEMENT_H
#define ADVERTISEMENT_H

#include "game_state.h"
#include "sim.h"

void apply_advertisement(int current_day, World *w);

/* Fraction (0.0-1.0) of would-be visitors who skip going out at all
   because nobody's advertised recently. World-scoped, since ads
   currently affect awareness of the town as a whole, not one tavern. */
float no_customers_because_no_ads(int current_day, World *w);

#endif
