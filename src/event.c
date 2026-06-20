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

static char array_of_events[3][32] = {"fight", "vomit", "steal"};
#define NUM_EVENTS (sizeof(array_of_events) / sizeof(array_of_events[0]))

void event_fight(World* w)
{
    w->pending_event = EVENT_FIGHT;
}

void event_vomit(World* w)
{
    w->pending_event = EVENT_VOMIT;
}

void event_steal(World* w)
{
    w->pending_event = EVENT_STEAL;
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

int handle_steal(int ch, Tavern* b, World* w)
{
    switch (ch)
    {
        case(1):
            if (rand() % 2 == 0) {
                b->reputation += 0.30f;
                b->rumor += 0.30f;
                b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
                b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
                log_message(&w->log, "You punched him and he ran away. Folk are impressed", LOG_INFO);
                return 1;
            } else {
                b->money -= FIGHT_MEDICAL_COST;
                b->reputation -= 0.30f;
                b->rumor -= 0.30f;
                b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
                b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
                log_message(&w->log, "Turns out, the son of a bitch is stronger than you! You pay for medical expenses, people make fun of you", LOG_WARN);
                return 0;
            }
            break; 
        case(2):
            b->money -= 50;
            b->reputation -= 0.15f;
            b->rumor -= 0.15f;
            log_message(&w->log, "You called the guards. People think you're a pussy", LOG_INFO);
            return 1;
            break; 
        case(3):
            b->money -= 200;
            b->ale.amount -= 10;
            log_message(&w->log, "You ignored the thief. People didn't see anything. You obviously lost some booze, and money.", LOG_INFO);
            return 1;
            break; 
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
        case 1:
            event_vomit(w);
            break;
        case 2:
            event_steal(w);
            break;
        }
    }
}
