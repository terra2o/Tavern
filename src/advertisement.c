#include "../include/advertisement.h"
#include "../include/market.h"
#include <stdio.h>

int lost_customers_ads = 0;

void apply_advertisement(int current_day, World *w)
{
    w->last_advertised_day = current_day;
}

int no_customers_because_no_ads(int current_day, World *w, DayResult *r, int customers)
{
    if (current_day - w->last_advertised_day >= 7)
    {
        lost_customers_ads = customers / 4;
        char buf[64];
        snprintf(buf, sizeof(buf), "You lost %d customers because you don't advertise enough", lost_customers_ads);
        log_message(&w->log, buf, LOG_WARN);
        return lost_customers_ads;
    }
    return 0;
}
