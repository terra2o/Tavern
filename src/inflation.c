#include "../include/inflation.h"
#include "../include/sim_random.h"

void inflation_tick(World* w)
{
    w->inflation_rate *= 1.0f + 0.0005f + frand() * 0.001f;
    if (w->inflation_rate > 5.0f)
        w->inflation_rate = 5.0f;
}
