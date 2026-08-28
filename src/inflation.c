/*
*
* inflation.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include "../include/inflation.h"
#include "../include/merchant.h"
#include "../include/sim.h"

#define INFLATION_BASE_DECAY               0.0015f /* pulls inflation back toward 1.0 absent pressure */
#define INFLATION_WAR_PRESSURE             0.0020f /* wartime spending/shortages push prices up */
#define INFLATION_MONEY_SUPPLY_SENSITIVITY 0.5f    /* fraction of daily money-supply growth that passes through */
#define INFLATION_SCARCITY_SENSITIVITY     0.01f   /* merchant stock scarcity -> price pressure */
#define INFLATION_MIN 0.5f
#define INFLATION_MAX 5.0f

static float avg_money_supply(const Kingdom* k)
{
    float total = 0.0f;
    int n = 0;
    int i, j;

    for (i = 0; i < k->town_count; i++) {
        const Town* t = &k->towns[i];
        for (j = 0; j < t->tavern_count; j++) {
            total += t->taverns[j].money;
            n++;
        }
        for (j = 0; j < t->population.count; j++) {
            if (t->population.citizens[j].alive) {
                total += t->population.citizens[j].wealth;
                n++;
            }
        }
    }
    return n > 0 ? total / (float)n : 0.0f;
}

static float merchant_scarcity(const Kingdom* k)
{
    float scarcity_sum = 0.0f;
    int n = 0;
    int i, j, d;
    for (i = 0; i < k->town_count; i++) {
        const Town* t = &k->towns[i];
        for (j = 0; j < t->merchant_count; j++) {
            const Merchant* m = &t->merchants[j];
            for (d = 0; d < DRINK_COUNT; d++) {
                float ratio = m->max_stock[d] > 0.0f ? m->stock[d] / m->max_stock[d] : 1.0f;
                scarcity_sum += (1.0f - ratio);
                n++;
            }
        }
    }
    return n > 0 ? scarcity_sum / n : 0.0f;
}

/* Runs the daily inflation model and returns the actual multiplier applied
   to k->inflation_rate today (post-clamp), so callers like update_merchant
   can compound that same day's inflation directly into nominal prices
   instead of only being bounded by the (slower-moving) absolute rate. */
float inflation_tick(Kingdom* k)
{
    float money_supply = avg_money_supply(k);
    float supply_growth = 0.0f;
    float pressure;
    float old_rate = k->inflation_rate;

    if (k->money_supply_prev > 0.0f)
        supply_growth = (money_supply - k->money_supply_prev) / k->money_supply_prev;
    k->money_supply_prev = money_supply;

    pressure = supply_growth * INFLATION_MONEY_SUPPLY_SENSITIVITY
             + merchant_scarcity(k) * INFLATION_SCARCITY_SENSITIVITY
             + (k->at_war ? INFLATION_WAR_PRESSURE : 0.0f)
             - INFLATION_BASE_DECAY;

    k->inflation_rate *= 1.0f + pressure;
    k->inflation_rate = CLAMP(k->inflation_rate, INFLATION_MIN, INFLATION_MAX);

    return k->inflation_rate / old_rate;
}
