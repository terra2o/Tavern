#ifndef MERCHANT_H
#define MERCHANT_H

#include <math.h>
#include "drink.h"
#include "game_state.h"

typedef struct Merchant{
	float drink_price[DRINK_COUNT];
	float quality;
	float instability;   /* supply risk: chance restock underperforms today */

	float stock[DRINK_COUNT];
	float restock_rate[DRINK_COUNT];
	float max_stock[DRINK_COUNT];

	float tavern_favor[MAX_TAVERNS]; /* per-tavern relationship, 0.0 to 1.0 */
} Merchant;

/* One day of drift: restocks stock[] toward max_stock[] (less reliably
   the higher instability is), derives a scarcity-driven price target
   per drink and eases price toward it, jiggles quality, and decays
   tavern_favor[] slightly for taverns that didn't buy today. */
void update_merchant(Merchant* m, float inflation_rate);

/* Units of drink d this merchant can currently sell. */
int merchant_available_stock(const Merchant* m, DrinkType d);

/* Price tavern_id actually pays for drink d, after its favor discount. */
float merchant_quote_price(const Merchant* m, int tavern_id, DrinkType d);

/* Records a completed purchase: drains stock[d] by qty and grows
   tavern_favor[tavern_id]. Call after a purchase succeeds. */
void merchant_record_purchase(Merchant* m, int tavern_id, DrinkType d, int qty);

/* Fills stock/max_stock/restock_rate with the default balancing values.
   Call once when creating a new Merchant (favor/prices are set by the
   caller separately). */
void merchant_init_default_stock(Merchant* m);

#endif