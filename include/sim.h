#ifndef SIM_H
#define SIM_H

#include "merchant.h"
#include "log.h"
#include "game_state.h"
#include "drink.h"
#include <math.h>

#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))

typedef struct PeriodicPayment {
    int pay_period;
    int next_payment_day;
    float rent_amount;
    float base_rent; // Original rent at game start; actual charge = base_rent * inflation_rate
} PeriodicPayment;

typedef struct Tavern{
    /* Index of this tavern in World.taverns[]; used to look itself up
       in a Merchant's per-tavern arrays (favor, etc). Derived from
       load/creation order, not itself persisted in the save file. */
    int id;

    float money;

    int total_inventory;
    Drink drinks[DRINK_COUNT];

    float quality_actual;
    float quality_perceived;

    float rumor;
    float consistency;
    float handsomeness;

    float reputation;

    /* previous day's price per drink, for the consistency penalty on
       wild price swings. Lives on the tavern (not a function-static)
       so multiple taverns each track their own price history. */
    float last_drink_price[DRINK_COUNT];

    int last_pathway_clean_day;

    PeriodicPayment rent;

    /* supplier_id is the source of truth (used for save/load and
       re-pointing after the merchant pool is (re)allocated); supplier
       is a cached pointer into World.merchants for fast access. */
    int supplier_id;
    Merchant* supplier;
} Tavern;

typedef struct {
    int customers;
    int sales[DRINK_COUNT];
    int demand[DRINK_COUNT];
    float revenue;

    /* Who showed up, for event triggers: counts of visitors whose own
       stats make trouble more likely, not a die roll. */
    int rowdy_visitors;      /* addiction above ROWDY_ADDICTION_THRESHOLD */
    int destitute_visitors;  /* homeless, or wealth below DESTITUTE_WEALTH_THRESHOLD */
} DayResult;

typedef enum {
	ACT_SKINCARE,
	ACT_CLEAN,
	ACT_TALK,
	ACT_CHECK_QUALITY,
	ACT_ADVERTISE,
    ACT_CLEAN_PATHWAY,
	ACT_BUY_ALE,
    ACT_BUY_WINE,
	ACT_ADJUST_ALE_PRICE,
    ACT_ADJUST_WINE_PRICE,
} Action;

/* Compute reputation from quality, rumor, consistency, handsomeness */
float compute_reputation(Tavern* b);

/* Apply an action to the tavern state */
void apply_action(Tavern* b, Action a, World* w, int amount);

/* Charges b->rent if due */
void process_payment(World* w, Tavern* b, int current_day);

/* Simulate one day for every tavern in the world (player's and AI
   rivals alike). Returns the player's tavern's total drinks sold. */
int simulate_day(World* w);

/* --- Tavern/Merchant pools (World owns these; taverns can be player-run
   or AI-run competitors sharing the same population) --- */

void world_taverns_init(World* w, int capacity);
void world_taverns_free(World* w);
/* Copies t into the pool and returns its index, or -1 if the pool is full */
int world_add_tavern(World* w, Tavern t);

void world_merchants_init(World* w, int capacity);
void world_merchants_free(World* w);
/* Copies m into the pool and returns its index, or -1 if the pool is full */
int world_add_merchant(World* w, Merchant m);

/* Re-point every tavern's b->supplier at w->merchants[b->supplier_id].
   Call after loading a save, since the merchant pool is freshly malloc'd. */
void world_relink_suppliers(World* w);

#endif /* SIM_H */
