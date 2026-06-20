#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include "game_state.h"
#include "sim.h"

void event_fight(World* w);
void event_vomit(World* w);
void event_steal(World* w);
void random_event(World* w);

/* Attempt to break up the fight yourself. Returns 1 on success, 0 on failure. */
int event_fight_break_up(Tavern* b, World* w);
int handle_steal(int ch, Tavern* b, World* w);

#endif
