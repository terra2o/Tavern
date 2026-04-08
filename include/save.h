#include <sim.h>
#include <stdio.h>
#include <string.h>

#define SAVE_PATH "tavernsavefile.txt"

int save_game(const char* path,
              const World* w,
              const Tavern* b,
              const Merchant* m,
              const PeriodicPayment* p);

int load_game(const char* path,
              World* w,
              Tavern* b,
              Merchant* m,
              PeriodicPayment* p);