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

typedef struct {
    int resolved;
} WarAttackState;

typedef struct {
    FightState fight_state;
    VomitState vomit_state;
    StealState steal_state;
    WarState war_state;
    WarSoldiersState war_soldiers_state;
    WarRefugeesState war_refugees_state;
    WarAttackState war_attack_state;
} EventStates;
    
void event_fight(World* w);
void event_vomit(World* w);
void event_steal(World* w);
void event_war(World* w);

/* Chance to start a new war. This is kingdom-level and unrelated to any
   one customer's stats, so it stays a die roll. */
void random_event(World* w);
void random_war_event(World* w);

/* Fight/vomit/steal odds driven by who actually showed up: taverns
   with rowdier or more destitute visitors that day are more likely to
   have trouble, instead of a flat daily die roll. */
void evaluate_customer_events(World* w, const DayResult* day);

/* Attempt to break up the fight yourself. Returns 1 on success, 0 on failure. */
int event_fight_break_up(Tavern* b, World* w);
int handle_steal(int ch, Tavern* b, World* w);

/* War event handlers. Take the player's choice (1/2/3) and apply chance-based outcomes. */
void handle_war_declaration(int choice, Tavern* b, World* w);
void handle_war_soldiers(int choice, Tavern* b, World* w);
void handle_war_refugees(int choice, Tavern* b, World* w);
void handle_war_attack(int choice, Tavern* b, World* w);

#endif
