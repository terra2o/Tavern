#include "../include/sim_random.h"
#include "../include/game_state.h"

void populate(World* w)
{
    int random_int = (int)(frand() * 5.0f) + 1;
    w->population += random_int;
}

