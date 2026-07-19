#include "../include/merchant.h"
#include "../include/sim_random.h"
#include "../include/sim.h"

/* Per-drink balancing: how volatile the price is, and its min/max
   before inflation is applied. Add a row here when adding a drink. */
static const float DRINK_VOLATILITY[DRINK_COUNT] = { 0.15f, 0.25f };
static const float DRINK_MIN_PRICE[DRINK_COUNT]  = { 3.0f, 80.0f };
static const float DRINK_MAX_PRICE[DRINK_COUNT]  = { 8.0f, 120.0f };

void update_merchant(Merchant* m, float inflation_rate)
{
    if (frand() < m->instability) {
        m->quality += (frand() - 0.5f) * 0.2f;
        for (int d = 0; d < DRINK_COUNT; d++)
            m->drink_price[d] *= (1.0f + (frand() - 0.5f) * DRINK_VOLATILITY[d]);
    }

    m->quality = CLAMP(m->quality, 0.3f, 1.0f);
    for (int d = 0; d < DRINK_COUNT; d++)
        m->drink_price[d] = CLAMP(m->drink_price[d],
                                   DRINK_MIN_PRICE[d] * inflation_rate,
                                   DRINK_MAX_PRICE[d] * inflation_rate);
}
