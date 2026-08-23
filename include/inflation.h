/*
*
* inflation.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef INFLATION_H
#define INFLATION_H

#include "game_state.h"

/* Returns the multiplier applied to w->inflation_rate today (post-clamp). */
float inflation_tick(World* w);

#endif
