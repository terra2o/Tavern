#include <math.h>
#include "../include/reputation.h"
#include "../include/sim.h"
#include "../include/sim_random.h"

#define EMPTY_INVENTORY_RUMOR_PENALTY 0.05f
#define QUALITY_PERCEIVED_CATCHUP_RATE 0.1f
#define RUMOR_NOISE 0.1f
#define HIGH_SALES_THRESHOLD 20
#define HIGH_SALES_RUMOR_BONUS 0.02f
/* Realized that you can just make something cost 50
   and keep spamming and you wouldn't really see any
   negative results, so I'm making this one higher */
#define LOW_SALES_THRESHOLD 5
#define LOW_SALES_RUMOR_PENALTY 0.10f

/* Reputation weights, must sum to 1.0 */
#define REPUTATION_WEIGHT_QUALITY 0.4f
#define REPUTATION_WEIGHT_RUMOR 0.3f
#define REPUTATION_WEIGHT_CONSISTENCY 0.2f
#define REPUTATION_WEIGHT_HANDSOMENESS 0.1f

/* Floor instead of 0, to avoid a reputation of exactly 0 causing issues */
#define REPUTATION_FLOOR 0.1f

void reputation_tick(Tavern* b, int sales_today)
{
    // Inventory affects rumor
    if (b->total_inventory == 0)
        b->rumor -= EMPTY_INVENTORY_RUMOR_PENALTY;

    // Quality perception slowly follows reality
    b->quality_perceived +=
        (b->quality_actual - b->quality_perceived) * QUALITY_PERCEIVED_CATCHUP_RATE;

    // Rumors are noisy
    b->rumor += (frand() - 0.5f) * RUMOR_NOISE;

    // Sales affect rumor
    if (sales_today > HIGH_SALES_THRESHOLD) b->rumor += HIGH_SALES_RUMOR_BONUS;
    if (sales_today < LOW_SALES_THRESHOLD) b->rumor -= LOW_SALES_RUMOR_PENALTY;

    // Clamp values
    b->quality_perceived = fminf(fmaxf(b->quality_perceived, 0.0f), 1.0f);
    b->rumor = fminf(fmaxf(b->rumor, 0.0f), 1.0f);
    b->consistency = fminf(fmaxf(b->consistency, 0.0f), 1.0f);

    // Recompute reputation
    b->reputation = REPUTATION_WEIGHT_QUALITY * b->quality_perceived +
                    REPUTATION_WEIGHT_RUMOR * b->rumor +
                    REPUTATION_WEIGHT_CONSISTENCY * b->consistency +
                    REPUTATION_WEIGHT_HANDSOMENESS * b->handsomeness;

    if (b->reputation < 0) b->reputation = REPUTATION_FLOOR;
    if (b->reputation > 1) b->reputation = 1.0f;
}
