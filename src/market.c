#include "../include/sim.h"
#include "../include/market.h"
#include "../include/sim_random.h"
#include "../include/advertisement.h"
#include "../include/pathway.h"
#include <stdio.h>

/* Weights for a citizen's desire to go out drinking at all today */
#define DESIRE_THIRST_WEIGHT 0.5f
#define DESIRE_ADDICTION_WEIGHT 0.3f
#define DESIRE_LOYALTY_WEIGHT 0.2f
/* The longer since their last drink, the more that desire is nudged up */
#define DROUGHT_BONUS_PER_DAY 0.02f
#define DROUGHT_BONUS_CAP 0.3f

/* Won't spend more than this fraction of current wealth on one drink.
   Wine costs far more than ale, so citizens treat it as an occasional
   splurge instead of judging it by the same everyday budget as ale. */
static const float MAX_SPEND_FRACTION_OF_WEALTH[DRINK_COUNT] = { 0.20f, 0.50f };

/* Bonus score for sticking with your favorite tavern over a new one */
#define FAVORITE_TAVERN_BONUS 0.3f

/* What a successful visit does to a citizen */
#define THIRST_RESET_ON_VISIT 0.7f
#define ADDICTION_GAIN_PER_VISIT 0.03f
#define LOYALTY_GAIN_PER_VISIT 0.1f
#define LOYALTY_ON_NEW_FAVORITE 0.1f

/* A visitor counts as "rowdy" (fight/vomit risk) or "destitute" (theft
   risk) based on their own stats, not chance. See evaluate_customer_events. */
#define ROWDY_ADDICTION_THRESHOLD 0.6f
#define DESTITUTE_WEALTH_THRESHOLD 15.0f

/* Decide whether citizen c wants to go out at all today, ignoring
   advertising and which tavern. Callers apply the ads gate and pick
   a tavern separately. */
static int citizen_wants_to_go_out(Citizen* c, int current_day)
{
    if (!c->alive) return 0;

    float desire = c->thirst * DESIRE_THIRST_WEIGHT
                 + c->addiction * DESIRE_ADDICTION_WEIGHT
                 + c->loyalty * DESIRE_LOYALTY_WEIGHT;

    if (c->last_drink_day >= 0) {
        int days_since = current_day - c->last_drink_day;
        desire += CLAMP(days_since * DROUGHT_BONUS_PER_DAY, 0.0f, DROUGHT_BONUS_CAP);
    }

    return frand() < CLAMP(desire, 0.0f, 1.0f);
}

/* Picks the best affordable (tavern, drink) combo for c, skipping any
   tavern whose dirty pathway turns this particular citizen away.
   Returns 1 and fills out_tavern/out_drink on success. */
static int citizen_pick_tavern(Citizen* c, World* w, int current_day,
                                int* out_tavern, int* out_drink, int* pathway_losses)
{
    int best_tavern = -1;
    int best_drink = DRINK_ALE;
    float best_score = -1.0f;

    for (int t = 0; t < w->tavern_count; t++) {
        Tavern* b = &w->taverns[t];

        float pathway_loss = people_fall_because_pathway_dirty(b, current_day);
        if (pathway_loss > 0.0f && frand() < pathway_loss) {
            pathway_losses[t]++;
            continue;
        }

        for (int d = 0; d < DRINK_COUNT; d++) {
            if (b->drinks[d].price <= 0.0f) continue;
            if (b->drinks[d].price > c->wealth * MAX_SPEND_FRACTION_OF_WEALTH[d]) continue;
            if (b->drinks[d].inventory.amount <= 0) continue;

            float score = b->reputation * c->drink_preference[d];
            if (t == c->favorite_tavern_id) score += c->loyalty * FAVORITE_TAVERN_BONUS;

            if (score > best_score) {
                best_score = score;
                best_tavern = t;
                best_drink = d;
            }
        }
    }

    if (best_tavern < 0) return 0;
    *out_tavern = best_tavern;
    *out_drink = best_drink;
    return 1;
}

static void citizen_visit(Citizen* c, World* w, int tavern_idx, int drink, int current_day, DayResult* r)
{
    Tavern* b = &w->taverns[tavern_idx];

    b->drinks[drink].inventory.amount--;
    b->money += b->drinks[drink].price;

    r->sales[drink]++;
    r->revenue += b->drinks[drink].price;

    c->last_drink_day = current_day;
    c->thirst = CLAMP(c->thirst - THIRST_RESET_ON_VISIT, 0.0f, 1.0f);
    c->addiction = CLAMP(c->addiction + ADDICTION_GAIN_PER_VISIT, 0.0f, 1.0f);

    if (tavern_idx == c->favorite_tavern_id) {
        c->loyalty = CLAMP(c->loyalty + LOYALTY_GAIN_PER_VISIT, 0.0f, 1.0f);
    } else {
        c->favorite_tavern_id = tavern_idx;
        c->loyalty = LOYALTY_ON_NEW_FAVORITE;
    }

    if (c->addiction > ROWDY_ADDICTION_THRESHOLD) r->rowdy_visitors++;
    if (c->homeless || c->wealth < DESTITUTE_WEALTH_THRESHOLD) r->destitute_visitors++;
}

void market_simulate_all(World* w, DayResult* results)
{
    for (int t = 0; t < w->tavern_count; t++) {
        results[t].customers = 0;
        results[t].revenue = 0.0f;
        results[t].rowdy_visitors = 0;
        results[t].destitute_visitors = 0;
        for (int d = 0; d < DRINK_COUNT; d++) {
            results[t].sales[d] = 0;
            results[t].demand[d] = 0;
        }
    }

    float ads_loss_fraction = no_customers_because_no_ads(w->day, w);

    int lost_to_ads = 0;
    int lost_to_pathway[MAX_TAVERNS] = {0};

    for (int i = 0; i < w->population.count; i++) {
        Citizen* c = &w->population.citizens[i];
        if (!c->alive) continue;

        if (!citizen_wants_to_go_out(c, w->day)) continue;

        /* Track ad-driven drop-off separately so the log message stays accurate */
        if (ads_loss_fraction > 0.0f && frand() < ads_loss_fraction) {
            lost_to_ads++;
            continue;
        }

        int tavern_idx, drink;
        if (!citizen_pick_tavern(c, w, w->day, &tavern_idx, &drink, lost_to_pathway))
            continue;

        results[tavern_idx].customers++;
        results[tavern_idx].demand[drink]++;
        citizen_visit(c, w, tavern_idx, drink, w->day, &results[tavern_idx]);
    }

    if (lost_to_ads > 0) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%d townsfolk stayed home - nobody's advertised in a while", lost_to_ads);
        log_message(&w->log, buf, LOG_WARN);
    }
    for (int t = 0; t < w->tavern_count; t++) {
        if (lost_to_pathway[t] > 0) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%d customers turned back at tavern #%d - the pathway is too dirty", lost_to_pathway[t], t);
            log_message(&w->log, buf, LOG_WARN);
        }
    }
}
