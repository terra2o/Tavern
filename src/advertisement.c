/*
*
* advertisement.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include "../include/advertisement.h"

#define ADS_STALE_DAYS 10
#define ADS_STALE_LOSS_FRACTION 0.25f

void apply_advertisement(int current_day, Town *t)
{
    t->last_advertised_day = current_day;
}

float no_customers_because_no_ads(int current_day, Town *t)
{
    if (current_day - t->last_advertised_day >= ADS_STALE_DAYS)
        return ADS_STALE_LOSS_FRACTION;
    return 0.0f;
}
