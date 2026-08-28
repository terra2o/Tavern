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

#include "kingdom.h"

/* Returns the multiplier applied to k->inflation_rate today (post-clamp). */
float inflation_tick(Kingdom* k);

#endif
