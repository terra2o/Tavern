/*
 *
 * modifier.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "modifier.h"

float modifier_get_total(ModifierTarget target, Modifier *modifiers, int count,
                         float base_val)
{
    float flat_total = 0.0f;
    float percent_total = 1.0f;
    int i;

    if (!modifiers || count == 0)
        return base_val;

    for (i = 0; i < count; i++) {
        if (modifiers[i].target == target) {
            if (modifiers[i].type == MOD_FLAT)
                flat_total += modifiers[i].value;
            else if (modifiers[i].type == MOD_PERCENT)
                percent_total += modifiers[i].value;
        }
    }

    return (base_val + flat_total) * percent_total;
}
