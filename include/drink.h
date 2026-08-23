/*
*
* drink.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef DRINK_H
#define DRINK_H

#include "../include/inventory.h"

/* Add new drinks here, and bump DRINK_COUNT.
   Then add matching entries wherever a DRINK_COUNT-sized array
   is filled in (merchant.c balancing, citizen spawn preference, etc).
   The two wine entries must stay adjacent and in this order -
   wine.h's WINE_TO_DRINK() maps WineType onto them by offset. */
typedef enum {
    DRINK_ALE,
    DRINK_WINE_APPLE,
    DRINK_WINE_GRAPE,
    DRINK_COUNT
} DrinkType;

typedef struct Drink {
    float price;
    Inventory inventory;
} Drink;

#endif
