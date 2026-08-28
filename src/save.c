/*
*
* save.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

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

    for (int fruit = 0; fruit < FRUIT_COUNT; fruit++) {
        fprintf(f, ",%d,%d",
                b->fruits[fruit].inventory.amount,
                b->fruits[fruit].inventory.expiration_date);
    }

    fprintf(f, ",%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%f,%f,%d,%f,%d,%d\n",
            b->quality_actual, b->quality_perceived, b->rumor, b->consistency,
            b->handsomeness, b->reputation, b->last_pathway_clean_day,
            b->rent.pay_period, b->rent.next_payment_day,
            b->rent.rent_amount, b->rent.base_rent,
            b->employees, b->employees_wage, b->rent.next_wage_day,
            b->tavern_size);
}

/* Returns 1 on success. Adds the parsed tavern into t. */
static int read_tavern(char* line, Town* t)
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

    for (int fruit = 0; fruit < FRUIT_COUNT; fruit++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n2;
        if (sscanf(cursor, "%d,%d%n",
                    &b.fruits[fruit].inventory.amount,
                    &b.fruits[fruit].inventory.expiration_date, &n2) != 2) return 0;
        cursor += n2;
    }

    if (*cursor != ',') return 0;
    cursor++;
    if (sscanf(cursor, "%f,%f,%f,%f,%f,%f,%d,%d,%d,%f,%f,%d,%f,%d,%d",
               &b.quality_actual, &b.quality_perceived, &b.rumor, &b.consistency,
               &b.handsomeness, &b.reputation, &b.last_pathway_clean_day,
               &b.rent.pay_period, &b.rent.next_payment_day,
               &b.rent.rent_amount, &b.rent.base_rent, &b.employees,
               &b.employees_wage, &b.rent.next_wage_day, &b.tavern_size) != 15) return 0;

    animals_init(&b.cats, ANIMALS_DEFAULT_CAPACITY);
    town_add_tavern(t, b);
    return 1;
}

static void write_cat(FILE* f, int tavern_index, const Cat* c)
{
    fprintf(f, "cat=%d,%d,%f,%f,%d,%f,%d,%d\n",
            tavern_index, c->age, c->thirst, c->curiosity, c->drunk,
            c->health, c->alive, c->male);
}

/* Appends straight onto the owning tavern's Animals.cats[], the same way
   read_tavern appends whole taverns via town_add_tavern. Needs t's
   [taverns] to already be fully read, since it looks tavern_index up in
   t->taverns[]. */
static int read_cat(char* line, Town* t)
{
    if (strncmp(line, "cat=", 4) != 0) return 0;
    char* cursor = line + 4;

    int tavern_index;
    Cat c;
    if (sscanf(cursor, "%d,%d,%f,%f,%d,%f,%d,%d",
               &tavern_index, &c.age, &c.thirst, &c.curiosity, &c.drunk,
               &c.health, &c.alive, &c.male) != 8) return 0;

    if (tavern_index < 0 || tavern_index >= t->tavern_count) return 0;
    Animals* cats = &t->taverns[tavern_index].cats;
    if (cats->count >= cats->capacity) return 0;

    cats->cats[cats->count++] = c;
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

static int read_merchant(char* line, Town* t)
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

    for (int favor_idx = 0; favor_idx < MAX_TAVERNS; favor_idx++) {
        if (*cursor != ',') return 0;
        cursor++;
        int n4;
        if (sscanf(cursor, "%f%n", &m.tavern_favor[favor_idx], &n4) != 1) return 0;
        cursor += n4;
    }

    town_add_merchant(t, m);
    return 1;
}

static void write_town(FILE* f, const Town* t)
{
    fprintf(f, "[town]\n");
    fprintf(f, "population_capacity=%d\n", t->population.capacity);
    fprintf(f, "last_advertised_day=%d\n", t->last_advertised_day);
    fprintf(f, "player_tavern_id=%d\n\n", t->player_tavern_id);

    fprintf(f, "[population]\n");
    for (int i = 0; i < t->population.count; i++) {
        Citizen* c = &t->population.citizens[i];
        fprintf(f, "citizen=%d,%f,%f,%f,%f,%f,%d,%d",
                c->age, c->thirst, c->wealth, c->addiction, c->income, c->loyalty,
                c->last_drink_day, c->favorite_tavern_id);
        for (int d = 0; d < DRINK_COUNT; d++)
            fprintf(f, ",%f", c->drink_preference[d]);
        fprintf(f, ",%f,%f,%d,%d\n", c->health, c->anger, c->homeless, c->alive);
    }
    fprintf(f, "\n");

    fprintf(f, "[merchants]\n");
    for (int i = 0; i < t->merchant_count; i++)
        write_merchant(f, i, &t->merchants[i]);
    fprintf(f, "\n");

    fprintf(f, "[taverns]\n");
    for (int i = 0; i < t->tavern_count; i++)
        write_tavern(f, i, &t->taverns[i]);
    fprintf(f, "\n");

    fprintf(f, "[cats]\n");
    for (int i = 0; i < t->tavern_count; i++) {
        const Animals* cats = &t->taverns[i].cats;
        for (int j = 0; j < cats->count; j++)
            write_cat(f, i, &cats->cats[j]);
    }
    fprintf(f, "\n");
}

int save_game(const char* path, const World* w)
{
    FILE* f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "version=%s\n\n", GAME_VERSION);

    fprintf(f, "[world]\n");
    fprintf(f, "day=%d\n", w->day);
    fprintf(f, "player_kingdom_id=%d\n\n", w->player_kingdom_id);

    for (int ki = 0; ki < w->kingdom_count; ki++) {
        const Kingdom* k = &w->kingdoms[ki];

        fprintf(f, "[kingdom]\n");
        fprintf(f, "at_war=%d\n", k->at_war);
        fprintf(f, "our_kingdom_attack=%d\n", k->our_kingdom_attack);
        fprintf(f, "war_end_day=%d\n", k->war_end_day);
        fprintf(f, "inflation_rate=%.6f\n", k->inflation_rate);
        fprintf(f, "money_supply_prev=%.6f\n", k->money_supply_prev);
        fprintf(f, "player_town_id=%d\n\n", k->player_town_id);

        for (int ti = 0; ti < k->town_count; ti++)
            write_town(f, &k->towns[ti]);
    }

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
    world_kingdoms_free(w);
    memset(w, 0, sizeof(*w));
    world_kingdoms_init(w, MAX_KINGDOMS);

    int population_capacity = 100000;

    char line[512];
    enum { NONE, WORLD, KINGDOM, TOWN, POPULATION, MERCHANTS, TAVERNS, CATS } section = NONE;
    Kingdom* current_kingdom = NULL;
    Town* current_town = NULL;

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#')
            continue;

        if (strcmp(line, "[world]\n") == 0) {
            section = WORLD;
            continue;
        }
        if (strcmp(line, "[kingdom]\n") == 0) {
            section = KINGDOM;
            Kingdom shell = {0};
            int idx = world_add_kingdom(w, shell);
            current_kingdom = (idx >= 0) ? &w->kingdoms[idx] : NULL;
            if (current_kingdom) kingdom_towns_init(current_kingdom, MAX_TOWNS);
            current_town = NULL;
            continue;
        }
        if (strcmp(line, "[town]\n") == 0) {
            section = TOWN;
            current_town = NULL;
            if (current_kingdom) {
                Town shell = {0};
                int idx = kingdom_add_town(current_kingdom, shell);
                if (idx >= 0) {
                    current_town = &current_kingdom->towns[idx];
                    town_taverns_init(current_town, MAX_TAVERNS);
                    town_merchants_init(current_town, MAX_MERCHANTS);
                }
            }
            continue;
        }
        if (strcmp(line, "[population]\n") == 0) {
            section = POPULATION;
            if (current_town) population_init(&current_town->population, population_capacity);
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
        if (strcmp(line, "[cats]\n") == 0) {
            section = CATS;
            continue;
        }

        switch (section) {
            case WORLD:
                sscanf(line, "day=%d", &w->day);
                sscanf(line, "player_kingdom_id=%d", &w->player_kingdom_id);
                break;

            case KINGDOM:
                if (!current_kingdom) break;
                sscanf(line, "at_war=%d", &current_kingdom->at_war);
                sscanf(line, "our_kingdom_attack=%d", &current_kingdom->our_kingdom_attack);
                sscanf(line, "war_end_day=%d", &current_kingdom->war_end_day);
                sscanf(line, "inflation_rate=%f", &current_kingdom->inflation_rate);
                sscanf(line, "money_supply_prev=%f", &current_kingdom->money_supply_prev);
                sscanf(line, "player_town_id=%d", &current_kingdom->player_town_id);
                break;

            case TOWN:
                if (!current_town) break;
                sscanf(line, "population_capacity=%d", &population_capacity);
                sscanf(line, "last_advertised_day=%d", &current_town->last_advertised_day);
                sscanf(line, "player_tavern_id=%d", &current_town->player_tavern_id);
                break;

            case POPULATION: {
                if (!current_town) break;
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

                if (sscanf(cursor, "%f,%f,%d,%d", &c.health, &c.anger, &c.homeless, &c.alive) == 4
                    && current_town->population.count < current_town->population.capacity) {
                    current_town->population.citizens[current_town->population.count++] = c;
                }
                break;
            }

            case MERCHANTS:
                if (!current_town) break;
                read_merchant(line, current_town);
                break;

            case TAVERNS:
                if (!current_town) break;
                read_tavern(line, current_town);
                break;

            case CATS:
                if (!current_town) break;
                read_cat(line, current_town);
                break;

            default:
                break;
        }
    }

    fclose(f);

    for (int ki = 0; ki < w->kingdom_count; ki++) {
        Kingdom* k = &w->kingdoms[ki];

        /* Guard for saves that predate inflation */
        if (k->inflation_rate <= 0.0f) k->inflation_rate = 1.0f;

        for (int ti = 0; ti < k->town_count; ti++) {
            Town* t = &k->towns[ti];

            /* Saves from before the merchant stock/favor fields existed
               will fail every read_merchant() call above (old
               [merchants] lines end right after quality/instability,
               where the new parser expects more fields), leaving
               merchant_count at 0 and every tavern's supplier NULL.
               Rather than crash on first use of b->supplier, fall back
               to one freshly-balanced merchant so an old save still
               loads, just with a reset supply economy. */
            if (t->merchant_count == 0) {
                Merchant fallback = {0};
                fallback.drink_price[DRINK_ALE] = 5.0f;
                fallback.drink_price[DRINK_WINE_APPLE] = 90.0f;
                fallback.drink_price[DRINK_WINE_GRAPE] = 90.0f;
                fallback.quality = 0.7f;
                fallback.instability = 0.2f;
                merchant_init_default_stock(&fallback);
                town_add_merchant(t, fallback);
                for (int i = 0; i < t->tavern_count; i++)
                    t->taverns[i].supplier_id = 0;
            }

            population_recount_alive(&t->population);
            for (int i = 0; i < t->tavern_count; i++)
                animals_recount_alive(&t->taverns[i].cats);
            town_relink_suppliers(t);

            /* Defensive clamping */
            for (int i = 0; i < t->tavern_count; i++) {
                Tavern* b = &t->taverns[i];
                b->rumor = CLAMP(b->rumor, 0, 1);
                b->consistency = CLAMP(b->consistency, 0, 1);
                b->reputation = CLAMP(b->reputation, 0, 1);
                if (b->rent.base_rent <= 0.0f)
                    b->rent.base_rent = b->rent.rent_amount > 0.0f ? b->rent.rent_amount : 1500.0f;
            }
        }
    }

    {
        Kingdom* k;
        Town* t;

        if (w->player_kingdom_id < 0 || w->player_kingdom_id >= w->kingdom_count)
            return 0;
        k = &w->kingdoms[w->player_kingdom_id];
        if (k->player_town_id < 0 || k->player_town_id >= k->town_count)
            return 0;
        t = &k->towns[k->player_town_id];
        return t->tavern_count > 0;
    }
}
