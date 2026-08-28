/*
*
* sim.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdio.h>
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

int tavern_actions_per_day(const Tavern* b)
{
    return 2 + b->employees;
}

void tavern_recompute_total_inventory(Tavern* b)
{
    int d;
    b->total_inventory = 0;
    for (d = 0; d < DRINK_COUNT; d++)
        b->total_inventory += b->drinks[d].inventory.amount;
}

void apply_action(Tavern* b, Action a, Town* t, Kingdom* k, World* w, int amount)
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
            if (t->population.alive_count >= 24) {
                b->rumor += (CLAMP(amount / (t->population.alive_count / 24), 0.0, 1.0));
            } else {
                b->rumor += 1.0;
            }
            apply_advertisement(w->day, t);
            break;

        case ACT_BUY_ALE: {
            int qty = amount < merchant_available_stock(b->supplier, DRINK_ALE)
                ? amount : merchant_available_stock(b->supplier, DRINK_ALE);
            float unit_price = merchant_quote_price(b->supplier, b->id, DRINK_ALE);
            b->drinks[DRINK_ALE].inventory.amount += qty;
            b->money -= qty * unit_price;
            tavern_recompute_total_inventory(b);
            merchant_record_purchase(b->supplier, b->id, DRINK_ALE, qty);
            break;
        }

        case ACT_BUY_WINE:
            /* handled outside apply_action - buying now needs to know
               which wine variety, which this function has no way to
               receive, so ui.c's ui_process_action does the real work */
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

        case ACT_COLLECT_FRUIT:
            /* handled outside apply_action, it's its own minigame loop */
            break;

        case ACT_MAKE_WINE: {
            /* FruitType and WineType share apple-then-grape order, so
               fruit i becomes wine variety i - no per-variety branching. */
            int made[WINE_COUNT];
            char buf[256];
            int i;

            for (i = 0; i < FRUIT_COUNT; i++) {
                made[i] = b->fruits[i].inventory.amount;
                b->drinks[WINE_TO_DRINK(i)].inventory.amount += made[i];
                b->fruits[i].inventory.amount = 0;
            }
            tavern_recompute_total_inventory(b);

            snprintf(buf, sizeof(buf), "You made %d apple wine and %d grape wine.",
                     made[WINE_APPLE], made[WINE_GRAPE]);
            log_message(&w->log, buf, LOG_INFO);

            break;
        }

        case ACT_HIRE_EMPLOYEES: {
            int max_employees = b->tavern_size * EMPLOYEES_PER_TAVERN_SIZE;
            if (b->employees >= max_employees) {
                log_message(&w->log, "Tavern is full, expand it to hire more employees.", LOG_INFO);
                break;
            }

            float employee_cut = b->employees_wage * k->inflation_rate;
            if (b->money >= employee_cut) {
                b->money -= employee_cut;
                b->employees++;
                log_message(&w->log, "You hired an employee.", LOG_INFO);
            } else {
                log_message(&w->log, "You don't have enough money to hire an employee.", LOG_INFO);
            }
            break;
        }

        case ACT_EXPAND_TAVERN: {
            float expand_cost = TAVERN_EXPAND_BASE_COST * b->tavern_size * k->inflation_rate;
            if (b->money >= expand_cost) {
                b->money -= expand_cost;
                b->tavern_size++;
                log_message(&w->log, "You expanded the tavern.", LOG_INFO);
            } else {
                log_message(&w->log, "You don't have enough money to expand the tavern.", LOG_INFO);
            }
            break;
        }
    }
}

void process_payment(Kingdom* k, World* w, Tavern* b, int current_day)
{
    int i;
    float employee_cut = b->employees_wage * k->inflation_rate;
    float total_paid_to_employees = 0;
    char buf_e[256];

    PeriodicPayment* p = &b->rent;
    if (current_day >= p->next_payment_day) {
        float actual_rent = p->base_rent * k->inflation_rate;
        char buf[256];
        b->money -= actual_rent;
        p->next_payment_day += p->pay_period;
        snprintf(buf, sizeof(buf), "Paid rent: $%.2f", actual_rent);
        log_message(&w->log, buf, LOG_IMPORTANT);
    }

    if (current_day >= p->next_wage_day) {
        if (b->employees >= 1) {
            for (i = 0; i < b->employees; i++) {
                b->money -= employee_cut;
                total_paid_to_employees += employee_cut;
            }
            snprintf(buf_e, sizeof(buf_e), "Wage paid to employees in total $%.2f", total_paid_to_employees);
            log_message(&w->log, buf_e, LOG_IMPORTANT);
        }
        p->next_wage_day += p->pay_period;
    }
}

/* Everything that happens once per day regardless of how many
   kingdoms/towns/taverns exist: population growth/aging, inflation,
   random events, and every merchant's price drift. Taverns sharing a
   merchant must not each re-roll its prices, so this updates the
   merchant pool directly instead of going through whichever tavern
   happens to call it. */
static void world_tick(World* w)
{
    int ki, ti;

    for (ki = 0; ki < w->kingdom_count; ki++) {
        Kingdom* k = &w->kingdoms[ki];
        float inflation_growth;

        for (ti = 0; ti < k->town_count; ti++) {
            Town* t = &k->towns[ti];
            int new_citizens = (int)(frand() * 5.0f) + 1;
            int j;
            for (j = 0; j < new_citizens; j++) citizen_spawn(&t->population);
            population_tick(&t->population, &w->log);
        }

        inflation_growth = inflation_tick(k);

        for (ti = 0; ti < k->town_count; ti++) {
            Town* t = &k->towns[ti];
            int m;
            for (m = 0; m < t->merchant_count; m++)
                update_merchant(&t->merchants[m], k->inflation_rate, inflation_growth);
        }
    }

    random_event(w);
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
    tavern_recompute_total_inventory(b);
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
   in b's town's pool against the current supplier and switches if a
   clearly better deal exists. */
static void ai_tavern_reconsider_supplier(Tavern* b, Town* t, World* w)
{
    int i;
    float avg_ale_price;
    float current_score;
    int best_id;
    float best_score;

    if (frand() >= AI_SUPPLIER_RECONSIDER_CHANCE || t->merchant_count <= 1) return;

    avg_ale_price = 0.0f;
    for (i = 0; i < t->merchant_count; i++)
        avg_ale_price += t->merchants[i].drink_price[DRINK_ALE];
    avg_ale_price /= t->merchant_count;

    current_score = supplier_score(b->supplier, b->id, avg_ale_price);
    best_id = b->supplier_id;
    best_score = current_score;
    for (i = 0; i < t->merchant_count; i++) {
        float s = supplier_score(&t->merchants[i], b->id, avg_ale_price);
        if (s > best_score) {
            best_score = s;
            best_id = i;
        }
    }

    if (best_id != b->supplier_id && best_score - current_score > AI_SUPPLIER_SWITCH_THRESHOLD) {
        char buf[128];
        b->supplier_id = best_id;
        b->supplier = &t->merchants[best_id];
        snprintf(buf, sizeof(buf), "Tavern #%d switched to a new supplier.", b->id);
        log_message(&w->log, buf, LOG_INFO);
    }
}

/* rival AI: presses a handful of the same buttons the
   player has, at random, plus a couple of heuristics apply_action
   doesn't cover (pricing and restocking) */
static void ai_tavern_decide(Tavern* b, Town* t, Kingdom* k, World* w)
{
    int d;
    float target;
    int buy;
    float cost;

    ai_tavern_reconsider_supplier(b, t, w);

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

    if (frand() < 0.4f) apply_action(b, ACT_CLEAN_PATHWAY, t, k, w, 0);
    if (frand() < 0.2f) apply_action(b, ACT_SKINCARE, t, k, w, 0);
    if (frand() < 0.2f) apply_action(b, ACT_TALK, t, k, w, 0);
    if (frand() < 0.1f) apply_action(b, ACT_CLEAN, t, k, w, 0);
}

/* How the player's tavern stacked up against the busiest rival today.
   Purely informational, logged once per day. Town-wide mood (thirst,
   addiction) is shown live in the left status panel instead, see
   draw_ui() in ui.c. */
static void log_daily_summary(Town* t, World* w, DayResult* results)
{
    char buf[160];
    int best_rival = -1;
    int best_rival_customers = -1;
    int player_customers;
    int i;

    if (t->tavern_count > 1) {
        for (i = 0; i < t->tavern_count; i++) {
            if (i == t->player_tavern_id) continue;
            if (results[i].customers > best_rival_customers) {
                best_rival_customers = results[i].customers;
                best_rival = i;
            }
        }
        player_customers = results[t->player_tavern_id].customers;
        snprintf(buf, sizeof(buf), "Competition: you drew %d customers, tavern #%d drew %d.",
                 player_customers, best_rival, best_rival_customers);
        log_message(&w->log, buf, LOG_INFO);
    }
}

int simulate_day(World* w)
{
    int sales_today = 0;
    int ki, ti;

    world_tick(w);

    for (ki = 0; ki < w->kingdom_count; ki++) {
        Kingdom* k = &w->kingdoms[ki];

        for (ti = 0; ti < k->town_count; ti++) {
            Town* t = &k->towns[ti];
            DayResult results[MAX_TAVERNS] = {0};
            int is_player_town = (k->id == w->player_kingdom_id && t->id == k->player_town_id);
            int j, d;

            /* Every tavern settles its state (price, stock, cleanliness)
               before the shared market pass, since citizens are choosing
               between taverns, not visiting each one independently. */
            for (j = 0; j < t->tavern_count; j++) {
                int is_player_tavern = is_player_town && j == t->player_tavern_id;
                if (!is_player_tavern)
                    ai_tavern_decide(&t->taverns[j], t, k, w);
                process_payment(k, w, &t->taverns[j], w->day);
            }

            market_simulate_all(t, w, results);

            for (j = 0; j < t->tavern_count; j++)
                tavern_post_market(&t->taverns[j], &results[j]);

            /* Whether a fight/vomit/steal event fires today is driven by
               who actually showed up at each tavern. The player's tavern
               surfaces an interactive prompt; rival taverns resolve
               automatically. */
            for (j = 0; j < t->tavern_count; j++)
                evaluate_customer_events(k, t, w, j, &results[j]);

            if (is_player_town) {
                log_daily_summary(t, w, results);
                for (d = 0; d < DRINK_COUNT; d++)
                    sales_today += results[t->player_tavern_id].sales[d];
            }
        }
    }

    /*
     *    THIS... is important.
     *    we only make w->day go up here,
     *    then we have local variables
     *    that just store its value locally
    */
    w->day++;

    return sales_today;
}
