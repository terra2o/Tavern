/*
*
* wine.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef WINE_H
#define WINE_H

#include "drink.h"

typedef enum WineType {
    WINE_APPLE,
    WINE_GRAPE,
    WINE_COUNT
} WineType;

/* Wine varieties are just the DrinkType entries right after DRINK_ALE,
   in the same apple-then-grape order as WineType, so no lookup table
   is needed to go from one to the other. */
#define WINE_TO_DRINK(w) (DRINK_WINE_APPLE + (w))

#endif
