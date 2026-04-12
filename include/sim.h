#ifndef SIM_H
#define SIM_H

#include "merchant.h"
#include "log.h"
#include "game_state.h"
#include <math.h>

#define CLAMP(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))

typedef struct PeriodicPayment {
    int pay_period;
    int next_payment_day;
    float rent_amount;
} PeriodicPayment;

typedef struct Inventory {
    int amount;
    int expiration_date;
} Inventory;

typedef struct Tavern{
    float money;
    float ale_price;
    float wine_price;

    int total_inventory;
    Inventory ale;
    Inventory wine;

    float quality_actual;
    float quality_perceived;

    float rumor;
    float consistency;
    float handsomeness;

    float reputation;

    int last_pathway_clean_day;
    
    PeriodicPayment rent;
    
    Merchant* supplier;
} Tavern;

typedef struct {
    int customers;
    int ale_sales;
    int wine_sales;
    int demand_ale;
    int demand_wine;
    float revenue;
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

void process_payment(World* w, PeriodicPayment* p, Tavern* b, int current_day);

/* Simulate one day: sales, inventory, money, reputation dynamics */
int simulate_day(Tavern* b, World* w, PeriodicPayment* p);

#endif /* SIM_H */
