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
#include "include/pathway.h"
#include "include/save.h"

#define GAME_VERSION "0.3.0"

int main(void) {
	srand(time(NULL));
	
	#define SAVE_PATH "tavernsavefile.txt"

	/* Initialize game state */
	World w;
	w.day = 0;

	Merchant m;
	m.price_per_ale = 0.5f;
	m.price_per_wine = 1.0f;
	m.quality = 0.7f;
	m.instability = 0.2f;

	Tavern b;
	b.money = 10.0f;
	b.price = 1.0f;
	b.ale.amount = 0;
	b.wine.amount = 0;
	b.quality_actual = m.quality;
	b.quality_perceived = 0.5f;
	b.rumor = 0.5f;
	b.consistency = 1.0f;
	b.handsomeness = 0.6f;
	b.reputation = 0.5f;
	b.supplier = &m;

	PeriodicPayment p;
	p.pay_period = 30;
	p.next_payment_day = w.day + p.pay_period;
	p.rent_amount = 1500;
	// TODO: make inflation mechanic later, this can't be static.


if (!load_game(SAVE_PATH, &w, &b, &m, &p)) {
    /* No save exists → fresh game initialization */
    w.day = 0;

    m.price_per_ale = 0.5f;
    m.quality = 0.7f;
    m.instability = 0.2f;

    b.money = 10.0f;
    b.price = 1.0f;
    b.ale.amount = 0;
	b.wine.amount = 0;
    b.quality_actual = m.quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier = &m;

    p.pay_period = 30;
    p.next_payment_day = p.pay_period;
    p.rent_amount = 1500;

    /* Immediately create baseline save */
    save_game(SAVE_PATH, &w, &b, &m, &p);
}

	DayResult r = market_simulate(&b);

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
	// hard coding version for simplicity...
	log_message(&w.log, "Tavern - version: 0.3.0", LOG_IMPORTANT);
	log_message(&w.log, "Welcome! Press a key to start the best tavern simulation ever...", LOG_IMPORTANT);

	/* UI state for non-blocking input */
	UiState ui_state;
	ui_state_init(&ui_state);

	/* For tracking which action is waiting for input */
	Action pending_action = (Action)0;
	int pending_action_active = 0;

	/* Main game loop */
	while (game_running) {
		/* PATHWAY MECHANIC*/
		people_fall_because_pathway_dirty(&w, &b, w.day, r.customers);

		/* Clamp reputation stats between valid ranges */
		b.rumor = CLAMP(b.rumor, 0, 1);
		b.consistency = CLAMP(b.consistency, 0, 1);

		/* Allow multiple actions per day */
		for (int action_num = 1;
			 action_num <= actions_per_day && game_running;
			 action_num++) {

			while (1) {
				/* Draw UI every frame with current state */
				draw_ui(&b, w.day, action_num, actions_per_day, &w, &ui_state);

				/* Non-blocking input: read one character */
				int ch = getch();
				napms(16);

				if (ch != ERR) {
					ui_handle_input(ch, &ui_state, &w);
				}

/* ---------- NUMBER INPUT RESULT HANDLING ---------- */

				if (ui_state.number_input.is_confirmed != 0)
				{
					int input_value   = ui_state.number_input.result;
					int was_cancelled = (ui_state.number_input.is_confirmed == -1);

					if (!was_cancelled)
					{
						switch (pending_action)
						{
							case ACT_ADJUST_PRICE:
							{
								b.price = CLAMP((float)input_value / 100.0f, 0.1f, 50.0f);
								char buf[128];
								snprintf(buf, sizeof(buf),
										"Price adjusted to $%.2f", b.price);
								log_message(&w.log, buf, LOG_INFO);
								break;
							}

							case ACT_ADVERTISE:
							{
								apply_action(&b, ACT_ADVERTISE, &w, input_value);
								char buf[128];
								snprintf(buf, sizeof(buf),
										"Advertised with budget $%d", input_value);
								log_message(&w.log, buf, LOG_INFO);
								break;
							}

							case ACT_BUY_ALE:
							{
								float cost = input_value * b.supplier->price_per_ale;

								if (b.money < cost)
								{
									int affordable = (int)(b.money / b.supplier->price_per_ale);
									if (affordable <= 0)
									{
										log_message(&w.log, "Cannot afford any ale.", LOG_INFO);
									}
									else
									{
										b.money     -= affordable * b.supplier->price_per_ale;
										b.ale.amount += affordable;

										char buf[128];
										snprintf(buf, sizeof(buf),
												"Bought %d mugs", affordable);
										log_message(&w.log, buf, LOG_INFO);
									}
								}
								else
								{
									apply_action(&b, ACT_BUY_ALE, &w, input_value);

									char buf[128];
									snprintf(buf, sizeof(buf),
											"Bought %d mugs for $%.2f",
											input_value, cost);
									log_message(&w.log, buf, LOG_INFO);
								}
								break;
							}

							case ACT_BUY_WINE:
							{
								float cost = input_value * b.supplier->price_per_ale;

								if (b.money < cost)
								{
									int affordable = (int)(b.money / b.supplier->price_per_ale);
									if (affordable <= 0)
									{
										log_message(&w.log, "Cannot afford any ale.", LOG_INFO);
									}
									else
									{
										b.money     -= affordable * b.supplier->price_per_ale;
										b.ale.amount += affordable;

										char buf[128];
										snprintf(buf, sizeof(buf),
												"Bought %d mugs", affordable);
										log_message(&w.log, buf, LOG_INFO);
									}
								}
								else
								{
									apply_action(&b, ACT_BUY_ALE, &w, input_value);

									char buf[128];
									snprintf(buf, sizeof(buf),
											"Bought %d mugs for $%.2f",
											input_value, cost);
									log_message(&w.log, buf, LOG_INFO);
								}
								break;	
							}

							default:
								break;
						}
					}
					else
					{
						switch (pending_action)
						{
							case ACT_ADJUST_PRICE:
								log_message(&w.log, "Price adjustment cancelled.", LOG_INFO);
								break;
							case ACT_ADVERTISE:
								log_message(&w.log, "Advertising cancelled.", LOG_INFO);
								break;
							case ACT_BUY_ALE:
								log_message(&w.log, "Stock purchase cancelled.", LOG_INFO);
								break;
							case ACT_BUY_WINE:
								log_message(&w.log, "Stock purchase cancelled.", LOG_INFO);
								break;
							default:
								break;
						}
					}

					/* cleanup */
					ui_state.number_input.is_confirmed = 0;
					pending_action_active = 0;

					break; /* action consumed */
				}

				if (ui_state.mode != UI_MODE_NORMAL)
				{
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
					pending_action = choice;
					pending_action_active = 1;
					ui_start_number_input(&ui_state, "Advertise budget (1-1000): ", 1, 1000);	
				}
				else if (choice == ACT_ADJUST_PRICE) {
					pending_action = choice;
					pending_action_active = 1;
					ui_start_number_input(&ui_state, "What is the new price? (100 is 1 dollar)", 1, 500);
				}
				else if (choice == ACT_BUY_ALE) {
					pending_action = choice;
					pending_action_active = 1;
					ui_start_number_input(&ui_state, "Buy how many mugs? ", 1, 10000);
				}
				else if (choice == ACT_BUY_WINE) {
					pending_action = choice;
					pending_action_active = 1;
					ui_start_number_input(&ui_state, "Buy how many glasses? ", 1, 10000);
				}
				else if (choice == ACT_CLEAN_PATHWAY) {
					apply_action(&b, choice, &w, 0);
					log_message(&w.log, "Cleaned pathway.", LOG_INFO);
					break;
				}
				else {
					apply_action(&b, choice, &w, 0);

					const char* action_names[] = {
						"Skincare treatment",
						"Cleaned shop",
						"Talked to townsfolk",
						"Checked quality",
						"Advertised",
						"Adjusted price",
						"Bought stock"
					};

					char buf[128];
					snprintf(buf, sizeof(buf),
							 "Action: %s",
							 action_names[choice]);
					log_message(&w.log, buf, LOG_INFO);
					break;
				}
			}
		}

		if (!game_running)
			break;

		/* End of day simulation (ADVANCES w.day) */
		int sales = simulate_day(&b, &w, &p);

		save_game(SAVE_PATH, &w, &b, &m, &p);

		char buf[256];
		snprintf(buf, sizeof(buf),
				 "End of day %d: %d sales | Money: $%.2f | Ale: %d | Wine: %d | Rep: %.2f",
				 w.day, sales, b.money, b.ale.amount, b.wine.amount, b.reputation);
		log_message(&w.log, buf, LOG_IMPORTANT);
	}

	endwin();
	
	return 0;
}
