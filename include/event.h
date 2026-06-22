#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include "game_state.h"
#include "sim.h"

/* Fight event state */
typedef struct {
    int resolved;
} FightState;

/* Vomit event state */
typedef struct {
    int resolved;
} VomitState;

/* Steal event state */
typedef struct {
    int resolved;
} StealState;

typedef struct {
    int our_kingdom_attack;
    int resolved;
} WarState;

typedef struct {
    int resolved;
} WarSoldiersState;

typedef struct {
    int resolved;
} WarRefugeesState;

void event_fight(World* w);
void event_vomit(World* w);
void event_steal(World* w);
void event_war(World* w);
void random_event(World* w);
void random_war_event(World* w);

/* Attempt to break up the fight yourself. Returns 1 on success, 0 on failure. */
int event_fight_break_up(Tavern* b, World* w);
int handle_steal(int ch, Tavern* b, World* w);

/* War event handlers — take player's choice (1/2/3), apply chance-based outcomes */
void handle_war_declaration(int choice, Tavern* b, World* w);
void handle_war_soldiers(int choice, Tavern* b, World* w);
void handle_war_refugees(int choice, Tavern* b, World* w);

#endif
