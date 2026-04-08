#include <stdlib.h>

float frand(void) {
    return rand() / (float)RAND_MAX;
}