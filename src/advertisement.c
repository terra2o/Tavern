#include "../include/advertisement.h"

#define ADS_STALE_DAYS 10
#define ADS_STALE_LOSS_FRACTION 0.25f

void apply_advertisement(int current_day, World *w)
{
    w->last_advertised_day = current_day;
}

float no_customers_because_no_ads(int current_day, World *w)
{
    if (current_day - w->last_advertised_day >= ADS_STALE_DAYS)
        return ADS_STALE_LOSS_FRACTION;
    return 0.0f;
}
