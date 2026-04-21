#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "log.h"

typedef struct World {
    int day; // This is absolute day since game started
    int population; // This is the population of the 'town' our tavern is in
    int last_advertised_day; // Gonna use this for when you haven't advertised for
    // some time, they stop coming...
    MessageLog log; // Logs of events happening 
} World;

#endif
