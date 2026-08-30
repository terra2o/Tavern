/*
*
* game_state.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "log.h"

#define MAX_TAVERNS 8   /* per-town tavern pool capacity */
#define MAX_MERCHANTS 8 /* per-town merchant pool capacity */
#define MAX_TOWNS 4     /* per-kingdom town pool capacity */
#define MAX_KINGDOMS 4  /* world kingdom pool capacity */

typedef enum {
    EVENT_NONE,
    EVENT_FIGHT,
    EVENT_VOMIT,
    EVENT_STEAL,
    EVENT_CAT_TROUBLE,
    EVENT_WAR,
    EVENT_WAR_SOLDIERS,
    EVENT_WAR_REFUGEES,
    EVENT_WAR_ATTACK
} PendingEventType;

typedef struct World {
    int day;                 /* This is absolute day since game started */
    MessageLog log;          /* Logs of events happening */
    PendingEventType pending_event;

    /* Pool of every kingdom that exists. Type is forward-declared (not
       #include "kingdom.h") to avoid a circular include, since kingdom.h
       includes this header for World. Each Kingdom owns its own pool of
       Towns, which each own their own population + tavern/merchant pools -
       see kingdom.h/town.h. */
    struct Kingdom* kingdoms;
    int kingdom_count;
    int kingdom_capacity;

    int player_kingdom_id; /* index into kingdoms[] the player currently occupies */
} World;

#endif
