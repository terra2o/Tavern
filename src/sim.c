#include <stdio.h>
#include <stdlib.h>
#include "../include/merchant.h"
#include "../include/sim.h"
#include "../include/advertisement.h"
#include "../include/sim_random.h"
#include "../include/market.h"
#include "../include/reputation.h"
#include "../include/pathway.h"
#include "../include/population.h"
#include "../include/inflation.h"
#include "../include/event.h"

void apply_action(Tavern* b, Action a, World* w, int amount)
{
    switch (a) {
        case ACT_SKINCARE:
            b->handsomeness += 0.08f;
            b->rumor += 0.10f;
            break;

        case ACT_CLEAN:
            b->consistency += 0.1f;
            b->quality_perceived += 0.1f;
            break;

        case ACT_TALK:
            b->rumor += (frand() - 0.3f) * 0.15f;
            break;

        case ACT_CHECK_QUALITY:
            b->quality_perceived +=
                (b->quality_actual - b->quality_perceived) * 0.3f;
            break;

        case ACT_ADVERTISE:
            b->money -= amount;
            /* 24 because i tried balancing it. */
            if (w->population.alive_count >= 24) {
                b->rumor += (CLAMP(amount / (w->population.alive_count / 24), 0.0, 1.0));
            } else {
                b->rumor += 1.0;
            }
            apply_advertisement(w->day, w);
            break;

        case ACT_BUY_ALE: {
            int qty = amount < merchant_available_stock(b->supplier, DRINK_ALE)
                ? amount : merchant_available_stock(b->supplier, DRINK_ALE);
            float unit_price = merchant_quote_price(b->supplier, b->id, DRINK_ALE);
            b->drinks[DRINK_ALE].inventory.amount += qty;
            b->money -= qty * unit_price;
            b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
            merchant_record_purchase(b->supplier, b->id, DRINK_ALE, qty);
            break;
        }

        case ACT_BUY_WINE: {
            int qty = amount < merchant_available_stock(b->supplier, DRINK_WINE)
                ? amount : merchant_available_stock(b->supplier, DRINK_WINE);
            float unit_price = merchant_quote_price(b->supplier, b->id, DRINK_WINE);
            b->drinks[DRINK_WINE].inventory.amount += qty;
            b->money -= qty * unit_price;
            b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
            merchant_record_purchase(b->supplier, b->id, DRINK_WINE, qty);
            break;
        }

        case ACT_ADJUST_ALE_PRICE:
            /* handled outside apply_action */
            break;

        case ACT_ADJUST_WINE_PRICE:
            /* handled outside apply_action */
            break;

        case ACT_CLEAN_PATHWAY:
            apply_clean_pathway(b, w->day);
            break;
    }
}

void process_payment(World* w, Tavern* b, int current_day)
{
    PeriodicPayment* p = &b->rent;
    if (current_day >= p->next_payment_day) {
        float actual_rent = p->base_rent * w->inflation_rate;
        char buf[256];
        b->money -= actual_rent;
        p->next_payment_day += p->pay_period;
        snprintf(buf, sizeof(buf), "Paid rent: $%.2f", actual_rent);
        log_message(&w->log, buf, LOG_IMPORTANT);
    }
}

/* Everything that happens once per day regardless of how many taverns
   exist: population growth/aging, inflation, random events, and every
   merchant's price drift. Taverns sharing a merchant must not each
   re-roll its prices, so this updates the merchant pool directly
   instead of going through whichever tavern happens to call it. */
static void world_tick(World* w)
{
    int new_citizens;
    int i;

    new_citizens = (int)(frand() * 5.0f) + 1;
    for (i = 0; i < new_citizens; i++) citizen_spawn(&w->population);
    population_tick(w);
    inflation_tick(w);
    random_event(w);
    for (i = 0; i < w->merchant_count; i++)
        update_merchant(&w->merchants[i], w->inflation_rate);
}

/* Post-market bookkeeping for one tavern: reputation/consistency
   tracking against that day's DayResult. Assumes market_simulate_all()
   already ran for the day (it needs every tavern's price/stock/
   pathway state settled first, since citizens are choosing between
   taverns, not visiting each independently). */
static void tavern_post_market(Tavern* b, const DayResult* day)
{
    int d;
    int sales_today;
    float price_change;

    b->quality_actual = b->supplier->quality;

    /* Consistency punishes wild price changes */
    for (d = 0; d < DRINK_COUNT; d++) {
        price_change = fabsf(b->drinks[d].price - b->last_drink_price[d]);
        b->consistency -= price_change * 0.5f;
        b->last_drink_price[d] = b->drinks[d].price;
    }

    b->quality_perceived = CLAMP(b->quality_perceived, 0, 1);
    b->rumor = CLAMP(b->rumor, 0, 1);
    b->consistency = CLAMP(b->consistency, 0, 1);

    sales_today = 0;
    for (d = 0; d < DRINK_COUNT; d++) sales_today += day->sales[d];
    b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
    reputation_tick(b, sales_today);
}

#define AI_SUPPLIER_RECONSIDER_CHANCE 0.1f
#define AI_SUPPLIER_SWITCH_THRESHOLD  0.05f
#define AI_SUPPLIER_WEIGHT_QUALITY    1.0f
#define AI_SUPPLIER_WEIGHT_PRICE      0.6f
#define AI_SUPPLIER_WEIGHT_FAVOR      0.3f
#define AI_SUPPLIER_WEIGHT_RISK       0.4f

/* Higher is more attractive. Price is normalized against avg_ale_price
   (the pool's average ale price) so it's comparable across merchants
   regardless of ale's raw price scale. */
static float supplier_score(const Merchant* m, int tavern_id, float avg_ale_price)
{
    float price_ratio = avg_ale_price > 0.0f
        ? merchant_quote_price(m, tavern_id, DRINK_ALE) / avg_ale_price
        : 1.0f;
    return m->quality * AI_SUPPLIER_WEIGHT_QUALITY
         - price_ratio * AI_SUPPLIER_WEIGHT_PRICE
         + m->tavern_favor[tavern_id] * AI_SUPPLIER_WEIGHT_FAVOR
         - m->instability * AI_SUPPLIER_WEIGHT_RISK;
}

/* Rarely (not every day, to avoid thrashing), compares every merchant
   in the pool against the current supplier and switches if a clearly
   better deal exists. */
static void ai_tavern_reconsider_supplier(Tavern* b, World* w)
{
    int i;
    float avg_ale_price;
    float current_score;
    int best_id;
    float best_score;

    if (frand() >= AI_SUPPLIER_RECONSIDER_CHANCE || w->merchant_count <= 1) return;

    avg_ale_price = 0.0f;
    for (i = 0; i < w->merchant_count; i++)
        avg_ale_price += w->merchants[i].drink_price[DRINK_ALE];
    avg_ale_price /= w->merchant_count;

    current_score = supplier_score(b->supplier, b->id, avg_ale_price);
    best_id = b->supplier_id;
    best_score = current_score;
    for (i = 0; i < w->merchant_count; i++) {
        float s = supplier_score(&w->merchants[i], b->id, avg_ale_price);
        if (s > best_score) {
            best_score = s;
            best_id = i;
        }
    }

    if (best_id != b->supplier_id && best_score - current_score > AI_SUPPLIER_SWITCH_THRESHOLD) {
        char buf[128];
        b->supplier_id = best_id;
        b->supplier = &w->merchants[best_id];
        snprintf(buf, sizeof(buf), "Tavern #%d switched to a new supplier.", b->id);
        log_message(&w->log, buf, LOG_INFO);
    }
}

/* rival AI: presses a handful of the same buttons the
   player has, at random, plus a couple of heuristics apply_action
   doesn't cover (pricing and restocking) */
static void ai_tavern_decide(Tavern* b, World* w)
{
    int d;
    float target;
    int buy;
    float cost;

    ai_tavern_reconsider_supplier(b, w);

    /* Track supplier cost with a randomized markup instead of a fixed price */
    for (d = 0; d < DRINK_COUNT; d++) {
        target = merchant_quote_price(b->supplier, b->id, d) * (1.5f + frand() * 0.5f);
        b->drinks[d].price += (target - b->drinks[d].price) * 0.2f;
    }

    /* Restock whichever drink is running low, if affordable and in stock */
    for (d = 0; d < DRINK_COUNT; d++) {
        if (b->drinks[d].inventory.amount < 5) {
            buy = 20;
            if (buy > merchant_available_stock(b->supplier, d))
                buy = merchant_available_stock(b->supplier, d);
            cost = buy * merchant_quote_price(b->supplier, b->id, d);
            if (buy > 0 && b->money >= cost) {
                b->drinks[d].inventory.amount += buy;
                b->money -= cost;
                merchant_record_purchase(b->supplier, b->id, d, buy);
            }
        }
    }

    if (frand() < 0.4f) apply_action(b, ACT_CLEAN_PATHWAY, w, 0);
    if (frand() < 0.2f) apply_action(b, ACT_SKINCARE, w, 0);
    if (frand() < 0.2f) apply_action(b, ACT_TALK, w, 0);
    if (frand() < 0.1f) apply_action(b, ACT_CLEAN, w, 0);
}

/* How the player's tavern stacked up against the busiest rival today.
   Purely informational, logged once per day. Town-wide mood (thirst,
   addiction) is shown live in the left status panel instead, see
   draw_ui() in ui.c. */
static void log_daily_summary(World* w, DayResult* results)
{
    char buf[160];
    int best_rival = -1;
    int best_rival_customers = -1;
    int player_customers;
    int i;

    if (w->tavern_count > 1) {
        for (i = 0; i < w->tavern_count; i++) {
            if (i == w->player_tavern_id) continue;
            if (results[i].customers > best_rival_customers) {
                best_rival_customers = results[i].customers;
                best_rival = i;
            }
        }
        player_customers = results[w->player_tavern_id].customers;
        snprintf(buf, sizeof(buf), "Competition: you drew %d customers, tavern #%d drew %d.",
                 player_customers, best_rival, best_rival_customers);
        log_message(&w->log, buf, LOG_INFO);
    }
}

int simulate_day(World* w)
{
    DayResult results[MAX_TAVERNS] = {0};
    int sales_today;
    int i;
    int d;
    int tavern;

    world_tick(w);

    /* Every tavern settles its state (price, stock, cleanliness) before
       the shared market pass, since citizens are choosing between
       taverns, not visiting each one independently. */
    for (i = 0; i < w->tavern_count; i++) {
        if (i != w->player_tavern_id)
            ai_tavern_decide(&w->taverns[i], w);
        process_payment(w, &w->taverns[i], w->day);
    }

    market_simulate_all(w, results);

    for (i = 0; i < w->tavern_count; i++)
        tavern_post_market(&w->taverns[i], &results[i]);

    /* Whether a fight/vomit/steal event fires today is driven by who
       actually showed up at each tavern. The player's tavern surfaces
       an interactive prompt; rival taverns resolve automatically. */
    for (tavern = 0; tavern < w->tavern_count; tavern++)
        evaluate_customer_events(w, tavern, &results[tavern]);
    log_daily_summary(w, results);

    sales_today = 0;
    for (d = 0; d < DRINK_COUNT; d++) sales_today += results[w->player_tavern_id].sales[d];

    /*
     *    THIS... is important.
     *    we only make w->day go up here,
     *    then we have local variables
     *    that just store its value locally
    */
    w->day++;

    return sales_today;
}

void world_taverns_init(World* w, int capacity)
{
    free(w->taverns); /* idempotent: safe to call again to reset the pool */
    w->tavern_capacity = capacity;
    w->tavern_count = 0;
    w->taverns = malloc(capacity * sizeof(Tavern));
}

void world_taverns_free(World* w)
{
    free(w->taverns);
    w->taverns = NULL;
    w->tavern_count = 0;
    w->tavern_capacity = 0;
}

int world_add_tavern(World* w, Tavern t)
{
    if (w->tavern_count >= w->tavern_capacity) return -1;
    t.id = w->tavern_count;
    w->taverns[w->tavern_count] = t;
    return w->tavern_count++;
}

void world_merchants_init(World* w, int capacity)
{
    free(w->merchants); /* idempotent: safe to call again to reset the pool */
    w->merchant_capacity = capacity;
    w->merchant_count = 0;
    w->merchants = malloc(capacity * sizeof(Merchant));
}

void world_merchants_free(World* w)
{
    free(w->merchants);
    w->merchants = NULL;
    w->merchant_count = 0;
    w->merchant_capacity = 0;
}

int world_add_merchant(World* w, Merchant m)
{
    if (w->merchant_count >= w->merchant_capacity) return -1;
    w->merchants[w->merchant_count] = m;
    return w->merchant_count++;
}

void world_relink_suppliers(World* w)
{
    int i;
    Tavern* t;

    for (i = 0; i < w->tavern_count; i++) {
        t = &w->taverns[i];
        t->supplier = (t->supplier_id >= 0 && t->supplier_id < w->merchant_count)
            ? &w->merchants[t->supplier_id]
            : NULL;
    }
}
