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
    emp->on_duty = 0;

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
