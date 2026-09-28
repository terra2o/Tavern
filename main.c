/*
 *
 * main.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "include/compat.h"
#include "include/event.h"
#include "include/game_state.h"
#include "include/log.h"
#include "include/religion.h"
#include "include/save.h"
#include "include/sim.h"
#include "include/ui.h"
#include "include/version.h"
#include <curses.h>
#include <stdlib.h>
#include <time.h>

#define VERSION_STRING "Tavern - Version: " GAME_VERSION

static void init_terminal(void)
{
    initscr();
    compat_console_init();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
#ifndef PDCURSES
    /* ncurses holds esc for up to 1000ms; pdcurses has no delay */
    set_escdelay(25);
#endif
    init_colors();
    curs_set(0);
}

static void event_handler(Tavern *b, Town *t, Kingdom *k, World *w,
                          UiState *ui_state, int actions_per_day, UiMode mode,
                          int *resolved)
{
    int ch;

    w->pending_event = EVENT_NONE;
    ui_state->mode = mode;
    *resolved = 0;
    while (!*resolved) {
        draw_ui(b, w->day, 0, actions_per_day, t, k, w, ui_state,
                &ui_state->war);
        ch = getch();
        napms(16);
        compat_handle_resize(ch);
        if (ch != KEY_RESIZE && ch != ERR)
            ui_handle_input(ch, ui_state, b, t, k, w);
    }
    ui_state->mode = UI_MODE_NORMAL;
}

static int handle_normal_action(int ch, Tavern *b, Town *t, Kingdom *k,
                                World *w, UiState *ui_state,
                                int actions_per_day)
{
    Action choice;
    const ActionInputSpec *spec;
    int cmax_x, cmax_y;

    if (ch == 's' || ch == 'S') {
        ui_state->mode = UI_MODE_SUPPLIER;
        ui_state->supplier.selected = b->supplier_id;
        return 0;
    }
    if (ch == 'd' || ch == 'D') {
        ui_state->mode = UI_MODE_DETAIL;
        return 0;
    }

    choice = read_action(ch);
    if (choice == (Action)-1)
        return -1;
    if (choice == (Action)-2)
        return 0;

    if (choice == ACT_SET_RELIGION) {
        ui_state->mode = UI_MODE_RELIGION;
        return 0;
    }

    if (choice == ACT_BUY_WINE || choice == ACT_ADJUST_WINE_PRICE) {
        ui_state->pending_action = choice;
        ui_state->mode = UI_MODE_WINE_VARIETY;
        return 0;
    }

    spec = find_action_input_spec(choice);
    if (spec != NULL) {
        ui_state->pending_action = choice;
        ui_start_number_input(ui_state, spec->prompt, spec->min_val,
                              spec->max_val, spec->is_float);
        return 0;
    }

    if (choice == ACT_CLEAN_PATHWAY) {
        apply_action(b, choice, t, k, w, 0);
        log_message(&w->log, "Cleaned pathway.", LOG_INFO);
        return 1;
    }

    if (choice == ACT_COLLECT_FRUIT) {
        getmaxyx(stdscr, cmax_y, cmax_x);
        collect_state_start(&ui_state->collect, cmax_x, cmax_y);
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_COLLECT,
                      &ui_state->collect.resolved);
        log_message(&w->log, "Went out to pick fruit.", LOG_INFO);
        return 1;
    }

    if (choice == ACT_HIRE_EMPLOYEES) {
        if (b->employee_count >= b->tavern_size * EMPLOYEES_PER_TAVERN_SIZE) {
            log_message(&w->log,
                        "Tavern is at employee capacity! Expand tavern first.",
                        LOG_WARN);
            return 0;
        }
        ui_state->mode = UI_MODE_HIRE_ROLE;
        return 0;
    }

    apply_action(b, choice, t, k, w, 0);
    /* hire and expand log their own outcome inside apply_action */
    if (choice != ACT_HIRE_EMPLOYEES && choice != ACT_EXPAND_TAVERN)
        log_message(&w->log, "Action completed.", LOG_INFO);
    return 1;
}

static int run_player_action(Tavern *b, Town *t, Kingdom *k, World *w,
                             UiState *ui_state, int action_num,
                             int actions_per_day)
{
    int ch;
    int res;

    while (1) {
        draw_ui(b, w->day, action_num, actions_per_day, t, k, w, ui_state,
                &ui_state->war);

        ch = getch();
        napms(16);
        compat_handle_resize(ch);

        if (ch != KEY_RESIZE && ch != ERR)
            ui_handle_input(ch, ui_state, b, t, k, w);

        if (ui_state->number_input.is_confirmed != 0) {
            ui_process_action(ui_state, b, t, k, w);
            return 1;
        }

        if (ui_state->mode != UI_MODE_NORMAL)
            continue;

        res = handle_normal_action(ch, b, t, k, w, ui_state, actions_per_day);
        if (res != 0)
            return res;
    }
}

static int run_day_actions(Tavern *b, Town *t, Kingdom *k, World *w,
                           UiState *ui_state)
{
    int actions_per_day = tavern_actions_per_day(b);
    int action_num;
    int e;
    int res;

    for (action_num = 1; action_num <= actions_per_day; action_num++) {
        res = run_player_action(b, t, k, w, ui_state, action_num,
                                actions_per_day);
        if (res < 0)
            return 0;

        for (e = 0; e < b->employee_count; e++)
            employee_tick_shift(&b->employees[e]);
    }
    return 1;
}

static void handle_pending_events(Tavern *b, Town *t, Kingdom *k, World *w,
                                  UiState *ui_state, int actions_per_day)
{
    switch (w->pending_event) {
    case EVENT_FIGHT:
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_FIGHT,
                      &ui_state->fight.resolved);
        break;
    case EVENT_VOMIT:
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_VOMIT,
                      &ui_state->vomit.resolved);
        break;
    case EVENT_STEAL:
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_STEAL,
                      &ui_state->steal.resolved);
        break;
    case EVENT_CAT_TROUBLE:
        event_handler(b, t, k, w, ui_state, actions_per_day,
                      UI_MODE_CAT_TROUBLE, &ui_state->cat_trouble.resolved);
        break;
    case EVENT_WAR:
        ui_state->war.our_kingdom_attack = k->our_kingdom_attack;
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_WAR,
                      &ui_state->war.resolved);
        break;
    case EVENT_WAR_SOLDIERS:
        event_handler(b, t, k, w, ui_state, actions_per_day,
                      UI_MODE_WAR_SOLDIERS, &ui_state->war_soldiers.resolved);
        break;
    case EVENT_WAR_REFUGEES:
        event_handler(b, t, k, w, ui_state, actions_per_day,
                      UI_MODE_WAR_REFUGEES, &ui_state->war_refugees.resolved);
        break;
    case EVENT_WAR_ATTACK:
        event_handler(b, t, k, w, ui_state, actions_per_day, UI_MODE_WAR_ATTACK,
                      &ui_state->war_attack.resolved);
        break;
    case EVENT_NONE:
    default:
        break;
    }
}

static void run_end_of_day(Tavern *b, Town *t, Kingdom *k, World *w,
                           UiState *ui_state)
{
    char save_path[512];
    int actions_per_day = tavern_actions_per_day(b);
    int sales = simulate_day(w);
    int total_wine;
    char buf_l[256];

    compat_get_save_path(save_path, sizeof(save_path));

    if (k->at_war && w->pending_event == EVENT_NONE)
        random_war_event(k, w);

    if (k->at_war && w->day >= k->war_end_day) {
        k->at_war = 0;
        log_message(&w->log, "The war has ended. Peace returns to the land.",
                    LOG_IMPORTANT);
    }

    handle_pending_events(b, t, k, w, ui_state, actions_per_day);
    save_game(save_path, w);

    total_wine = b->drinks[DRINK_WINE_APPLE].inventory.amount +
                 b->drinks[DRINK_WINE_GRAPE].inventory.amount;
    tavern_snprintf(buf_l, sizeof(buf_l),
                    "End of day %d: %d sales | Money: $%.2f | Ale: %d | "
                    "Wine: %d | Bread: %d | Stew: %d | Rep: %.2f",
                    w->day, sales, b->money,
                    b->drinks[DRINK_ALE].inventory.amount, total_wine,
                    b->foods[FOOD_BREAD].inventory.amount,
                    b->foods[FOOD_STEW].inventory.amount, b->reputation);
    log_message(&w->log, buf_l, LOG_IMPORTANT);
}

int main(void)
{
    World w = {0};
    Kingdom *k;
    Town *t;
    Tavern *b;
    UiState ui_state;
    char version[64];
    char pool_buf[64];
    char save_path[512];

    srand((unsigned int)time(NULL));
    religion_init_all();
    
    compat_get_save_path(save_path, sizeof(save_path));

    if (!load_game(save_path, &w)) {
        init_new_game(&w);
        save_game(save_path, &w);
    }

    k = world_player_kingdom(&w);
    t = world_player_town(&w);
    b = world_player_tavern(&w);

    init_terminal();

    tavern_snprintf(version, sizeof(version), "%s", VERSION_STRING);
    log_message(&w.log, version, LOG_IMPORTANT);
    log_message(
        &w.log,
        "Welcome! Press a key to start the best tavern simulation ever...",
        LOG_IMPORTANT);

    tavern_snprintf(pool_buf, sizeof(pool_buf),
                    "Taverns in town: %d | Merchants: %d", t->tavern_count,
                    t->merchant_count);
    log_message(&w.log, pool_buf, LOG_INFO);

    ui_state_init(&ui_state);
    ui_state.war.our_kingdom_attack = k->our_kingdom_attack;

    while (run_day_actions(b, t, k, &w, &ui_state))
        run_end_of_day(b, t, k, &w, &ui_state);

    world_kingdoms_free(&w);
    endwin();

    return 0;
}
