#ifndef MARKET_H
#define MARKET_H

#include "sim.h"

/* Simulates one day of foot traffic across every tavern in w->taverns at
   once: each citizen decides whether to go out, and if so, picks one
   tavern and one drink to buy there (or none, if nothing fits their
   budget/taste). results must have room for at least w->tavern_count
   entries; results[i] corresponds to w->taverns[i]. */
void market_simulate_all(World* w, DayResult* results);

#endif
