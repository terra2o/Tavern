#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "log.h"

typedef struct World {
    int day; // This is absolute day since game started
    MessageLog log; // Logs of events happening 
} World;

#endif