#include "../include/merchant.h"
#include "../include/sim_random.h"
#include "../include/sim.h"
// sim.h has definition for CLAMP, 
// probably have to move that.

void update_merchant(Merchant* m) 
{
    if (frand() < m->instability) 
    {
        m->quality += (frand() - 0.5f) * 0.2f;
        m->price_per_ale *= (1.0f + (frand() - 0.5f) * 0.15f);
        m->price_per_wine *= (1.0f + (frand() - 0.5f) * 0.25f);
    }
	
	m->quality = CLAMP(m->quality, 0.3f, 1.0f);
    m->price_per_ale = CLAMP(m->price_per_ale, 3.0f, 8.0f);
    m->price_per_wine = CLAMP(m->price_per_wine, 15.0f, 50.0f);
}
