#include "../include/sim.h"
#include "../include/sim_random.h"
#include "../include/advertisement.h"

DayResult market_simulate(Tavern* b, World* w, DayResult* r) {
    int customers = (int)(w->population * (0.05f + frand() * 0.10f));

    int lost_customers_ads = no_customers_because_no_ads(w->day, w, r, customers);
    customers = customers - lost_customers_ads;

    float ale_affordability = (b->ale_price > 0.0f && b->supplier->price_per_ale > 0.0f)
        ? CLAMP(b->supplier->price_per_ale / b->ale_price, 0.0f, 1.0f)
        : 0.0f;

    float wine_affordability = (b->wine_price > 0.0f && b->supplier->price_per_wine > 0.0f)
        ? CLAMP(b->supplier->price_per_wine / b->wine_price, 0.0f, 1.0f)
        : 0.0f;

    float buy_prob_ale = b->reputation * ale_affordability;
    float buy_prob_wine = b->reputation * wine_affordability;

    int demand_ale = 0;
    for (int i = 0; i < customers; i++)
        if (frand() < buy_prob_ale) demand_ale++;

    int demand_wine = 0;
    for (int i = 0; i < customers; i++)
        if (frand() < buy_prob_wine) demand_wine++;

    int ale_sales = demand_ale < b->ale.amount ? demand_ale : b->ale.amount;
    int wine_sales = demand_wine < b->wine.amount ? demand_wine : b->wine.amount;

    b->ale.amount -= ale_sales;
    b->wine.amount -= wine_sales;

    b->money += ale_sales * b->ale_price;
    b->money += wine_sales * b->wine_price;

    DayResult result;
    result.customers = customers;
    result.ale_sales = ale_sales;
    result.wine_sales = wine_sales;
    result.demand_ale = demand_ale;
    result.demand_wine = demand_wine;
    result.revenue = (ale_sales * b->ale_price) + (wine_sales * b->wine_price);

    return result;
}
