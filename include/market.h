/*
*
* market.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef MARKET_H
#define MARKET_H

#include "sim.h"

/* Simulates one day of foot traffic across every tavern in t->taverns at
   once: each citizen in t->population decides whether to go out, and if
   so, picks one tavern and one drink to buy there (or none, if nothing
   fits their budget/taste). results must have room for at least
   t->tavern_count entries; results[i] corresponds to t->taverns[i]. */
void market_simulate_all(Town* t, World* w, DayResult* results);

#endif
