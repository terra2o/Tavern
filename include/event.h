#ifndef EVENT_H
#define EVENT_H

#include <stdlib.h>
#include "game_state.h"
#include "sim.h"

void event_fight(World* w);
void random_event(World* w);

/* Attempt to break up the fight yourself. Returns 1 on success, 0 on failure. */
int event_fight_break_up(Tavern* b, World* w);

#endif
