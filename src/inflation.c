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
#include "../include/sim.h"

#define INFLATION_BASE_DECAY               0.0015f /* pulls inflation back toward 1.0 absent pressure */
#define INFLATION_WAR_PRESSURE             0.0020f /* wartime spending/shortages push prices up */
#define INFLATION_MONEY_SUPPLY_SENSITIVITY 0.5f    /* fraction of daily money-supply growth that passes through */
#define INFLATION_SCARCITY_SENSITIVITY     0.01f   /* merchant stock scarcity -> price pressure */
#define INFLATION_MIN 0.5f
#define INFLATION_MAX 5.0f

static float avg_money_supply(const World* w)
{
    float total = 0.0f;
    int n = 0;
    int i;

    for (i = 0; i < w->tavern_count; i++) {
        total += w->taverns[i].money;
        n++;
    }
    for (i = 0; i < w->population.count; i++) {
        if (w->population.citizens[i].alive) {
            total += w->population.citizens[i].wealth;
            n++;
        }
    }
    return n > 0 ? total / (float)n : 0.0f;
}

static float merchant_scarcity(const World* w)
{
    float scarcity_sum = 0.0f;
    int n = 0;
    int i, d;
    for (i = 0; i < w->merchant_count; i++) {
        const Merchant* m = &w->merchants[i];
        for (d = 0; d < DRINK_COUNT; d++) {
            float ratio = m->max_stock[d] > 0.0f ? m->stock[d] / m->max_stock[d] : 1.0f;
            scarcity_sum += (1.0f - ratio);
            n++;
        }
    }
    return n > 0 ? scarcity_sum / n : 0.0f;
}

/* Runs the daily inflation model and returns the actual multiplier applied
   to w->inflation_rate today (post-clamp), so callers like update_merchant
   can compound that same day's inflation directly into nominal prices
   instead of only being bounded by the (slower-moving) absolute rate. */
float inflation_tick(World* w)
{
    float money_supply = avg_money_supply(w);
    float supply_growth = 0.0f;
    float pressure;
    float old_rate = w->inflation_rate;

    if (w->money_supply_prev > 0.0f)
        supply_growth = (money_supply - w->money_supply_prev) / w->money_supply_prev;
    w->money_supply_prev = money_supply;

    pressure = supply_growth * INFLATION_MONEY_SUPPLY_SENSITIVITY
             + merchant_scarcity(w) * INFLATION_SCARCITY_SENSITIVITY
             + (w->at_war ? INFLATION_WAR_PRESSURE : 0.0f)
             - INFLATION_BASE_DECAY;

    w->inflation_rate *= 1.0f + pressure;
    w->inflation_rate = CLAMP(w->inflation_rate, INFLATION_MIN, INFLATION_MAX);

    return w->inflation_rate / old_rate;
}
