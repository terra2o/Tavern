#include "../include/sim.h"
#include "../include/version.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void write_tavern(FILE* f, int index, const Tavern* b)
{
    fprintf(f, "tavern=%d,%.2f,%d", index, b->money, b->supplier_id);
    for (int d = 0; d < DRINK_COUNT; d++) {
        fprintf(f, ",%f,%d,%d",
                b->drinks[d].price, b->drinks[d].inventory.amount,
                b->drinks[d].inventory.expiration_date);
    }
    for (int d = 0; d < DRINK_COUNT; d++)
        fprintf(f, ",%f", b->last_drink_price[d]);
    fprintf(f, ",%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%f,%f\n",
            b->quality_actual, b->quality_perceived, b->rumor, b->consistency,
            b->handsomeness, b->reputation, b->last_pathway_clean_day,
            b->rent.pay_period, b->rent.next_payment_day,
            b->rent.rent_amount, b->rent.base_rent);
}

/* Returns 1 on success. Advances *cursor past everything it consumed. */
static int read_tavern(char* line, World* w)
{
    if (strncmp(line, "tavern=", 7) != 0) return 0;
    char* cursor = line + 7;

    Tavern b = {0};
    int index, n;
    if (sscanf(cursor, "%d,%f,%d%n", &index, &b.money, &b.supplier_id, &n) != 3) return 0;
    cursor += n;

    for (int d = 0; d < DRINK_COUNT; d++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n2;
        if (sscanf(cursor, "%f,%d,%d%n",
                   &b.drinks[d].price, &b.drinks[d].inventory.amount,
                   &b.drinks[d].inventory.expiration_date, &n2) != 3) return 0;
        cursor += n2;
    }

    for (int d = 0; d < DRINK_COUNT; d++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n2;
        if (sscanf(cursor, "%f%n", &b.last_drink_price[d], &n2) != 1) return 0;
        cursor += n2;
    }

    if (*cursor != ',') return 0;
    cursor++;
    if (sscanf(cursor, "%f,%f,%f,%f,%f,%f,%d,%d,%d,%f,%f",
               &b.quality_actual, &b.quality_perceived, &b.rumor, &b.consistency,
               &b.handsomeness, &b.reputation, &b.last_pathway_clean_day,
               &b.rent.pay_period, &b.rent.next_payment_day,
               &b.rent.rent_amount, &b.rent.base_rent) != 11) return 0;

    world_add_tavern(w, b);
    return 1;
}

static void write_merchant(FILE* f, int index, const Merchant* m)
{
    fprintf(f, "merchant=%d", index);
    for (int d = 0; d < DRINK_COUNT; d++)
        fprintf(f, ",%f", m->drink_price[d]);
    fprintf(f, ",%.3f,%.3f", m->quality, m->instability);
    for (int d = 0; d < DRINK_COUNT; d++)
        fprintf(f, ",%f,%f,%f", m->stock[d], m->restock_rate[d], m->max_stock[d]);
    for (int t = 0; t < MAX_TAVERNS; t++)
        fprintf(f, ",%f", m->tavern_favor[t]);
    fprintf(f, "\n");
}

static int read_merchant(char* line, World* w)
{
    if (strncmp(line, "merchant=", 9) != 0) return 0;
    char* cursor = line + 9;

    Merchant m = {0};
    int index, n;
    if (sscanf(cursor, "%d%n", &index, &n) != 1) return 0;
    cursor += n;

    for (int d = 0; d < DRINK_COUNT; d++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n2;
        if (sscanf(cursor, "%f%n", &m.drink_price[d], &n2) != 1) return 0;
        cursor += n2;
    }

    if (*cursor != ',') return 0;
    cursor++;
    int n2;
    if (sscanf(cursor, "%f,%f%n", &m.quality, &m.instability, &n2) != 2) return 0;
    cursor += n2;

    for (int d = 0; d < DRINK_COUNT; d++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n3;
        if (sscanf(cursor, "%f,%f,%f%n",
                   &m.stock[d], &m.restock_rate[d], &m.max_stock[d], &n3) != 3) return 0;
        cursor += n3;
    }

    for (int t = 0; t < MAX_TAVERNS; t++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n4;
        if (sscanf(cursor, "%f%n", &m.tavern_favor[t], &n4) != 1) return 0;
        cursor += n4;
    }

    world_add_merchant(w, m);
    return 1;
}

int save_game(const char* path, const World* w)
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
    fprintf(f, "war_end_day=%d\n", w->war_end_day);
    fprintf(f, "player_tavern_id=%d\n\n", w->player_tavern_id);

    fprintf(f, "[population]\n");
    for (int i = 0; i < w->population.count; i++) {
        Citizen* c = &w->population.citizens[i];
        fprintf(f, "citizen=%d,%f,%f,%f,%f,%f,%d,%d",
                c->age, c->thirst, c->wealth, c->addiction, c->income, c->loyalty,
                c->last_drink_day, c->favorite_tavern_id);
        for (int d = 0; d < DRINK_COUNT; d++)
            fprintf(f, ",%f", c->drink_preference[d]);
        fprintf(f, ",%f,%d,%d\n", c->health, c->homeless, c->alive);
    }
    fprintf(f, "\n");

    fprintf(f, "[merchants]\n");
    for (int i = 0; i < w->merchant_count; i++)
        write_merchant(f, i, &w->merchants[i]);
    fprintf(f, "\n");

    fprintf(f, "[taverns]\n");
    for (int i = 0; i < w->tavern_count; i++)
        write_tavern(f, i, &w->taverns[i]);

    fclose(f);
    return 1;
}


int load_game(const char* path, World* w)
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
    world_taverns_free(w);
    world_merchants_free(w);
    memset(w, 0, sizeof(*w));

    int population_capacity = 100000;

    world_taverns_init(w, MAX_TAVERNS);
    world_merchants_init(w, MAX_MERCHANTS);

    char line[512];
    enum { NONE, WORLD, POPULATION, MERCHANTS, TAVERNS } section = NONE;

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
        if (strcmp(line, "[merchants]\n") == 0) {
            section = MERCHANTS;
            continue;
        }
        if (strcmp(line, "[taverns]\n") == 0) {
            section = TAVERNS;
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
                sscanf(line, "player_tavern_id=%d", &w->player_tavern_id);
                break;

            case POPULATION: {
                Citizen c;
                if (strncmp(line, "citizen=", 8) != 0) break;
                char* cursor = line + 8;
                int n;
                if (sscanf(cursor, "%d,%f,%f,%f,%f,%f,%d,%d%n",
                           &c.age, &c.thirst, &c.wealth, &c.addiction, &c.income, &c.loyalty,
                           &c.last_drink_day, &c.favorite_tavern_id, &n) != 8) break;
                cursor += n;

                int ok = 1;
                for (int d = 0; d < DRINK_COUNT; d++) {
                    if (*cursor != ',') { ok = 0; break; }
                    cursor++;
                    int n2;
                    if (sscanf(cursor, "%f%n", &c.drink_preference[d], &n2) != 1) { ok = 0; break; }
                    cursor += n2;
                }
                if (!ok || *cursor != ',') break;
                cursor++;

                if (sscanf(cursor, "%f,%d,%d", &c.health, &c.homeless, &c.alive) == 3
                    && w->population.count < w->population.capacity) {
                    w->population.citizens[w->population.count++] = c;
                }
                break;
            }

            case MERCHANTS:
                read_merchant(line, w);
                break;

            case TAVERNS:
                read_tavern(line, w);
                break;

            default:
                break;
        }
    }

    fclose(f);

    /* Saves from before the merchant stock/favor fields existed will
       fail every read_merchant() call above (old [merchants] lines end
       right after quality/instability, where the new parser expects
       more fields), leaving merchant_count at 0 and every tavern's
       supplier NULL. Rather than crash on first use of b->supplier,
       fall back to one freshly-balanced merchant so an old save still
       loads, just with a reset supply economy. */
    if (w->merchant_count == 0) {
        Merchant fallback = {0};
        fallback.drink_price[DRINK_ALE] = 5.0f;
        fallback.drink_price[DRINK_WINE] = 90.0f;
        fallback.quality = 0.7f;
        fallback.instability = 0.2f;
        merchant_init_default_stock(&fallback);
        world_add_merchant(w, fallback);
        for (int i = 0; i < w->tavern_count; i++)
            w->taverns[i].supplier_id = 0;
    }

    population_recount_alive(&w->population);
    world_relink_suppliers(w);

    /* Defensive clamping */
    for (int i = 0; i < w->tavern_count; i++) {
        Tavern* b = &w->taverns[i];
        b->rumor = CLAMP(b->rumor, 0, 1);
        b->consistency = CLAMP(b->consistency, 0, 1);
        b->reputation = CLAMP(b->reputation, 0, 1);
        if (b->rent.base_rent <= 0.0f)
            b->rent.base_rent = b->rent.rent_amount > 0.0f ? b->rent.rent_amount : 1500.0f;
    }

    /* Guard for saves that predate inflation */
    if (w->inflation_rate <= 0.0f) w->inflation_rate = 1.0f;

    return w->tavern_count > 0;
}
