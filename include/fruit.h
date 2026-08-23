/*
*
* fruit.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef FRUIT_H
#define FRUIT_H

#include "../include/inventory.h"

typedef enum {
    FRUIT_APPLE,
    FRUIT_GRAPE,
    FRUIT_COUNT,
} FruitType;

typedef struct Fruit {
    Inventory inventory;
} Fruit;

#endif
