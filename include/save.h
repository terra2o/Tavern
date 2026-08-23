/*
*
* save.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef SAVE_H
#define SAVE_H

#include "sim.h"
#include <stdio.h>
#include <string.h>

#define SAVE_PATH "tavernsavefile.txt"

int save_game(const char* path, const World* w);
int load_game(const char* path, World* w);

#endif
