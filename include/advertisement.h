#ifndef ADVERTISEMENT_H
#define ADVERTISEMENT_H

#include "game_state.h"
#include "sim.h"

void apply_advertisement(int current_day, World *w);

int no_customers_because_no_ads(int current_day, World *w, DayResult *r, int customers);

#endif
