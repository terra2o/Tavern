#include <math.h>
#include "reputation.h"
#include "sim.h"
#include "sim_random.h"

// TODO: reputation gets stuck when it's 0, fix that.

void reputation_tick(Tavern* b, int sales_today) {
    // Inventory affects rumor
    if (b->total_inventory == 0)
        b->rumor -= 0.05f;

    // Quality perception slowly follows reality
    b->quality_perceived +=
        (b->quality_actual - b->quality_perceived) * 0.1f;

    // Rumors are noisy
    b->rumor += (frand() - 0.5f) * 0.1f;

    // Sales affect rumor
    if (sales_today > 80) b->rumor += 0.02f;
    if (sales_today < 10) b->rumor -= 0.03f;

    // Clamp values
    b->quality_perceived = fminf(fmaxf(b->quality_perceived, 0.0f), 1.0f);
    b->rumor = fminf(fmaxf(b->rumor, 0.0f), 1.0f);
    b->consistency = fminf(fmaxf(b->consistency, 0.0f), 1.0f);

    // Recompute reputation
    b->reputation = 0.4f * b->quality_perceived +
                    0.3f * b->rumor +
                    0.2f * b->consistency +
                    0.1f * b->handsomeness;

    if (b->reputation < 0) b->reputation = 0.0f;
    if (b->reputation > 1) b->reputation = 1.0f;
}
