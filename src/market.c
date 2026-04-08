#include <sim.h>
#include <sim_random.h>

DayResult market_simulate(Tavern* b) {
    int customers = 243;

    float affordability = (b->price > 0.0f)
        ? CLAMP(1.0f / b->price, 0.0f, 1.0f)
        : 0.0f;

    float buy_prob = b->reputation * affordability;

    int demand = 0;
    for (int i = 0; i < customers; i++)
        if (frand() < buy_prob) demand++;

    int ale_sales = demand < b->ale.amount ? demand : b->ale.amount;
    int wine_sales = demand < b->wine.amount ? demand : b->wine.amount;

    b->ale.amount -= ale_sales;
    b->wine.amount -= wine_sales;


    // TODO: different price for both 
    b->money += ale_sales * b->price;
    b->money += wine_sales * b->price;

    DayResult result;
    result.customers = customers;
    result.ale_sales = ale_sales;
    result.wine_sales = wine_sales;
    result.demand = demand;
    result.revenue = ale_sales * b->price;

    return result;
}
