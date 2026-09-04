/*
*
* employee.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef EMPLOYEE_H
#define EMPLOYEE_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef signed char int8_t;

typedef enum Role {
    ROLE_BARTENDER,
    ROLE_WAITER,
    ROLE_COOK,
    ROLE_CLEANER
} Role;

typedef enum Shift {
    SHIFT_MORNING,
    SHIFT_EVENING,
    SHIFT_NIGHT
} Shift;

typedef struct EmployeeStats {
    uint8_t speed;
    uint8_t skill;
    uint8_t stamina;
    uint8_t morale;
} EmployeeStats;

typedef struct Employee {
    uint32_t id;
    char name[64];
    Role role;
    Shift shift;
    uint32_t wage_cents;
    EmployeeStats stats;
    uint8_t energy;
    int on_duty;
} Employee;

void employee_init(Employee *emp, uint32_t id, const char *name, Role role, uint32_t wage_cents);

void employee_set_role(Employee *emp, Role new_role);
void employee_set_shift(Employee *emp, Shift new_shift);
void employee_set_wage(Employee *emp, uint32_t new_wage);

void employee_train(Employee *emp, uint8_t skill_gain);
void employee_consume_energy(Employee *emp, uint8_t amount);
void employee_rest(Employee *emp, uint8_t amount);
void employee_adjust_morale(Employee *emp, int8_t delta);
void employee_tick_shift(Employee *emp);

#endif
