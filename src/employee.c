/*
 *
 * employee.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "employee.h"
#include "sim.h"
#include <stdlib.h>
#include <string.h>

void employee_init(Employee *emp, uint32_t id, const char *name, Role role,
                   uint32_t wage_cents)
{
    if (!emp)
        return;

    emp->id = id;
    emp->role = role;
    emp->wage_cents = wage_cents;
    emp->energy = 100;
    emp->on_duty = 1;

    if (name) {
        strncpy(emp->name, name, sizeof(emp->name) - 1);
        emp->name[sizeof(emp->name) - 1] = '\0';
    } else {
        emp->name[0] = '\0';
    }

    emp->stats.speed = 10;
    emp->stats.skill = 10;
    emp->stats.stamina = 10;
    emp->stats.morale = 50;
}

void employee_set_role(Employee *emp, Role new_role)
{
    if (!emp)
        return;
    emp->role = new_role;
}

void employee_set_wage(Employee *emp, uint32_t new_wage)
{
    if (!emp)
        return;
    emp->wage_cents = new_wage;
}

void employee_train(Employee *emp, uint8_t skill_gain)
{
    if (!emp)
        return;

    /* prevent overflow when boosting skill */
    if (255 - emp->stats.skill < skill_gain) {
        emp->stats.skill = 255;
    } else {
        emp->stats.skill += skill_gain;
    }
}

void employee_consume_energy(Employee *emp, uint8_t amount)
{
    if (!emp)
        return;

    if (emp->energy <= amount) {
        emp->energy = 0;
    } else {
        emp->energy -= amount;
    }
}

void employee_rest(Employee *emp, uint8_t amount)
{
    if (!emp)
        return;

    if (255 - emp->energy < amount) {
        emp->energy = 255;
    } else {
        emp->energy += amount;
    }
}

void employee_adjust_morale(Employee *emp, int8_t delta)
{
    int new_morale;

    if (!emp)
        return;

    new_morale = (int)emp->stats.morale + delta;
    if (new_morale > 100) {
        emp->stats.morale = 100;
    } else if (new_morale < 0) {
        emp->stats.morale = 0;
    } else {
        emp->stats.morale = (uint8_t)new_morale;
    }
}

void employee_tick_shift(Employee *emp)
{
    uint8_t drain;

    if (!emp || !emp->on_duty)
        return;

    /* scale energy drain based on stamina attribute */
    drain = (emp->stats.stamina > 20) ? 1 : (21 - emp->stats.stamina);
    employee_consume_energy(emp, drain);

    if (emp->energy == 0) {
        employee_adjust_morale(emp, -5);
    }
}

#define SOLO_BARTENDER_CAPACITY 20
#define SOLO_WAITER_CAPACITY 20

int employee_get_bartender_capacity(const struct Tavern *b)
{
    int total = SOLO_BARTENDER_CAPACITY;
    int i;

    if (!b || !b->employees)
        return total;

    for (i = 0; i < b->employee_count; i++) {
        const Employee *emp = &b->employees[i];
        if (emp->role == ROLE_BARTENDER && emp->on_duty) {
            int cap = 20 + (emp->stats.speed * 2) + emp->stats.skill;
            if (emp->energy < 20) {
                cap = cap * emp->energy / 100;
                if (cap < 5)
                    cap = 5;
            }
            total += cap;
        }
    }
    return total;
}

int employee_get_waiter_capacity(const struct Tavern *b)
{
    int total = SOLO_WAITER_CAPACITY;
    int i;

    if (!b || !b->employees)
        return total;

    for (i = 0; i < b->employee_count; i++) {
        const Employee *emp = &b->employees[i];
        if (emp->role == ROLE_WAITER && emp->on_duty) {
            int cap = 20 + (emp->stats.speed * 2) + emp->stats.stamina;
            if (emp->energy < 20) {
                cap = cap * emp->energy / 100;
                if (cap < 5)
                    cap = 5;
            }
            total += cap;
        }
    }
    return total;
}

void employee_cleaners_work(struct Tavern *b, int current_day,
                            struct MessageLog *log)
{
    int i;
    int cleaned_pathway = 0;
    int cleaned_tavern = 0;

    if (!b || !b->employees)
        return;

    for (i = 0; i < b->employee_count; i++) {
        Employee *emp = &b->employees[i];
        if (emp->role == ROLE_CLEANER && emp->on_duty && emp->energy > 10) {
            /* pathway upkeep when dirty */
            if (!cleaned_pathway &&
                (current_day - b->last_pathway_clean_day >= 2)) {
                b->last_pathway_clean_day = current_day;
                cleaned_pathway = 1;
                employee_consume_energy(emp, 8);
                if (log)
                    log_message(log, "Cleaner cleaned the pathway outside.",
                                LOG_INFO);
            } else if (!cleaned_tavern) {
                b->consistency =
                    CLAMP(b->consistency + 0.05f * (emp->stats.skill / 10.0f),
                          0.0f, 1.0f);
                b->quality_perceived =
                    CLAMP(b->quality_perceived + 0.04f, 0.0f, 1.0f);
                cleaned_tavern = 1;
                employee_consume_energy(emp, 5);
                if (log)
                    log_message(log, "Cleaner tidied up the tavern.", LOG_INFO);
            }
        }
    }
}

void employee_cooks_work(struct Tavern *b, struct MessageLog *log)
{
    int i;
    char buf[128];

    if (!b || !b->employees)
        return;

    for (i = 0; i < b->employee_count; i++) {
        Employee *emp = &b->employees[i];
        if (emp->role == ROLE_COOK && emp->on_duty && emp->energy > 10) {
            int bread = 4 + (emp->stats.speed / 4);
            int stew = 2 + (emp->stats.skill / 5);
            float bread_cost = bread * 0.50f;
            float stew_cost = stew * 1.50f;
            float total_cost = bread_cost + stew_cost;

            if (b->money >= total_cost) {
                b->money -= total_cost;
                b->foods[FOOD_BREAD].inventory.amount += bread;
                b->foods[FOOD_STEW].inventory.amount += stew;
                employee_consume_energy(emp, 8);
                if (log) {
                    tavern_snprintf(
                        buf, sizeof(buf),
                        "Cook %s prepared %d bread and %d stew ($%.2f spent).",
                        emp->name, bread, stew, total_cost);
                    log_message(log, buf, LOG_INFO);
                }
            } else if (b->money >= bread_cost) {
                b->money -= bread_cost;
                b->foods[FOOD_BREAD].inventory.amount += bread;
                employee_consume_energy(emp, 4);
                if (log) {
                    tavern_snprintf(buf, sizeof(buf),
                                    "Cook %s baked %d bread ($%.2f spent).",
                                    emp->name, bread, bread_cost);
                    log_message(log, buf, LOG_INFO);
                }
            }
        }
    }
}

int employee_fire(struct Tavern *b, Employee *emp)
{
    int i, found = -1;
    Employee *new_employees;

    if (!b || !emp || !b->employees || b->employee_count <= 0)
        return 0;

    for (i = 0; i < b->employee_count; i++) {
        if (&b->employees[i] == emp ||
            (emp->id != 0 && b->employees[i].id == emp->id)) {
            found = i;
            break;
        }
    }

    if (found == -1)
        return 0;

    for (i = found; i < b->employee_count - 1; i++) {
        b->employees[i] = b->employees[i + 1];
    }
    b->employee_count--;

    if (b->employee_count == 0) {
        free(b->employees);
        b->employees = NULL;
    } else {
        new_employees = (Employee *)realloc(
            b->employees, sizeof(Employee) * b->employee_count);
        if (new_employees)
            b->employees = new_employees;
    }

    return 1;
}

int employee_fire_by_id(struct Tavern *b, uint32_t id)
{
    int i;

    if (!b || !b->employees || b->employee_count <= 0)
        return 0;

    for (i = 0; i < b->employee_count; i++) {
        if (b->employees[i].id == id)
            return employee_fire(b, &b->employees[i]);
    }

    return 0;
}
