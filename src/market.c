/*
 *
 * market.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "market.h"
#include "advertisement.h"
#include "pathway.h"
#include "religion.h"
#include "sim.h"
#include "sim_random.h"
#include <stdio.h>

/* Weights for a citizen's desire to go out drinking at all today */
#define DESIRE_THIRST_WEIGHT 0.5f
#define DESIRE_ADDICTION_WEIGHT 0.3f
#define DESIRE_LOYALTY_WEIGHT 0.2f
/* The longer since their last drink, the more that desire is nudged up */
#define DROUGHT_BONUS_PER_DAY 0.02f
#define DROUGHT_BONUS_CAP 0.3f

/* Won't spend more than this fraction of current wealth on one drink.
   Either wine variety costs far more than ale, so citizens treat wine
   as an occasional splurge instead of judging it by the same everyday
   budget as ale. */
static const float MAX_SPEND_FRACTION_OF_WEALTH[DRINK_COUNT] = {0.20f, 0.50f,
                                                                0.50f};

/* Bonus score for sticking with your favorite tavern over a new one */
#define FAVORITE_TAVERN_BONUS 0.3f

/* What a successful visit does to a citizen */
#define THIRST_RESET_ON_VISIT 0.7f
#define ADDICTION_GAIN_PER_VISIT 0.03f
#define LOYALTY_GAIN_PER_VISIT 0.1f
#define LOYALTY_ON_NEW_FAVORITE 0.1f
#define ANGER_GAIN_PER_VISIT 0.02f /* heavy drinking still stokes it a bit */
#define ANGER_GAIN_PER_PATHWAY_FALL                                            \
    0.08f /* tripping on a filthy pathway stokes it more */

/* A visitor counts as "rowdy" (fight/vomit risk) or "destitute" (theft
   risk) based on their own stats, not chance. See evaluate_customer_events. */
#define ROWDY_ANGER_THRESHOLD 0.6f
#define DESTITUTE_WEALTH_THRESHOLD 15.0f

/* Decide whether citizen c wants to go out at all today, ignoring
   advertising and which tavern. Callers apply the ads gate and pick
   a tavern separately. */
static int citizen_wants_to_go_out(Citizen *c, int current_day)
{
    float desire;

    if (!c->alive)
        return 0;

    desire = c->thirst * DESIRE_THIRST_WEIGHT +
             c->addiction * DESIRE_ADDICTION_WEIGHT +
             c->loyalty * DESIRE_LOYALTY_WEIGHT;

    if (c->last_drink_day >= 0) {
        int days_since = current_day - c->last_drink_day;
        desire +=
            CLAMP(days_since * DROUGHT_BONUS_PER_DAY, 0.0f, DROUGHT_BONUS_CAP);
    }

    return frand() < CLAMP(desire, 0.0f, 1.0f);
}

/* Picks the best affordable (tavern, drink) combo for c, skipping any
   tavern whose dirty pathway turns this particular citizen away.
   Returns 1 and fills out_tavern/out_drink on success. */
static int citizen_pick_tavern(Citizen *c, Town *town, int current_day,
                               int *out_tavern, int *out_drink,
                               int *pathway_losses)
{
    int best_tavern = -1;
    int best_drink = DRINK_ALE;
    float best_score = -1.0f;
    int t;

    for (t = 0; t < town->tavern_count; t++) {
        Tavern *b = &town->taverns[t];
        float pathway_loss = people_fall_because_pathway_dirty(b, current_day);
        int d;

        if (pathway_loss > 0.0f && frand() < pathway_loss) {
            pathway_losses[t]++;
            c->anger =
                CLAMP(c->anger + ANGER_GAIN_PER_PATHWAY_FALL, 0.0f, 1.0f);
            continue;
        }

        for (d = 0; d < DRINK_COUNT; d++) {
            float score;

            if (b->drinks[d].price <= 0.0f)
                continue;
            if (b->drinks[d].price >
                c->wealth * MAX_SPEND_FRACTION_OF_WEALTH[d])
                continue;
            if (b->drinks[d].inventory.amount <= 0)
                continue;

            score = b->reputation * c->drink_preference[d];
            if (t == c->favorite_tavern_id)
                score += c->loyalty * FAVORITE_TAVERN_BONUS;

            if (b->religion_id != -1) {
                if (c->religion_id == b->religion_id)
                    score *= 1.5f; /* bonus for same religion */
                else if (c->religion_id != -1)
                    score *= 0.5f; /* penalty for different religion */
            }

            if (score > best_score) {
                best_score = score;
                best_tavern = t;
                best_drink = d;
            }
        }
    }

    if (best_tavern < 0)
        return 0;
    *out_tavern = best_tavern;
    *out_drink = best_drink;
    return 1;
}

static void citizen_visit(Citizen *c, Town *town, int tavern_idx, int drink,
                          int current_day, DayResult *r)
{
    Tavern *b = &town->taverns[tavern_idx];

    b->drinks[drink].inventory.amount--;

    float actual_price = b->drinks[drink].price;
    if (b->religion_id != -1) {
        actual_price = modifier_get_total(
            MOD_MERCHANT_SELL_PRICE, g_religions[b->religion_id].modifiers,
            g_religions[b->religion_id].modifier_count, actual_price);
    }

    b->money += actual_price;

    r->sales[drink]++;
    r->revenue += actual_price;

    c->last_drink_day = current_day;
    c->thirst = CLAMP(c->thirst - THIRST_RESET_ON_VISIT, 0.0f, 1.0f);
    c->addiction = CLAMP(c->addiction + ADDICTION_GAIN_PER_VISIT, 0.0f, 1.0f);

    if (tavern_idx == c->favorite_tavern_id) {
        c->loyalty = CLAMP(c->loyalty + LOYALTY_GAIN_PER_VISIT, 0.0f, 1.0f);
    } else {
        c->favorite_tavern_id = tavern_idx;
        c->loyalty = LOYALTY_ON_NEW_FAVORITE;
    }

    c->anger = CLAMP(c->anger + ANGER_GAIN_PER_VISIT, 0.0f, 1.0f);

    if (c->anger > ROWDY_ANGER_THRESHOLD)
        r->rowdy_visitors++;
    if (c->homeless || c->wealth < DESTITUTE_WEALTH_THRESHOLD)
        r->destitute_visitors++;
}

static void citizen_order_food(Citizen *c, Tavern *b, DayResult *r,
                               int *deliveries_done, int waiter_cap)
{
    int f;

    /* patrons with enough wealth occasionally order food if a waiter can
     * deliver */
    if (*deliveries_done >= waiter_cap || c->wealth < 3.0f || frand() > 0.5f)
        return;

    /* prefer stew if affordable and available, otherwise bread */
    if (b->foods[FOOD_STEW].inventory.amount > 0 &&
        c->wealth >= b->foods[FOOD_STEW].price &&
        b->foods[FOOD_STEW].price > 0.0f) {
        f = FOOD_STEW;
    } else if (b->foods[FOOD_BREAD].inventory.amount > 0 &&
               c->wealth >= b->foods[FOOD_BREAD].price &&
               b->foods[FOOD_BREAD].price > 0.0f) {
        f = FOOD_BREAD;
    } else {
        return;
    }

    b->foods[f].inventory.amount--;
    b->money += b->foods[f].price;
    r->revenue += b->foods[f].price;
    r->food_sales[f]++;
    c->wealth -= b->foods[f].price;
    c->anger = CLAMP(c->anger - 0.05f, 0.0f, 1.0f);
    c->loyalty = CLAMP(c->loyalty + 0.05f, 0.0f, 1.0f);
    (*deliveries_done)++;
}

void market_simulate_all(Town *town, World *w, DayResult *results)
{
    float ads_loss_fraction;
    int lost_to_ads = 0;
    int lost_to_pathway[MAX_TAVERNS] = {0};
    int bartender_cap[MAX_TAVERNS] = {0};
    int waiter_cap[MAX_TAVERNS] = {0};
    int drinks_served[MAX_TAVERNS] = {0};
    int deliveries_done[MAX_TAVERNS] = {0};
    int lost_to_bartender[MAX_TAVERNS] = {0};
    int lost_to_waiter[MAX_TAVERNS] = {0};
    /* TODO: refactor this. unreadable */
    int t;
    int d;
    int f;
    int i;

    for (t = 0; t < town->tavern_count; t++) {
        results[t].customers = 0;
        results[t].revenue = 0.0f;
        results[t].rowdy_visitors = 0;
        results[t].destitute_visitors = 0;
        for (d = 0; d < DRINK_COUNT; d++) {
            results[t].sales[d] = 0;
            results[t].demand[d] = 0;
        }
        for (f = 0; f < FOOD_COUNT; f++)
            results[t].food_sales[f] = 0;

        bartender_cap[t] = employee_get_bartender_capacity(&town->taverns[t]);
        waiter_cap[t] = employee_get_waiter_capacity(&town->taverns[t]);
    }

    ads_loss_fraction = no_customers_because_no_ads(w->day, town);

    for (i = 0; i < town->population.count; i++) {
        Citizen *c = &town->population.citizens[i];
        int tavern_idx, drink;

        if (!c->alive)
            continue;

        if (!citizen_wants_to_go_out(c, w->day))
            continue;

        /* Track ad-driven drop-off separately so the log message stays accurate
         */
        if (ads_loss_fraction > 0.0f && frand() < ads_loss_fraction) {
            lost_to_ads++;
            continue;
        }

        if (!citizen_pick_tavern(c, town, w->day, &tavern_idx, &drink,
                                 lost_to_pathway))
            continue;

        results[tavern_idx].demand[drink]++;

        /* bartender and waiter staffing constraints */
        if (drinks_served[tavern_idx] >= bartender_cap[tavern_idx]) {
            lost_to_bartender[tavern_idx]++;
            c->anger = CLAMP(c->anger + 0.15f, 0.0f, 1.0f);
            continue;
        }

        if (deliveries_done[tavern_idx] >= waiter_cap[tavern_idx]) {
            lost_to_waiter[tavern_idx]++;
            c->anger = CLAMP(c->anger + 0.10f, 0.0f, 1.0f);
            continue;
        }

        results[tavern_idx].customers++;
        drinks_served[tavern_idx]++;
        deliveries_done[tavern_idx]++;
        citizen_visit(c, town, tavern_idx, drink, w->day, &results[tavern_idx]);
        citizen_order_food(c, &town->taverns[tavern_idx], &results[tavern_idx],
                           &deliveries_done[tavern_idx],
                           waiter_cap[tavern_idx]);
    }

    if (lost_to_ads > 0) {
        char buf[128];
        tavern_snprintf(
            buf, sizeof(buf),
            "%d townsfolk stayed home - nobody's advertised in a while",
            lost_to_ads);
        log_message(&w->log, buf, LOG_WARN);
    }
    for (t = 0; t < town->tavern_count; t++) {
        if (lost_to_pathway[t] > 0) {
            char buf[128];
            tavern_snprintf(buf, sizeof(buf),
                            "%d customers turned back at tavern #%d - the "
                            "pathway is too dirty",
                            lost_to_pathway[t], t);
            log_message(&w->log, buf, LOG_WARN);
        }
        if (t == town->player_tavern_id) {
            if (lost_to_bartender[t] > 0) {
                char buf[128];
                tavern_snprintf(
                    buf, sizeof(buf),
                    "%d customers could not get drinks - need more bartenders!",
                    lost_to_bartender[t]);
                log_message(&w->log, buf, LOG_WARN);
            }
            if (lost_to_waiter[t] > 0) {
                char buf[128];
                tavern_snprintf(
                    buf, sizeof(buf),
                    "%d customers left without drinks - need more waiters!",
                    lost_to_waiter[t]);
                log_message(&w->log, buf, LOG_WARN);
            }
        }
    }
}
