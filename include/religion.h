/*
 *
 * religion.h for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#ifndef RELIGION_H
#define RELIGION_H

#include "kingdom.h"
#include "modifier.h"
#include "population.h"

#define RELIGION_COUNT 5
#define MAX_RELIGION_MODIFIERS 4

typedef struct Religion {
    int id;
    char *name;
    Modifier *modifiers;
    int modifier_count;
} Religion;

extern Religion g_religions[RELIGION_COUNT];

void religion_init_all(void);

/* returns the religion_id that the majority of citizens have in the given
 * kingdom, or -1 if none */
int kingdom_get_majority_religion(const Kingdom *k);

/* returns the majority religion of a single town */
int town_get_majority_religion(const Town *town);

/* handles random conversion attempts when citizens talk */
void citizen_talk_religion(Citizen *a, Citizen *b);

/* handles conversion from a fight */
void religion_fight_convert(Citizen *loser, Citizen *winner);

#endif /* RELIGION_H */
