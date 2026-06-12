/*
*
* event.c for "Tavern"
*
* Copyright 2026 terra2o
*
*/

#include <stdlib.h>
#include "../include/event.h"
#include "../include/sim.h"

#define FIGHT_MEDICAL_COST 500.0f

static char array_of_events[2][32] = {"fight", "vomit"};
#define NUM_EVENTS (sizeof(array_of_events) / sizeof(array_of_events[0]))

void event_fight(World* w)
{
    w->pending_event = EVENT_FIGHT;
}

int event_fight_break_up(Tavern* b, World* w)
{
    if (rand() % 2 == 0) {
        b->reputation += 0.30f;
        b->rumor += 0.30f;
        b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
        b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
        log_message(&w->log, "You stepped in and broke up the fight. Patrons are impressed.", LOG_INFO);
        return 1;
    } else {
        b->money -= FIGHT_MEDICAL_COST;
        log_message(&w->log, "You tried to break up the fight but got hurt. Medical bill: $500.", LOG_WARN);
        return 0;
    }
}

void random_event(World* w)
{
    int chance = rand() % 4; /* 25% chance */

    if (chance == 3) {
        int event_index = rand() % NUM_EVENTS;

        switch (event_index) {
        case 0:
            event_fight(w);
            break;
        }
    }
}
