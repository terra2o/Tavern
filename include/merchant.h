#ifndef MERCHANT_H
#define MERCHANT_H

#include <math.h>
#include "drink.h"

typedef struct Merchant{
	float drink_price[DRINK_COUNT];
	float quality;
	float instability;
} Merchant;

void update_merchant(Merchant* m, float inflation_rate);

#endif