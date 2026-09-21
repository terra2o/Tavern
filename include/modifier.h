/*
 *
 * modifier.h for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#ifndef MODIFIER_H
#define MODIFIER_H

typedef enum ModifierTarget {
    MOD_TIP_RATE,
    MOD_PATIENCE_DECAY,
    MOD_FIGHT_CHANCE,
    MOD_DRINK_TOLERANCE,
    MOD_CLEANLINESS_TOLERANCE,
    MOD_BARDS_APPRECIATION,
    MOD_SEATING_PREFERENCE_BOOTH,
    MOD_MERCHANT_PRICE_WINE,
    MOD_MERCHANT_PRICE_ALE,
    MOD_MERCHANT_SELL_PRICE,
    MOD_COUNT
} ModifierTarget;

/* distinguishes flat additions from percentage scalers */
typedef enum ModifierType { MOD_FLAT, MOD_PERCENT } ModifierType;

typedef struct Modifier {
    ModifierTarget target;
    ModifierType type;
    float value; /* positive = buff, negative = debuff */
} Modifier;

/* calculate the total modifier value for a given target */
float modifier_get_total(ModifierTarget target, Modifier *modifiers, int count,
                         float base_val);

#endif /* MODIFIER_H */
