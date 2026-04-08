#ifndef MERCHANT_H
#define MERCHANT_H

#include <math.h>

typedef struct Merchant{
	float price_per_ale;
	float price_per_wine;
	float quality;
	float instability;
} Merchant;

void update_merchant(Merchant* m);

#endif