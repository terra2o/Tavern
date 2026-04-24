/*
*
* main.c for Tavern
* 
* Copyright 2026 terra2o
* 
*/

#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <string.h>
#include "include/market.h"
#include "include/game_state.h"
#include "include/sim.h"
#include "include/log.h"
#include "include/ui.h"
#include "include/save.h"

#define GAME_VERSION "0.8.0"
#define VERSION_STRING "Tavern - Version: " GAME_VERSION

int main(void) {
	srand(time(NULL));
	
	#define SAVE_PATH "tavernsavefile.txt"

	/* Initialize game state */
	World w;
	w.day = 0;
    w.population = 150;
    w.last_advertised_day = 0;

	Merchant m;
    m.price_per_ale = 1.0f;
    m.price_per_wine = 25.0f;
	m.quality = 0.7f;
	m.instability = 0.2f;

	Tavern b;
	b.money = 300.0f;
	b.ale_price = 5.0f;
    b.wine_price = 100.0;
	b.ale.amount = 0;
	b.wine.amount = 0;
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
	// TODO: make inflation mechanic later, this can't be static.


if (!load_game(SAVE_PATH, &w, &b, &m, &p)) {
    /* No save exists → fresh game initialization */
    w.day = 0;
    w.population = 150;
    w.last_advertised_day = 0;

    m.price_per_ale = 1.0f;
    m.price_per_wine = 25.0f;
    m.quality = 0.7f;
    m.instability = 0.2f;

	b.money = 300.0f;
	b.ale_price = 5.0f;
    b.wine_price = 100.0;
    b.ale.amount = 0;
	b.wine.amount = 0;
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

    /* Immediately create baseline save */
    save_game(SAVE_PATH, &w, &b, &m, &p);
}

	int actions_per_day = 2;

	/* Initialize ncurses */
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE); /* Non-blocking getch() */
	init_colors();
	curs_set(0); /* Hide cursor */

	int game_running = 1;
    char version[64];
    snprintf(version, sizeof(version), "%s", VERSION_STRING); 
	log_message(&w.log, version, LOG_IMPORTANT);
	log_message(&w.log, "Welcome! Press a key to start the best tavern simulation ever...", LOG_IMPORTANT);

	/* UI state for non-blocking input */
	UiState ui_state;
	ui_state_init(&ui_state);

	/* Main game loop */
	while (game_running) {
		/* PATHWAY MECHANIC*/
		// people_fall_because_pathway_dirty(&w, &b, w.day, r.customers);

		// /* Clamp reputation stats between valid ranges */
		// b.rumor = CLAMP(b.rumor, 0, 1);
		// b.consistency = CLAMP(b.consistency, 0, 1);

		/* Allow multiple actions per day */
		for (int action_num = 1;
			 action_num <= actions_per_day && game_running;
			 action_num++) {

			while (1) {
				draw_ui(&b, w.day, action_num, actions_per_day, &w, &ui_state);

				int ch = getch();
				napms(16);

				if (ch != ERR) {
					ui_handle_input(ch, &ui_state, &w);
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
					ui_start_number_input(&ui_state, "Advertise budget (1-10000): ", 1, 10000);
				}
				else if (choice == ACT_ADJUST_ALE_PRICE) {
					ui_state.pending_action = choice;
					ui_start_number_input(&ui_state, "What is the new price? (100 is 1 dollar)", 1, 10000);
				}
				else if (choice == ACT_ADJUST_WINE_PRICE) {
					ui_state.pending_action = choice;
					ui_start_number_input(&ui_state, "What is the new price? (100 is 1 dollar)", 1, 10000);
				}
				else if (choice == ACT_BUY_ALE) {
					ui_state.pending_action = choice;
					ui_start_number_input(&ui_state, "Buy how many mugs? ", 1, 10000);
				}
				else if (choice == ACT_BUY_WINE) {
					ui_state.pending_action = choice;
					ui_start_number_input(&ui_state, "Buy how many glasses? ", 1, 10000);
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

		save_game(SAVE_PATH, &w, &b, &m, &p);

		char buf_l[256];
		snprintf(buf_l, sizeof(buf_l),
				 "End of day %d: %d sales | Money: $%.2f | Ale: %d | Wine: %d | Rep: %.2f",
				 w.day, sales, b.money, b.ale.amount, b.wine.amount, b.reputation);
		log_message(&w.log, buf_l, LOG_IMPORTANT);
	}

	endwin();
	
	return 0;
}
