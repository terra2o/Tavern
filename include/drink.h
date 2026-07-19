#ifndef DRINK_H
#define DRINK_H

/* Add new drinks here, and bump DRINK_COUNT.
   Then add matching entries wherever a DRINK_COUNT-sized array
   is filled in (merchant.c balancing, citizen spawn preference, etc). */
typedef enum {
    DRINK_ALE,
    DRINK_WINE,
    DRINK_COUNT
} DrinkType;

typedef struct Inventory {
    int amount;
    int expiration_date;
} Inventory;

typedef struct Drink {
    float price;
    Inventory inventory;
} Drink;

#endif
