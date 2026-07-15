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

void apply_action(Tavern* b, Action a, World* w, int amount) {
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
            // 24 because i tried balancing it.
			b->rumor += (CLAMP(amount / (w->population.count / 24), 0.0, 1.0));
            apply_advertisement(w->day, w);
			break;

		case ACT_BUY_ALE:
            b->ale.amount += amount;
            b->money -= amount * b->supplier->price_per_ale;
            b->total_inventory = b->ale.amount + b->wine.amount;
            break;


		case ACT_BUY_WINE:
			b->wine.amount += amount;
			b->money -= amount * b->supplier->price_per_wine;
			b->total_inventory = b->ale.amount + b->wine.amount;
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

void process_payment(World* w, PeriodicPayment* p, Tavern* b, int current_day) {
	if (current_day >= p->next_payment_day) {
		float actual_rent = p->base_rent * w->inflation_rate;
		b->money -= actual_rent;
		p->next_payment_day += p->pay_period;
        char buf[256];
        snprintf(buf, sizeof(buf), "Paid rent: $%.2f", actual_rent);
        log_message(&w->log, buf, LOG_IMPORTANT);
	}
}

int simulate_day(Tavern* b, World* w, PeriodicPayment* p) {
	// Rent Logic
	process_payment(w, p, b, w->day);
    int new_citizens = (int)(frand() * 5.0f) + 1;
    for (int i = 0; i < new_citizens; i++) citizen_spawn(&w->population);
    population_tick(w);
    // Should inflation_tick(w); be exactly here? not sure...
	inflation_tick(w);
	random_event(w);
	DayResult day = market_simulate(b, w);
	update_merchant(b->supplier, w->inflation_rate);

	b->quality_actual = b->supplier->quality;


	// Consistency punishes wild price changes
	static float ale_last_price = 1.0f;
	float ale_price_change = fabsf(b->ale_price - ale_last_price);
	b->consistency -= ale_price_change * 0.5f;
	ale_last_price = b->ale_price;


	static float wine_last_price = 1.0f;
	float wine_price_change = fabsf(b->wine_price - wine_last_price);
	b->consistency -= wine_price_change * 0.5f;
	wine_last_price = b->wine_price;

	b->quality_perceived = CLAMP(b->quality_perceived, 0, 1);
	b->rumor = CLAMP(b->rumor, 0, 1);
	b->consistency = CLAMP(b->consistency, 0, 1);

	int sales_today = day.ale_sales + day.wine_sales;
	b->total_inventory = b->ale.amount + b->wine.amount;
	reputation_tick(b, sales_today);

	// int days_since_clean =
	// 	w->day - b->last_pathway_clean_day;

	// if (days_since_clean >= 7) {
	// 	return 0;
	// }

	// THIS... is important.
	// we only make w->day go up here,
	// then we have local variables
	// that just store its value locally
	w->day++;

	// return day.ale_sales;
	// return day.wine_sales;
    int total_sales = (day.ale_sales + day.wine_sales);
    return total_sales;
}
