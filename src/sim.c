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

        case ACT_BUY_ALE:
            b->drinks[DRINK_ALE].inventory.amount += amount;
            b->money -= amount * b->supplier->drink_price[DRINK_ALE];
            b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
            break;


        case ACT_BUY_WINE:
            b->drinks[DRINK_WINE].inventory.amount += amount;
            b->money -= amount * b->supplier->drink_price[DRINK_WINE];
            b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
            break;

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

/* rival AI: presses a handful of the same buttons the
   player has, at random, plus a couple of heuristics apply_action
   doesn't cover (pricing and restocking) */
static void ai_tavern_decide(Tavern* b, World* w)
{
    int d;
    float target;
    int buy;
    float cost;

    /* Track supplier cost with a randomized markup instead of a fixed price */
    for (d = 0; d < DRINK_COUNT; d++) {
        target = b->supplier->drink_price[d] * (1.5f + frand() * 0.5f);
        b->drinks[d].price += (target - b->drinks[d].price) * 0.2f;
    }

    /* Restock whichever drink is running low, if affordable */
    for (d = 0; d < DRINK_COUNT; d++) {
        if (b->drinks[d].inventory.amount < 5) {
            buy = 20;
            cost = buy * b->supplier->drink_price[d];
            if (b->money >= cost) {
                b->drinks[d].inventory.amount += buy;
                b->money -= cost;
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
