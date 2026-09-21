/*
 *
 * religion.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "religion.h"
#include "sim_random.h"
#include <stdlib.h>
#include <string.h>

Religion g_religions[RELIGION_COUNT];

static Modifier mod_earth[] = {
    {MOD_MERCHANT_PRICE_WINE, MOD_PERCENT, -0.2f} /* 20% cheaper wine */
};

static Modifier mod_sun[] = {
    {MOD_MERCHANT_PRICE_ALE, MOD_PERCENT, -0.2f} /* 20% cheaper ale */
};

static Modifier mod_rainbow[] = {
    {MOD_MERCHANT_SELL_PRICE, MOD_PERCENT, 0.2f} /* 20% higher sell price */
};

static Modifier mod_moon[] = {
    {MOD_FIGHT_CHANCE, MOD_PERCENT, 0.3f} /* 30% more chance to fight */
};

static Modifier mod_discipline[] = {
    {MOD_FIGHT_CHANCE, MOD_PERCENT, -1.0f} /* no fights */
};

void religion_init_all(void)
{
    g_religions[0].id = 0;
    g_religions[0].name = "Earth";
    g_religions[0].modifiers = mod_earth;
    g_religions[0].modifier_count = 1;

    g_religions[1].id = 1;
    g_religions[1].name = "Sun";
    g_religions[1].modifiers = mod_sun;
    g_religions[1].modifier_count = 1;

    g_religions[2].id = 2;
    g_religions[2].name = "Rainbow";
    g_religions[2].modifiers = mod_rainbow;
    g_religions[2].modifier_count = 1;

    g_religions[3].id = 3;
    g_religions[3].name = "Moon";
    g_religions[3].modifiers = mod_moon;
    g_religions[3].modifier_count = 1;

    g_religions[4].id = 4;
    g_religions[4].name = "Discipline";
    g_religions[4].modifiers = mod_discipline;
    g_religions[4].modifier_count = 1;
}

int kingdom_get_majority_religion(const Kingdom *k)
{
    int counts[RELIGION_COUNT] = {0};
    int max_count = 0;
    int majority_id = -1;
    int total_citizens = 0;
    int t, c, i;

    for (t = 0; t < k->town_count; t++) {
        Town *town = &k->towns[t];
        for (c = 0; c < town->population.count; c++) {
            Citizen *cit = &town->population.citizens[c];
            if (cit->alive) {
                total_citizens++;
                if (cit->religion_id >= 0 && cit->religion_id < RELIGION_COUNT)
                    counts[cit->religion_id]++;
            }
        }
    }

    for (i = 0; i < RELIGION_COUNT; i++) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            majority_id = i;
        }
    }

    /* a religion must be held by more than 50% of the population to be the
     * majority */
    if (max_count > total_citizens / 2)
        return majority_id;

    return -1;
}

int town_get_majority_religion(const Town *town)
{
    int counts[RELIGION_COUNT] = {0};
    int max_count = 0;
    int majority_id = -1;
    int total_citizens = 0;
    int c, i;

    for (c = 0; c < town->population.count; c++) {
        Citizen *cit = &town->population.citizens[c];
        if (cit->alive) {
            total_citizens++;
            if (cit->religion_id >= 0 && cit->religion_id < RELIGION_COUNT)
                counts[cit->religion_id]++;
        }
    }

    for (i = 0; i < RELIGION_COUNT; i++) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            majority_id = i;
        }
    }

    if (max_count > total_citizens / 2)
        return majority_id;

    return -1;
}

void citizen_talk_religion(Citizen *a, Citizen *b)
{
    /* if both have the same religion or both have no religion, do nothing */
    if (a->religion_id == b->religion_id)
        return;

    /* a citizen with a religion tries to convert one without, or one with a
     * different religion */
    if (a->religion_id != -1 && b->religion_id == -1) {
        if (frand() < 0.2f) /* 20% chance to convert a non-believer */
            b->religion_id = a->religion_id;
    } else if (b->religion_id != -1 && a->religion_id == -1) {
        if (frand() < 0.2f)
            a->religion_id = b->religion_id;
    } else {
        /* both have different religions, stubborn conversion */
        if (frand() < 0.05f) { /* 5% chance */
            if (frand() < 0.5f)
                b->religion_id = a->religion_id;
            else
                a->religion_id = b->religion_id;
        }
    }
}

void religion_fight_convert(Citizen *loser, Citizen *winner)
{
    if (winner->religion_id != -1 &&
        loser->religion_id != winner->religion_id) {
        /* the loser feels let down by their God and converts */
        loser->religion_id = winner->religion_id;
    }
}
