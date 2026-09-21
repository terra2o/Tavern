/*
 *
 * sim.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "sim.h"
#include "advertisement.h"
#include "event.h"
#include "inflation.h"
#include "market.h"
#include "merchant.h"
#include "pathway.h"
#include "population.h"
#include "reputation.h"
#include "sim_random.h"
#include <stdio.h>

int tavern_actions_per_day(const Tavern *b) { return 2 + b->employee_count; }

int tavern_action_start_minute(int action_num, int total_actions)
{
    if (total_actions <= 0)
        return 0;
    return (action_num - 1) * 1440 / total_actions;
}

int tavern_action_end_minute(int action_num, int total_actions)
{
    if (total_actions <= 0)
        return 1440;
    return action_num * 1440 / total_actions;
}

float tavern_action_duration_hours(int total_actions)
{
    if (total_actions <= 0)
        return 24.0f;
    return 24.0f / (float)total_actions;
}

void tavern_recompute_total_inventory(Tavern *b)
{
    int d;
    b->total_inventory = 0;
    for (d = 0; d < DRINK_COUNT; d++)
        b->total_inventory += b->drinks[d].inventory.amount;
}

void apply_action(Tavern *b, Action a, Town *t, Kingdom *k, World *w,
                  int amount)
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
            b->rumor +=
                (CLAMP(amount / (t->population.alive_count / 24), 0.0, 1.0));
        } else {
            b->rumor += 1.0;
        }
        apply_advertisement(w->day, t);
        break;

    case ACT_BUY_ALE: {
        int qty = amount < merchant_available_stock(b->supplier, DRINK_ALE)
                      ? amount
                      : merchant_available_stock(b->supplier, DRINK_ALE);
        float unit_price = merchant_quote_price(b->supplier, b, DRINK_ALE);
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

        tavern_snprintf(buf, sizeof(buf),
                        "You made %d apple wine and %d grape wine.",
                        made[WINE_APPLE], made[WINE_GRAPE]);
        log_message(&w->log, buf, LOG_INFO);

        break;
    }

    case ACT_HIRE_EMPLOYEES:
        /* handled outside apply_action via UI_MODE_HIRE_ROLE */
        break;

    case ACT_EXPAND_TAVERN: {
        float expand_cost =
            TAVERN_EXPAND_BASE_COST * b->tavern_size * k->inflation_rate;
        if (b->money >= expand_cost) {
            b->money -= expand_cost;
            b->tavern_size++;
            log_message(&w->log, "You expanded the tavern.", LOG_INFO);
        } else {
            log_message(&w->log,
                        "You don't have enough money to expand the tavern.",
                        LOG_INFO);
        }
        break;
    }
    case ACT_WATER_BOWL_OUTSIDE: {
        b->is_water_bowl_outside = 1;
        b->last_water_bowl_day = w->day;
    }
    case ACT_SET_RELIGION:
        break;
    }
}

static void process_wages(World *w, Tavern *b)
{
    int i;
    float total_paid_to_employees = 0.0f;
    char buf_e[256];

    if (b->employee_count <= 0 || !b->employees)
        return;

    for (i = 0; i < b->employee_count; i++) {
        float wage = (float)b->employees[i].wage_cents / 100.0f;
        b->money -= wage;
        total_paid_to_employees += wage;
    }
    tavern_snprintf(buf_e, sizeof(buf_e),
                    "Wage paid to employees in total $%.2f",
                    total_paid_to_employees);
    log_message(&w->log, buf_e, LOG_IMPORTANT);
}

void process_payment(Kingdom *k, World *w, Tavern *b, int current_day)
{
    PeriodicPayment *p = &b->rent;

    if (current_day >= p->next_payment_day) {
        float actual_rent = p->base_rent * k->inflation_rate;
        char buf[256];
        b->money -= actual_rent;
        p->next_payment_day += p->pay_period;
        tavern_snprintf(buf, sizeof(buf), "Paid rent: $%.2f", actual_rent);
        log_message(&w->log, buf, LOG_IMPORTANT);
    }

    if (current_day >= p->next_wage_day) {
        process_wages(w, b);
        p->next_wage_day += p->pay_period;
    }
}

/* Everything that happens once per day regardless of how many
   kingdoms/towns/taverns exist: population growth/aging, inflation,
   random events, and every merchant's price drift. Taverns sharing a
   merchant must not each re-roll its prices, so this updates the
   merchant pool directly instead of going through whichever tavern
   happens to call it. */
static void town_population_grow(Town *t, MessageLog *log)
{
    int new_citizens = (int)(frand() * 5.0f) + 1;
    int j;

    for (j = 0; j < new_citizens; j++)
        citizen_spawn(&t->population);
    population_tick(&t->population, log);
}

static void town_merchants_update(Town *t, float inflation_rate,
                                  float inflation_growth)
{
    int m;

    for (m = 0; m < t->merchant_count; m++)
        update_merchant(&t->merchants[m], inflation_rate, inflation_growth);
}

static void world_tick(World *w)
{
    int ki, ti;

    for (ki = 0; ki < w->kingdom_count; ki++) {
        Kingdom *k = &w->kingdoms[ki];
        float inflation_growth;

        for (ti = 0; ti < k->town_count; ti++)
            town_population_grow(&k->towns[ti], &w->log);

        inflation_growth = inflation_tick(k);

        /* Update kingdom majority religion */
        k->religion_id = kingdom_get_majority_religion(k);

        for (ti = 0; ti < k->town_count; ti++)
            town_merchants_update(&k->towns[ti], k->inflation_rate,
                                  inflation_growth);

        if (k->at_war && w->day >= k->war_end_day) {
            k->at_war = 0;
            if (frand() < 0.5f) { /* They win */
                int t, c;
                for (t = 0; t < k->town_count; t++) {
                    for (c = 0; c < k->towns[t].population.count; c++) {
                        Citizen *cit = &k->towns[t].population.citizens[c];
                        if (cit->alive && frand() < 0.3f &&
                            k->religion_id != -1)
                            cit->religion_id = k->religion_id;
                    }
                }
            } else { /* They lose */
                int t, c, new_rel = rand() % RELIGION_COUNT;
                for (t = 0; t < k->town_count; t++) {
                    for (c = 0; c < k->towns[t].population.count; c++) {
                        Citizen *cit = &k->towns[t].population.citizens[c];
                        if (cit->alive && frand() < 0.3f)
                            cit->religion_id = new_rel;
                    }
                }
            }
        }
    }

    /* Update relationships based on religion */
    for (ki = 0; ki < w->kingdom_count; ki++) {
        int kj;
        Kingdom *k1 = &w->kingdoms[ki];
        for (kj = ki + 1; kj < w->kingdom_count; kj++) {
            Kingdom *k2 = &w->kingdoms[kj];
            if (k1->religion_id != k2->religion_id && k1->religion_id != -1 &&
                k2->religion_id != -1) {
                k1->relationship_points[kj] -= 1;
                k2->relationship_points[ki] -= 1;
            }

            if (k1->relationship_points[kj] <= -50 && !k1->at_war &&
                !k2->at_war) {
                if (frand() < 0.05f) { /* slight chance */
                    event_war(k1, w);  /* k1 attacks k2 (simplified) */
                }
            }
        }
    }

    random_event(w);
}

/* Post-market bookkeeping for one tavern: reputation/consistency
   tracking against that day's DayResult. Assumes market_simulate_all()
   already ran for the day (it needs every tavern's price/stock/
   pathway state settled first, since citizens are choosing between
   taverns, not visiting each independently). */
static void tavern_post_market(Tavern *b, const DayResult *day)
{
    int d;
    int sales_today;
    float price_change;

    b->quality_actual = b->supplier->quality;

    /* Consistency punishes wild price changes */
    for (d = 0; d < DRINK_COUNT; d++) {
        price_change = (float)fabs(b->drinks[d].price - b->last_drink_price[d]);
        b->consistency -= price_change * 0.5f;
        b->last_drink_price[d] = b->drinks[d].price;
    }

    b->quality_perceived = CLAMP(b->quality_perceived, 0, 1);
    b->rumor = CLAMP(b->rumor, 0, 1);
    b->consistency = CLAMP(b->consistency, 0, 1);

    sales_today = 0;
    for (d = 0; d < DRINK_COUNT; d++)
        sales_today += day->sales[d];
    tavern_recompute_total_inventory(b);
    reputation_tick(b, sales_today);
}

#define AI_SUPPLIER_RECONSIDER_CHANCE 0.1f
#define AI_SUPPLIER_SWITCH_THRESHOLD 0.05f
#define AI_SUPPLIER_WEIGHT_QUALITY 1.0f
#define AI_SUPPLIER_WEIGHT_PRICE 0.6f
#define AI_SUPPLIER_WEIGHT_FAVOR 0.3f
#define AI_SUPPLIER_WEIGHT_RISK 0.4f

/* Higher is more attractive. Price is normalized against avg_ale_price
   (the pool's average ale price) so it's comparable across merchants
   regardless of ale's raw price scale. */
static float supplier_score(const Merchant *m, Tavern *b, float avg_ale_price)
{
    float price_ratio =
        avg_ale_price > 0.0f
            ? merchant_quote_price(m, b, DRINK_ALE) / avg_ale_price
            : 1.0f;
    float favor = (b && b->id >= 0 && b->id < MAX_TAVERNS)
                      ? m->tavern_favor[b->id]
                      : 0.0f;
    return m->quality * AI_SUPPLIER_WEIGHT_QUALITY -
           price_ratio * AI_SUPPLIER_WEIGHT_PRICE +
           favor * AI_SUPPLIER_WEIGHT_FAVOR -
           m->instability * AI_SUPPLIER_WEIGHT_RISK;
}

/* Rarely (not every day, to avoid thrashing), compares every merchant
   in b's town's pool against the current supplier and switches if a
   clearly better deal exists. */
static void ai_tavern_reconsider_supplier(Tavern *b, Town *t, World *w)
{
    int i;
    float avg_ale_price;
    float current_score;
    int best_id;
    float best_score;

    if (frand() >= AI_SUPPLIER_RECONSIDER_CHANCE || t->merchant_count <= 1)
        return;

    avg_ale_price = 0.0f;
    for (i = 0; i < t->merchant_count; i++)
        avg_ale_price += t->merchants[i].drink_price[DRINK_ALE];
    avg_ale_price /= t->merchant_count;

    current_score = supplier_score(b->supplier, b, avg_ale_price);
    best_id = b->supplier_id;
    best_score = current_score;
    for (i = 0; i < t->merchant_count; i++) {
        float s = supplier_score(&t->merchants[i], b, avg_ale_price);
        if (s > best_score) {
            best_score = s;
            best_id = i;
        }
    }

    if (best_id != b->supplier_id &&
        best_score - current_score > AI_SUPPLIER_SWITCH_THRESHOLD) {
        char buf[128];
        b->supplier_id = best_id;
        b->supplier = &t->merchants[best_id];
        tavern_snprintf(buf, sizeof(buf),
                        "Tavern #%d switched to a new supplier.", b->id);
        log_message(&w->log, buf, LOG_INFO);
    }
}

/* rival AI: presses a handful of the same buttons the
   player has, at random, plus a couple of heuristics apply_action
   doesn't cover (pricing and restocking) */
static void ai_tavern_decide(Tavern *b, Town *t, Kingdom *k, World *w)
{
    int d;
    float target;
    int buy;
    float cost;

    ai_tavern_reconsider_supplier(b, t, w);

    /* Track supplier cost with a randomized markup instead of a fixed price */
    for (d = 0; d < DRINK_COUNT; d++) {
        target =
            merchant_quote_price(b->supplier, b, d) * (1.5f + frand() * 0.5f);
        b->drinks[d].price += (target - b->drinks[d].price) * 0.2f;
    }

    /* restock whichever drink is running low, if affordable and in stock */
    for (d = 0; d < DRINK_COUNT; d++) {
        if (b->drinks[d].inventory.amount >= 5)
            continue;

        buy = 20;
        if (buy > merchant_available_stock(b->supplier, d))
            buy = merchant_available_stock(b->supplier, d);
        cost = buy * merchant_quote_price(b->supplier, b, d);
        if (buy > 0 && b->money >= cost) {
            b->drinks[d].inventory.amount += buy;
            b->money -= cost;
            merchant_record_purchase(b->supplier, b->id, d, buy);
        }
    }

    if (frand() < 0.4f)
        apply_action(b, ACT_CLEAN_PATHWAY, t, k, w, 0);
    if (frand() < 0.2f)
        apply_action(b, ACT_SKINCARE, t, k, w, 0);
    if (frand() < 0.2f)
        apply_action(b, ACT_TALK, t, k, w, 0);
    if (frand() < 0.1f)
        apply_action(b, ACT_CLEAN, t, k, w, 0);
    if (frand() < 0.3f)
        apply_action(b, ACT_WATER_BOWL_OUTSIDE, t, k, w, 0);
}

/* A bowl left outside dries up after a few days, same idea as
   PATHWAY_DIRTY_THRESHOLD_DAYS in pathway.c. */
#define WATER_BOWL_DRY_THRESHOLD_DAYS 3
#define CAT_THIRST_SEEK_THRESHOLD                                              \
    0.6f /* won't bother going out looking until this thirsty */
#define CAT_THIRST_RESET_ON_DRINK 0.8f
#define CAT_DRUNK_CHANCE                                                       \
    1.0f /* chance a cat that just drank booze gets drunk */

static int tavern_water_bowl_filled(const Tavern *b, int current_day)
{
    return b->is_water_bowl_outside && (current_day - b->last_water_bowl_day) <
                                           WATER_BOWL_DRY_THRESHOLD_DAYS;
}

/* AI taverns have no interactive UI, so a drunk cat causing a scene there
   is resolved on the spot instead of going through World.pending_event
   (which only the player's tavern can present). Same shape as
   ai_handle_fight/ai_handle_vomit in event.c. */
static void ai_handle_cat_trouble(Tavern *b, World *w, int tavern_id)
{
    char buf[128];

    if (rand() % 2 == 0) {
        b->handsomeness -= 0.05f;
        b->rumor -= 0.10f;
    } else {
        b->money -= 30.0f; /* pays someone to shoo it out and mop up */
    }
    b->handsomeness = CLAMP(b->handsomeness, 0.0f, 1.0f);
    b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);

    tavern_snprintf(buf, sizeof(buf),
                    "A drunk cat caused a scene at tavern #%d.", tavern_id);
    log_message(&w->log, buf, LOG_INFO);
}

/* Thirsty cats look for a filled water bowl outside; if no tavern in
   town has one, they sneak into a random tavern and steal a drink
   instead, sometimes getting drunk and causing trouble. cats_tick()
   (animals.c) already handled aging/thirst growth/lifecycle earlier in
   the day - this is the market.c-style "who visits where" pass, just
   for cats, so it needs Tavern/Kingdom, which is why it lives here
   instead of animals.c. */
static void cat_visit_tavern(Cat *c, Town *t, World *w, int is_player_town)
{
    int bowl_tavern = -1;
    int j, drink;
    Tavern *b;

    if (!c->alive || c->thirst < CAT_THIRST_SEEK_THRESHOLD ||
        t->tavern_count == 0)
        return;

    for (j = 0; j < t->tavern_count; j++) {
        if (tavern_water_bowl_filled(&t->taverns[j], w->day)) {
            bowl_tavern = j;
            break;
        }
    }

    if (bowl_tavern >= 0) {
        c->thirst = 0.0f;
        return;
    }

    /* no filled bowl anywhere in town; sneaks into a random tavern */
    j = rand() % t->tavern_count;
    b = &t->taverns[j];
    drink = (frand() < 0.7f)
                ? DRINK_ALE
                : (rand() % 2 == 0 ? DRINK_WINE_APPLE : DRINK_WINE_GRAPE);

    if (b->drinks[drink].inventory.amount <= 0)
        return;

    b->drinks[drink].inventory.amount--;
    tavern_recompute_total_inventory(b);
    c->thirst = CLAMP(c->thirst - CAT_THIRST_RESET_ON_DRINK, 0.0f, 1.0f);

    if (frand() >= CAT_DRUNK_CHANCE)
        return;

    c->drunk = 1;
    if (is_player_town && j == t->player_tavern_id) {
        if (w->pending_event == EVENT_NONE)
            event_cat_trouble(w);
        return;
    }

    ai_handle_cat_trouble(b, w, j);
}

/* thirsty cats seek a filled bowl; if none exists, they sneak into taverns */
static void cats_visit_taverns(Kingdom *k, Town *t, World *w)
{
    Animals *a = &t->cats;
    int is_player_town =
        (k->id == w->player_kingdom_id && t->id == k->player_town_id);
    int i;

    for (i = 0; i < a->count; i++)
        cat_visit_tavern(&a->cats[i], t, w, is_player_town);
}

/* compare player tavern performance against the busiest rival today */
static void log_daily_summary(Town *t, World *w, DayResult *results)
{
    char buf[160];
    int best_rival = -1;
    int best_rival_customers = -1;
    int player_customers;
    int i;

    if (t->tavern_count <= 1)
        return;

    for (i = 0; i < t->tavern_count; i++) {
        if (i == t->player_tavern_id)
            continue;
        if (results[i].customers > best_rival_customers) {
            best_rival_customers = results[i].customers;
            best_rival = i;
        }
    }
    player_customers = results[t->player_tavern_id].customers;
    tavern_snprintf(buf, sizeof(buf),
                    "Competition: you drew %d customers, tavern #%d drew %d.",
                    player_customers, best_rival, best_rival_customers);
    log_message(&w->log, buf, LOG_INFO);
}

static void tavern_daily_prep(Tavern *b, Town *t, Kingdom *k, World *w,
                              int is_player_tavern)
{
    int e;

    if (!is_player_tavern)
        ai_tavern_decide(b, t, k, w);
    process_payment(k, w, b, w->day);
    employee_cleaners_work(b, w->day, is_player_tavern ? &w->log : NULL);
    employee_cooks_work(b, is_player_tavern ? &w->log : NULL);

    for (e = 0; e < b->employee_count; e++) {
        Employee *emp = &b->employees[e];
        employee_rest(emp, emp->on_duty ? 15 : 30);
    }
}

static int simulate_town_day(Town *t, Kingdom *k, World *w, int is_player_town)
{
    DayResult results[MAX_TAVERNS] = {0};
    int sales_today = 0;
    int j, d;

    /* taverns settle state and run duties before citizens pick venues */
    for (j = 0; j < t->tavern_count; j++) {
        int is_player_tavern = is_player_town && j == t->player_tavern_id;
        tavern_daily_prep(&t->taverns[j], t, k, w, is_player_tavern);
    }

    market_simulate_all(t, w, results);

    for (j = 0; j < t->tavern_count; j++)
        tavern_post_market(&t->taverns[j], &results[j]);

    cats_tick(&t->cats, &w->log);
    cats_visit_taverns(k, t, w);

    for (j = 0; j < t->tavern_count; j++)
        evaluate_customer_events(k, t, w, j, &results[j]);

    if (is_player_town) {
        log_daily_summary(t, w, results);
        for (d = 0; d < DRINK_COUNT; d++)
            sales_today += results[t->player_tavern_id].sales[d];
    }

    return sales_today;
}

int simulate_day(World *w)
{
    int sales_today = 0;
    int ki, ti;

    world_tick(w);

    for (ki = 0; ki < w->kingdom_count; ki++) {
        Kingdom *k = &w->kingdoms[ki];

        for (ti = 0; ti < k->town_count; ti++) {
            Town *t = &k->towns[ti];
            int is_player_town =
                (k->id == w->player_kingdom_id && t->id == k->player_town_id);
            sales_today += simulate_town_day(t, k, w, is_player_town);
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

/* fresh tavern with default starting stats, supplied by merchant_id */
static Tavern make_starter_tavern(int day, int merchant_id, const Merchant *m)
{
    Tavern b = {0};
    b.money = 700.0f;
    b.drinks[DRINK_ALE].price = 5.0f;
    b.drinks[DRINK_WINE_APPLE].price = 120.0f;
    b.drinks[DRINK_WINE_GRAPE].price = 120.0f;
    b.drinks[DRINK_ALE].inventory.amount = 10;
    b.drinks[DRINK_WINE_APPLE].inventory.amount = 2;
    b.drinks[DRINK_WINE_GRAPE].inventory.amount = 2;
    b.last_drink_price[DRINK_ALE] = 1.0f;
    b.last_drink_price[DRINK_WINE_APPLE] = 1.0f;
    b.last_drink_price[DRINK_WINE_GRAPE] = 1.0f;
    b.fruits[FRUIT_APPLE].inventory.expiration_date = 30;
    b.fruits[FRUIT_GRAPE].inventory.expiration_date = 30;
    b.fruits[FRUIT_APPLE].inventory.amount = 1;
    b.fruits[FRUIT_GRAPE].inventory.amount = 1;
    b.foods[FOOD_BREAD].price = 3.0f;
    b.foods[FOOD_BREAD].inventory.amount = 5;
    b.foods[FOOD_BREAD].inventory.expiration_date = 30;
    b.foods[FOOD_STEW].price = 7.5f;
    b.foods[FOOD_STEW].inventory.amount = 3;
    b.foods[FOOD_STEW].inventory.expiration_date = 30;
    b.quality_actual = m->quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier_id = merchant_id;
    b.last_pathway_clean_day = 0;
    b.rent.pay_period = 30;
    b.rent.next_payment_day = day + b.rent.pay_period;
    b.rent.rent_amount = 1500;
    b.rent.base_rent = 1500;
    b.rent.next_wage_day = day + b.rent.pay_period;
    b.employee_count = 0;
    b.tavern_size = 1;
    b.religion_id = -1;
    return b;
}

void init_new_game(World *w)
{
    Kingdom kingdom = {0};
    Town town = {0};
    Merchant m_init = {0};
    Merchant m_rival = {0};
    Tavern b_init;
    Tavern rival;
    int merchant_id;
    int rival_merchant_id;
    int i;

    w->day = 0;

    world_kingdoms_init(w, MAX_KINGDOMS);

    kingdom.inflation_rate = 1.0f;
    kingdom.money_supply_prev = 0.0f;
    kingdom_towns_init(&kingdom, MAX_TOWNS);

    town.last_advertised_day = 0;
    town_merchants_init(&town, MAX_MERCHANTS);
    town_taverns_init(&town, MAX_TAVERNS);
    town_cats_init(&town, ANIMALS_DEFAULT_CAPACITY);
    /* seed small starter colony since cats_tick only reproduces existing pairs
     */
    for (i = 0; i < 4; i++)
        cat_spawn(&town.cats);
    population_init(&town.population, 100000);
    for (i = 0; i < 150; i++) {
        citizen_spawn(&town.population);
        if (i < RELIGION_COUNT) {
            town.population.citizens[i].religion_id = i;
        }
    }
    m_init.drink_price[DRINK_ALE] = 5.0f;
    m_init.drink_price[DRINK_WINE_APPLE] = 90.0f;
    m_init.drink_price[DRINK_WINE_GRAPE] = 90.0f;
    m_init.quality = 0.7f;
    m_init.instability = 0.2f;
    merchant_init_default_stock(&m_init);
    merchant_id = town_add_merchant(&town, m_init);

    b_init = make_starter_tavern(w->day, merchant_id, &m_init);
    town.player_tavern_id = town_add_tavern(&town, b_init);

    /* rival supplier with different quality and pricing */
    m_rival.drink_price[DRINK_ALE] = 4.5f;
    m_rival.drink_price[DRINK_WINE_APPLE] = 90.0f;
    m_rival.drink_price[DRINK_WINE_GRAPE] = 90.0f;
    m_rival.quality = 0.6f;
    m_rival.instability = 0.35f;
    merchant_init_default_stock(&m_rival);
    rival_merchant_id = town_add_merchant(&town, m_rival);

    rival = make_starter_tavern(w->day, rival_merchant_id, &m_rival);
    rival.money = 500.0f;
    town_add_tavern(&town, rival);

    town_relink_suppliers(&town);

    kingdom.player_town_id = kingdom_add_town(&kingdom, town);
    w->player_kingdom_id = world_add_kingdom(w, kingdom);
}
