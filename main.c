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

/* Fresh tavern with default starting stats, supplied by merchant_id.
   w is only used for w->day, to schedule the first rent payment. */
static Tavern make_starter_tavern(const World* w, int merchant_id, const Merchant* m)
{
    Tavern b = {0};
    b.money = 700.0f;
    b.drinks[DRINK_ALE].price = 5.0f;
    b.drinks[DRINK_WINE].price = 120.0f;
    b.drinks[DRINK_ALE].inventory.amount = 10;
    b.drinks[DRINK_WINE].inventory.amount = 2;
    b.last_drink_price[DRINK_ALE] = 1.0f;
    b.last_drink_price[DRINK_WINE] = 1.0f;
    b.quality_actual = m->quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier_id = merchant_id;
    b.last_pathway_clean_day = 0;
    b.rent.pay_period = 30;
    b.rent.next_payment_day = w->day + b.rent.pay_period;
    b.rent.rent_amount = 1500;
    b.rent.base_rent = 1500;
    return b;
}

static void init_new_game(World* w)
{
    w->day = 0;
    w->last_advertised_day = 0;
    w->inflation_rate = 1.0f;

    world_merchants_init(w, MAX_MERCHANTS);
    world_taverns_init(w, MAX_TAVERNS);

    Merchant m_init = {0};
    m_init.drink_price[DRINK_ALE] = 5.0f;
    m_init.drink_price[DRINK_WINE] = 90.0f;
    m_init.quality = 0.7f;
    m_init.instability = 0.2f;
    merchant_init_default_stock(&m_init);
    int merchant_id = world_add_merchant(w, m_init);

    Tavern b_init = make_starter_tavern(w, merchant_id, &m_init);
    w->player_tavern_id = world_add_tavern(w, b_init);

    /* Rival's supplier: cheaper on ale but riskier and lower quality,
       so the two starter taverns draw from genuinely different
       merchants instead of sharing one. */
    Merchant m_rival = {0};
    m_rival.drink_price[DRINK_ALE] = 4.5f;
    m_rival.drink_price[DRINK_WINE] = 90.0f;
    m_rival.quality = 0.6f;
    m_rival.instability = 0.35f;
    merchant_init_default_stock(&m_rival);
    int rival_merchant_id = world_add_merchant(w, m_rival);

    Tavern rival = make_starter_tavern(w, rival_merchant_id, &m_rival);
    rival.money = 500.0f;
    world_add_tavern(w, rival);

    world_relink_suppliers(w);
}

int main(void)
{
    srand(time(NULL));

    World w = {0};
    population_init(&w.population, 100000);
    for (int i = 0; i < 150; i++) citizen_spawn(&w.population);

    if (!load_game(SAVE_PATH, &w)) {
        init_new_game(&w);
        save_game(SAVE_PATH, &w);
    }

    Tavern* b = &w.taverns[w.player_tavern_id];

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

    char pool_buf[64];
    snprintf(pool_buf, sizeof(pool_buf), "Taverns in world: %d | Merchants: %d",
             w.tavern_count, w.merchant_count);
    log_message(&w.log, pool_buf, LOG_INFO);

    UiState ui_state;
    ui_state_init(&ui_state);
    ui_state.war.our_kingdom_attack = w.our_kingdom_attack;

    while (game_running) {
        /* Allow multiple actions per day */
        for (int action_num = 1;
             action_num <= actions_per_day && game_running;
             action_num++) {

            while (1) {
                draw_ui(b, w.day, action_num, actions_per_day, &w, &ui_state, &ui_state.war);

                int ch = getch();
                napms(16);

                if (ch != ERR)
                    ui_handle_input(ch, &ui_state, b, &w);

                if (ui_state.number_input.is_confirmed != 0) {
                    ui_process_action(&ui_state, b, &w);
                    break;
                }

                if (ui_state.mode != UI_MODE_NORMAL)
                    continue;

                if (ch == 's' || ch == 'S') {
                    ui_state.mode = UI_MODE_SUPPLIER;
                    ui_state.supplier.selected = b->supplier_id;
                    continue;
                }

                Action choice = read_action(ch);

                if (choice == (Action)-1) {
                    game_running = 0;
                    break;
                }
                else if (choice == (Action)-2)
                    continue;
                else if (find_action_input_spec(choice) != NULL) {
                    const ActionInputSpec* spec = find_action_input_spec(choice);
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, spec->prompt, spec->min_val, spec->max_val, spec->is_float);
                }
                else if (choice == ACT_CLEAN_PATHWAY) {
                    apply_action(b, choice, &w, 0);
                    log_message(&w.log, "Cleaned pathway.", LOG_INFO);
                    break;
                }
                else {
                    apply_action(b, choice, &w, 0);
                    log_message(&w.log, "Action completed.", LOG_INFO);
                    break;
                }
            }
        }

        if (!game_running)
            break;

        /* End of day simulation (ADVANCES w.day) */
        int sales = simulate_day(&w);

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
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_FIGHT, &ui_state.fight.resolved);
        else if (w.pending_event == EVENT_VOMIT)
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_VOMIT, &ui_state.vomit.resolved);
        else if (w.pending_event == EVENT_STEAL)
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_STEAL, &ui_state.steal.resolved);
        else if (w.pending_event == EVENT_WAR) {
            ui_state.war.our_kingdom_attack = w.our_kingdom_attack;
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_WAR, &ui_state.war.resolved);
        } else if (w.pending_event == EVENT_WAR_SOLDIERS)
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_WAR_SOLDIERS, &ui_state.war_soldiers.resolved);
        else if (w.pending_event == EVENT_WAR_REFUGEES)
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_WAR_REFUGEES, &ui_state.war_refugees.resolved);
        else if (w.pending_event == EVENT_WAR_ATTACK)
            event_handler(b, &w, &ui_state, actions_per_day, UI_MODE_WAR_ATTACK, &ui_state.war_attack.resolved);

        save_game(SAVE_PATH, &w);

        char buf_l[256];
        snprintf(buf_l, sizeof(buf_l),
                 "End of day %d: %d sales | Money: $%.2f | Ale: %d | Wine: %d | Rep: %.2f",
                 w.day, sales, b->money, b->drinks[DRINK_ALE].inventory.amount,
                 b->drinks[DRINK_WINE].inventory.amount, b->reputation);
        log_message(&w.log, buf_l, LOG_IMPORTANT);
    }

    population_free(&w.population);
    world_taverns_free(&w);
    world_merchants_free(&w);
    endwin();

    return 0;
}
