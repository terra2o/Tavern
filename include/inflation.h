#ifndef INFLATION_H
#define INFLATION_H

#include "game_state.h"

/* Returns the multiplier applied to w->inflation_rate today (post-clamp). */
float inflation_tick(World* w);

#endif
