#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "log.h"

typedef enum {
    EVENT_NONE,
    EVENT_FIGHT,
    EVENT_VOMIT,
    EVENT_STEAL,
    EVENT_WAR,
    EVENT_WAR_SOLDIERS,
    EVENT_WAR_REFUGEES
} PendingEventType;

typedef struct World {
    int day;                 /* This is absolute day since game started */
    int population;          /* This is the population of the 'town' our tavern is in */
    int last_advertised_day; /* Gonna use this for when you haven't advertised for
                                some time, they stop coming... */
    MessageLog log;          /* Logs of events happening */
    float inflation_rate;    /* Cumulative price multiplier since day 0, starts at 1.0 */
    PendingEventType pending_event;
    int at_war;              /* 1 if currently at war */
    int our_kingdom_attack;  /* 1 if our kingdom is the attacker, 0 if defender */
    int war_end_day;         /* day the war ends */
} World;

#endif
