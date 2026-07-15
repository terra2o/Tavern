/*
*
* main.c for Tavern
* 
* Copyright 2026 terra2o
* 
*/

#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <string.h>
#include "include/market.h"
#include "include/game_state.h"
#include "include/sim.h"
#include "include/log.h"
#include "include/ui.h"
#include "include/save.h"
#include "include/event.h"
#include "include/version.h"

#define VERSION_STRING "Tavern - Version: " GAME_VERSION


static void event_handler(Tavern* b, World* w, UiState* ui_state, int actions_per_day, UiMode mode, int* resolved)
{
    w->pending_event = EVENT_NONE;
    ui_state->mode = mode;
    *resolved = 0;
    while (!*resolved) {
        draw_ui(b, w->day, 0, actions_per_day, w, ui_state, &ui_state->war);
        int ch = getch();
        napms(16);
        if (ch != ERR)
            ui_handle_input(ch, ui_state, b, w);
    }
    ui_state->mode = UI_MODE_NORMAL;
}

int main(void) {
    srand(time(NULL));
    
    #define SAVE_PATH "tavernsavefile.txt"

    World w = {0};
    w.day = 0;
    population_init(&w.population, 100000);
    for (int i = 0; i < 150; i++) citizen_spawn(&w.population);
    w.last_advertised_day = 0;
    w.inflation_rate = 1.0f;

    Merchant m;
    m.price_per_ale = 5.0f;
    m.price_per_wine = 90.0f;
    m.quality = 0.7f;
    m.instability = 0.2f;

    Tavern b;
    b.money = 700.0f;
    b.ale_price = 5.0f;
    b.wine_price = 120.0f;
    b.ale.amount = 10;
    b.wine.amount = 2;
    b.total_inventory = 0;
    b.quality_actual = m.quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier = &m;
    b.last_pathway_clean_day = 0;

    PeriodicPayment p;
    p.pay_period = 30;
    p.next_payment_day = w.day + p.pay_period;
    p.rent_amount = 1500;
    p.base_rent = 1500;


if (!load_game(SAVE_PATH, &w, &b, &m, &p)) {
    w.day = 0;
    w.last_advertised_day = 0;
    w.inflation_rate = 1.0f;

    m.price_per_ale = 5.0f;
    m.price_per_wine = 90.0f;
    m.quality = 0.7f;
    m.instability = 0.2f;

    b.money = 700.0f;
    b.ale_price = 5.0f;
    b.wine_price = 120.0f;
    b.ale.amount = 10;
    b.wine.amount = 2;
    b.quality_actual = m.quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier = &m;
    b.last_pathway_clean_day = 0;

    p.pay_period = 30;
    p.next_payment_day = w.day + p.pay_period;
    p.rent_amount = 1500;
    p.base_rent = 1500;

    save_game(SAVE_PATH, &w, &b, &m, &p);
}

    int actions_per_day = 2;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    init_colors();
    curs_set(0);

    int game_running = 1;
    char version[64];
    snprintf(version, sizeof(version), "%s", VERSION_STRING); 
    log_message(&w.log, version, LOG_IMPORTANT);
    log_message(&w.log, "Welcome! Press a key to start the best tavern simulation ever...", LOG_IMPORTANT);

    UiState ui_state;
    ui_state_init(&ui_state);
    ui_state.war.our_kingdom_attack = w.our_kingdom_attack;

    while (game_running) {
        /* Allow multiple actions per day */
        for (int action_num = 1;
             action_num <= actions_per_day && game_running;
             action_num++) {

            while (1) {
                draw_ui(&b, w.day, action_num, actions_per_day, &w, &ui_state, &ui_state.war);

                int ch = getch();
                napms(16);

                if (ch != ERR) {
                    ui_handle_input(ch, &ui_state, &b, &w);
                }

                if (ui_state.number_input.is_confirmed != 0) {
                    ui_process_action(&ui_state, &b, &w);
                    break;
                }

                if (ui_state.mode != UI_MODE_NORMAL) {
                    continue;
                }

                Action choice = read_action(ch);

                if (choice == (Action)-1) {
                    game_running = 0;
                    break;
                }
                else if (choice == (Action)-2) {
                    continue;
                }
                else if (choice == ACT_ADVERTISE) {
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, "Advertise budget (1-10000): ", 1, 10000, 0);
                }
                else if (choice == ACT_ADJUST_ALE_PRICE) {
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, "New ale price (e.g. 3.50): ", 0.1f, 500.0f, 1);
                }
                else if (choice == ACT_ADJUST_WINE_PRICE) {
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, "New wine price (e.g. 5.00): ", 0.1f, 500.0f, 1);
                }
                else if (choice == ACT_BUY_ALE) {
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, "Buy how many mugs? ", 1, 10000, 0);
                }
                else if (choice == ACT_BUY_WINE) {
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, "Buy how many glasses? ", 1, 10000, 0);
                }
                else if (choice == ACT_CLEAN_PATHWAY) {
                    apply_action(&b, choice, &w, 0);
                    log_message(&w.log, "Cleaned pathway.", LOG_INFO);
                    break;
                }
                else {
                    apply_action(&b, choice, &w, 0);
                    log_message(&w.log, "Action completed.", LOG_INFO);
                    break;
                }
            }
        }

        if (!game_running)
            break;

        /* End of day simulation (ADVANCES w.day) */
        int sales = simulate_day(&b, &w, &p);

        /* Resolve any pending event before the next day */
        /* War events fire additionally when at war */
        if (w.at_war && w.pending_event == EVENT_NONE)
            random_war_event(&w);

        /* Check if war ends */
        if (w.at_war && w.day >= w.war_end_day) {
            w.at_war = 0;
            log_message(&w.log, "The war has ended. Peace returns to the land.", LOG_IMPORTANT);
        }

        if (w.pending_event == EVENT_FIGHT)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_FIGHT, &ui_state.fight.resolved);
        else if (w.pending_event == EVENT_VOMIT)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_VOMIT, &ui_state.vomit.resolved);
        else if (w.pending_event == EVENT_STEAL)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_STEAL, &ui_state.steal.resolved);
        else if (w.pending_event == EVENT_WAR) {
            ui_state.war.our_kingdom_attack = w.our_kingdom_attack;
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_WAR, &ui_state.war.resolved);
        } else if (w.pending_event == EVENT_WAR_SOLDIERS)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_WAR_SOLDIERS, &ui_state.war_soldiers.resolved);
        else if (w.pending_event == EVENT_WAR_REFUGEES)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_WAR_REFUGEES, &ui_state.war_refugees.resolved);
        else if (w.pending_event == EVENT_WAR_ATTACK)
            event_handler(&b, &w, &ui_state, actions_per_day, UI_MODE_WAR_ATTACK, &ui_state.war_attack.resolved);

        save_game(SAVE_PATH, &w, &b, &m, &p);

        char buf_l[256];
        snprintf(buf_l, sizeof(buf_l),
                 "End of day %d: %d sales | Money: $%.2f | Ale: %d | Wine: %d | Rep: %.2f",
                 w.day, sales, b.money, b.ale.amount, b.wine.amount, b.reputation);
        log_message(&w.log, buf_l, LOG_IMPORTANT);
    }

    population_free(&w.population);
    endwin();

    return 0;
}
