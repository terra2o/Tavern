#include "../include/sim.h"
#include "../include/version.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SAVE_PATH "tavernsavefile.txt"

int save_game(const char* path,
              const World* w,
              const Tavern* b,
              const Merchant* m,
              const PeriodicPayment* p)
{
    FILE* f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "version=%s\n\n", GAME_VERSION);

    fprintf(f, "[world]\n");
    fprintf(f, "day=%d\n", w->day);
    fprintf(f, "population_count=%d\n", w->population.count);
    fprintf(f, "population_capacity=%d\n", w->population.capacity);
    fprintf(f, "last_advertised_day=%d\n", w->last_advertised_day);
    fprintf(f, "inflation_rate=%.6f\n", w->inflation_rate);
    fprintf(f, "at_war=%d\n", w->at_war);
    fprintf(f, "our_kingdom_attack=%d\n", w->our_kingdom_attack);
    fprintf(f, "war_end_day=%d\n\n", w->war_end_day);

    fprintf(f, "[population]\n");
    for (int i = 0; i < w->population.count; i++) {
        Citizen* c = &w->population.citizens[i];
        fprintf(f, "citizen=%d,%f,%f,%d,%d\n",
                c->age, c->thirst, c->wealth, c->homeless, c->alive);
    }
    fprintf(f, "\n");

    fprintf(f, "[tavern]\n");
    fprintf(f, "money=%.2f\n", b->money);
    fprintf(f, "ale_price=%.2f\n", b->ale_price);
    fprintf(f, "wine_price=%.2f\n", b->wine_price);
    fprintf(f, "ale=%d\n", b->ale.amount);
    fprintf(f, "wine=%d\n", b->wine.amount);
    fprintf(f, "quality_actual=%.3f\n", b->quality_actual);
    fprintf(f, "quality_perceived=%.3f\n", b->quality_perceived);
    fprintf(f, "rumor=%.3f\n", b->rumor);
    fprintf(f, "consistency=%.3f\n", b->consistency);
    fprintf(f, "handsomeness=%.3f\n", b->handsomeness);
    fprintf(f, "reputation=%.3f\n\n", b->reputation);

    fprintf(f, "[merchant]\n");
    fprintf(f, "price_per_ale=%.3f\n", m->price_per_ale);
    fprintf(f, "price_per_wine=%.3f\n", m->price_per_wine);
    fprintf(f, "quality=%.3f\n", m->quality);
    fprintf(f, "instability=%.3f\n\n", m->instability);

    fprintf(f, "[payment]\n");
    fprintf(f, "pay_period=%d\n", p->pay_period);
    fprintf(f, "next_payment_day=%d\n", p->next_payment_day);
    fprintf(f, "rent_amount=%f\n", p->rent_amount);
    fprintf(f, "base_rent=%f\n", p->base_rent);

    fclose(f);
    return 1;
}


int load_game(const char* path,
              World* w,
              Tavern* b,
              Merchant* m,
              PeriodicPayment* p)
{
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    /*
    why we do this before loading
    is because if we don't write
    0 to all;
     * some fields might not appear
     * some values might fail to parse
     * some fields might be added later
    */
    free(w->population.citizens);
    memset(w, 0, sizeof(*w));
    memset(b, 0, sizeof(*b));
    memset(m, 0, sizeof(*m));
    memset(p, 0, sizeof(*p));

    int population_capacity = 100000;

    char line[256];
    enum { NONE, WORLD, POPULATION, TAVERN, MERCHANT, PAYMENT } section = NONE;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        if (strcmp(line, "[world]\n") == 0) {
            section = WORLD;
            continue;
        }
        if (strcmp(line, "[population]\n") == 0) {
            section = POPULATION;
            population_init(&w->population, population_capacity);
            continue;
        }
        if (strcmp(line, "[tavern]\n") == 0) {
            section = TAVERN;
            continue;
        }
        if (strcmp(line, "[merchant]\n") == 0) {
            section = MERCHANT;
            continue;
        }
        if (strcmp(line, "[payment]\n") == 0) {
            section = PAYMENT;
            continue;
        }

        switch (section) {
            case WORLD:
                sscanf(line, "day=%d", &w->day);
                sscanf(line, "population_capacity=%d", &population_capacity);
                sscanf(line, "last_advertised_day=%d", &w->last_advertised_day);
                sscanf(line, "inflation_rate=%f", &w->inflation_rate);
                sscanf(line, "at_war=%d", &w->at_war);
                sscanf(line, "our_kingdom_attack=%d", &w->our_kingdom_attack);
                sscanf(line, "war_end_day=%d", &w->war_end_day);
                break;

            case POPULATION: {
                Citizen c;
                if (sscanf(line, "citizen=%d,%f,%f,%d,%d",
                           &c.age, &c.thirst, &c.wealth, &c.homeless, &c.alive) == 5
                    && w->population.count < w->population.capacity) {
                    w->population.citizens[w->population.count++] = c;
                }
                break;
            }

            case TAVERN:
                sscanf(line, "money=%f", &b->money);
                sscanf(line, "ale_price=%f", &b->ale_price);
                sscanf(line, "wine_price=%f", &b->wine_price);
                sscanf(line, "ale=%d", &b->ale.amount);
                sscanf(line, "wine=%d", &b->wine.amount);
                sscanf(line, "quality_actual=%f", &b->quality_actual);
                sscanf(line, "quality_perceived=%f", &b->quality_perceived);
                sscanf(line, "rumor=%f", &b->rumor);
                sscanf(line, "consistency=%f", &b->consistency);
                sscanf(line, "handsomeness=%f", &b->handsomeness);
                sscanf(line, "reputation=%f", &b->reputation);
                break;

            case MERCHANT:
                sscanf(line, "price_per_ale=%f", &m->price_per_ale);
                sscanf(line, "price_per_wine=%f", &m->price_per_wine);
                sscanf(line, "quality=%f", &m->quality);
                sscanf(line, "instability=%f", &m->instability);
                break;

            case PAYMENT:
                sscanf(line, "pay_period=%d", &p->pay_period);
                sscanf(line, "next_payment_day=%d", &p->next_payment_day);
                sscanf(line, "rent_amount=%f", &p->rent_amount);
                sscanf(line, "base_rent=%f", &p->base_rent);
                break;

            default:
                break;
        }
    }

    fclose(f);

    /* Re-link pointers */
    b->supplier = m;

    /* Defensive clamping */
    b->rumor = CLAMP(b->rumor, 0, 1);
    b->consistency = CLAMP(b->consistency, 0, 1);
    b->reputation = CLAMP(b->reputation, 0, 1);

    /* Guard for saves that predate inflation */
    if (w->inflation_rate <= 0.0f) w->inflation_rate = 1.0f;
    if (p->base_rent <= 0.0f) p->base_rent = p->rent_amount > 0.0f ? p->rent_amount : 1500.0f;

    return 1;
}

