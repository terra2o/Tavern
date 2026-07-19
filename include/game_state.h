#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "log.h"
#include "population.h"

#define MAX_TAVERNS 8
#define MAX_MERCHANTS 8

typedef enum {
    EVENT_NONE,
    EVENT_FIGHT,
    EVENT_VOMIT,
    EVENT_STEAL,
    EVENT_WAR,
    EVENT_WAR_SOLDIERS,
    EVENT_WAR_REFUGEES,
    EVENT_WAR_ATTACK
} PendingEventType;

typedef struct World {
    int day;                 /* This is absolute day since game started */
    Population population;   /* This is the population of the 'town' our tavern is in */
    int last_advertised_day; /* Gonna use this for when you haven't advertised for
                                some time, they stop coming... */
    MessageLog log;          /* Logs of events happening */
    float inflation_rate;    /* Cumulative price multiplier since day 0, starts at 1.0 */
    PendingEventType pending_event;
    int at_war;              /* 1 if currently at war */
    int our_kingdom_attack;  /* 1 if our kingdom is the attacker, 0 if defender */
    int war_end_day;         /* day the war ends */

    /* Pools of every tavern/merchant that exists, player-owned or not.
       Types are forward-declared (not #include "sim.h"/"merchant.h") to
       avoid a circular include, since sim.h includes this header for World. */
    struct Tavern* taverns;
    int tavern_count;
    int tavern_capacity;

    struct Merchant* merchants;
    int merchant_count;
    int merchant_capacity;

    int player_tavern_id; /* index into taverns[] the player currently controls */
} World;

#endif
