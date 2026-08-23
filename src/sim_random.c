/*
*
* sim_random.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdlib.h>

float frand(void)
{
    return rand() / (float)RAND_MAX;
}