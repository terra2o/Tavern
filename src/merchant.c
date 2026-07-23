#include "../include/merchant.h"
#include "../include/sim_random.h"
#include "../include/sim.h"

/* Per-drink balancing: how volatile the price is, and its min/max
   before inflation is applied. Add a row here when adding a drink. */
static const float DRINK_MIN_PRICE[DRINK_COUNT]  = { 3.0f, 80.0f };
static const float DRINK_MAX_PRICE[DRINK_COUNT]  = { 8.0f, 120.0f };

static const float DRINK_DEFAULT_MAX_STOCK[DRINK_COUNT]       = { 400.0f, 150.0f };
static const float DRINK_DEFAULT_RESTOCK_RATE[DRINK_COUNT]    = { 40.0f, 12.0f };
static const float DRINK_STOCK_PRICE_SENSITIVITY[DRINK_COUNT] = { 0.35f, 0.35f };

#define FAVOR_DISCOUNT_MAX    0.15f
#define FAVOR_GAIN_PER_UNIT   0.002f
#define FAVOR_DAILY_DECAY     0.01f

void update_merchant(Merchant* m, float inflation_rate)
{
    int d;
    float restock_reliability;
    float stock_ratio;
    float target;
    int t;

    /* Restock is less reliable the riskier this merchant's supply is. */
    restock_reliability = 1.0f - m->instability * 0.5f;
    for (d = 0; d < DRINK_COUNT; d++) {
        m->stock[d] += m->restock_rate[d] * restock_reliability;
        m->stock[d] = CLAMP(m->stock[d], 0.0f, m->max_stock[d]);
    }

    if (frand() < m->instability)
        m->quality += (frand() - 0.5f) * 0.2f;
    m->quality = CLAMP(m->quality, 0.3f, 1.0f);

    for (d = 0; d < DRINK_COUNT; d++) {
        stock_ratio = m->max_stock[d] > 0.0f ? m->stock[d] / m->max_stock[d] : 1.0f;
        target = m->drink_price[d] * (1.0f + (1.0f - stock_ratio) * DRINK_STOCK_PRICE_SENSITIVITY[d]);
        m->drink_price[d] += (target - m->drink_price[d]) * 0.2f;
        m->drink_price[d] = CLAMP(m->drink_price[d],
                                   DRINK_MIN_PRICE[d] * inflation_rate,
                                   DRINK_MAX_PRICE[d] * inflation_rate);
    }

    for (t = 0; t < MAX_TAVERNS; t++)
        m->tavern_favor[t] = CLAMP(m->tavern_favor[t] - FAVOR_DAILY_DECAY, 0.0f, 1.0f);
}

int merchant_available_stock(const Merchant* m, DrinkType d)
{
    return (int)m->stock[d];
}

float merchant_quote_price(const Merchant* m, int tavern_id, DrinkType d)
{
    float favor;

    favor = (tavern_id >= 0 && tavern_id < MAX_TAVERNS) ? m->tavern_favor[tavern_id] : 0.0f;
    return m->drink_price[d] * (1.0f - favor * FAVOR_DISCOUNT_MAX);
}

void merchant_record_purchase(Merchant* m, int tavern_id, DrinkType d, int qty)
{
    m->stock[d] = CLAMP(m->stock[d] - (float)qty, 0.0f, m->max_stock[d]);

    if (tavern_id >= 0 && tavern_id < MAX_TAVERNS) {
        m->tavern_favor[tavern_id] += (float)qty * FAVOR_GAIN_PER_UNIT;
        m->tavern_favor[tavern_id] = CLAMP(m->tavern_favor[tavern_id], 0.0f, 1.0f);
    }
}

void merchant_init_default_stock(Merchant* m)
{
    int d;
    for (d = 0; d < DRINK_COUNT; d++) {
        m->max_stock[d] = DRINK_DEFAULT_MAX_STOCK[d];
        m->stock[d] = DRINK_DEFAULT_MAX_STOCK[d];
        m->restock_rate[d] = DRINK_DEFAULT_RESTOCK_RATE[d];
    }
}
