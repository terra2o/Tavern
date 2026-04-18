#include <math.h>
#include "../include/reputation.h"
#include "../include/sim.h"
#include "../include/sim_random.h"

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

    // 0.1f in order to avoid it being 0, which causes issues
    if (b->reputation < 0) b->reputation = 0.1f;
    if (b->reputation > 1) b->reputation = 1.0f;
}
