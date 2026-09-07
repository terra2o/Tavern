/*
 *
 * food.h for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#ifndef FOOD_H
#define FOOD_H

#include "inventory.h"

typedef enum { FOOD_BREAD, FOOD_STEW, FOOD_COUNT } FoodType;

typedef struct Food {
    float price;
    Inventory inventory;
} Food;

#endif
