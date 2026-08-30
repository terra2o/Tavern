/*
*
* main.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
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

#ifdef _WIN32
#include <windows.h>
#endif

#define VERSION_STRING "Tavern - Version: " GAME_VERSION

#ifdef _WIN32
/* GetCurrentConsoleFontEx/SetCurrentConsoleFontEx were added in Vista and
   don't exist in XP's kernel32.dll. Linking against them directly makes the
   whole executable fail to load on XP with "procedure entry point ... could
   not be located", so they're resolved dynamically and skipped if absent. */
typedef BOOL (WINAPI *GetCurrentConsoleFontEx_t)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);
typedef BOOL (WINAPI *SetCurrentConsoleFontEx_t)(HANDLE, BOOL, PCONSOLE_FONT_INFOEX);

static void windows_shrink_console_font(void)
{
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    if (!k32)
        return;

    GetCurrentConsoleFontEx_t pGetCurrentConsoleFontEx =
        (GetCurrentConsoleFontEx_t)GetProcAddress(k32, "GetCurrentConsoleFontEx");
    SetCurrentConsoleFontEx_t pSetCurrentConsoleFontEx =
        (SetCurrentConsoleFontEx_t)GetProcAddress(k32, "SetCurrentConsoleFontEx");
    if (!pGetCurrentConsoleFontEx || !pSetCurrentConsoleFontEx)
        return;

    HANDLE con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_FONT_INFOEX font = {0};
    font.cbSize = sizeof(font);
    if (pGetCurrentConsoleFontEx(con, FALSE, &font)) {
        if (font.dwFontSize.X > 8) font.dwFontSize.X = 8;
        if (font.dwFontSize.Y > 12) font.dwFontSize.Y = 12;
        pSetCurrentConsoleFontEx(con, FALSE, &font);
    }

    CloseHandle(con);
}

/* Keep the console screen buffer sized to whatever the window can actually
   grow to on this screen/font, instead of padding it out to a large fixed
   size. The window can never be dragged bigger than the buffer, so some
   headroom is needed for resizing to work at all, but a buffer much bigger
   than what the screen can show creates scrollbars that the legacy Windows
   10 console host (conhost.exe) redraws very badly, causing visible
   glitching/stutter that doesn't happen on older console hosts (Vista) or
   Windows Terminal, which don't share that renderer. Bounding the buffer to
   GetLargestConsoleWindowSize() gives exactly enough room to resize up to
   full screen without any headroom beyond that. */
static void windows_grow_console_buffer(void)
{
    HANDLE con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(con, &csbi)) {
        COORD size = GetLargestConsoleWindowSize(con);
        SHORT win_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        SHORT win_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (size.X < win_x) size.X = win_x;
        if (size.Y < win_y) size.Y = win_y;

        /* SetConsoleScreenBufferSize() makes the console enqueue a fresh
           WINDOW_BUFFER_SIZE_EVENT even when called with the size it
           already has. Since this function runs from the KEY_RESIZE
           handler, calling it unconditionally turns one resize event
           into an infinite loop: handle resize -> call this -> new
           (spurious) resize event -> handle resize -> ... which starves
           real keyboard input for as long as it keeps going. Only call
           it when the size is actually changing. */
        if (size.X > 0 && size.Y > 0 &&
            (size.X != csbi.dwSize.X || size.Y != csbi.dwSize.Y))
            SetConsoleScreenBufferSize(con, size);
    }

    CloseHandle(con);
}

/* True only if the console window's actual size disagrees with what
   PDCurses thinks LINES/COLS are. Used to tell a real resize apart from
   a spurious WINDOW_BUFFER_SIZE_EVENT (including ones caused by our own
   resize handling calling SetConsoleScreenBufferSize/
   SetConsoleActiveScreenBuffer), since acting on a spurious one just
   re-triggers another such event and loops forever, starving keyboard
   input for as long as it keeps going.

   Must open "CONOUT$" rather than use GetStdHandle(STD_OUTPUT_HANDLE):
   PDCurses creates its own console screen buffer on startup and makes
   it the active (visible) one via SetConsoleActiveScreenBuffer, but
   STD_OUTPUT_HANDLE keeps pointing at the original buffer, which is now
   invisible and never resized. Querying it reads a permanently stale
   window rect, so the resize is never seen, regardless of Windows
   version. "CONOUT$" always refers to whichever buffer is currently
   active. */
static int windows_console_size_changed(void)
{
    HANDLE con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return 1;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(con, &csbi)) {
        CloseHandle(con);
        return 1;
    }

    int win_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int win_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    CloseHandle(con);
    return win_x != COLS || win_y != LINES;
}

/* KEY_RESIZE only reaches PDCurses via a WINDOW_BUFFER_SIZE_EVENT from the
   console subsystem. Modern conhost.exe (Vista and later) reliably posts
   that event for a plain window-drag resize, but XP's older csrss-hosted
   console (and possibly other pre-conhost hosts) does not, so a dragged
   resize can go completely unnoticed there. Polling the actual console size
   once per frame sidesteps the event entirely and works the same on every
   Windows version. */
static void windows_poll_resize(void)
{
    if (windows_console_size_changed()) {
        resize_term(0, 0);
        windows_grow_console_buffer();
    }
}

/* The game never uses the mouse, so PDCurses just leaves QuickEdit Mode
   at whatever the console already had (see pdc_quick_edit in
   vendor/pdcurses/wincon/pdcscrn.c and pdckbd.c). QuickEdit is on by
   default in classic Windows consoles (cmd.exe), and with it on, clicking
   into the window to focus it starts a text selection that blocks
   ReadConsoleInput (what getch() relies on) until Escape or a right click
   cancels it. That looks exactly like the game "not taking input", so
   force it off explicitly instead of relying on the user's console
   defaults. */
static void windows_disable_quick_edit(void)
{
    HANDLE con = GetStdHandle(STD_INPUT_HANDLE);
    if (con == INVALID_HANDLE_VALUE)
        return;

    DWORD mode;
    if (GetConsoleMode(con, &mode)) {
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        mode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(con, mode);
    }
}
#endif

static void event_handler(Tavern* b, Town* t, Kingdom* k, World* w, UiState* ui_state, int actions_per_day, UiMode mode, int* resolved)
{
    w->pending_event = EVENT_NONE;
    ui_state->mode = mode;
    *resolved = 0;
    while (!*resolved) {
        draw_ui(b, w->day, 0, actions_per_day, t, k, w, ui_state, &ui_state->war);
        int ch = getch();
        napms(16);
#ifdef _WIN32
        windows_poll_resize();
        if (ch != KEY_RESIZE && ch != ERR)
            ui_handle_input(ch, ui_state, b, t, k, w);
#else
        if (ch == KEY_RESIZE)
            resize_term(0, 0);
        else if (ch != ERR)
            ui_handle_input(ch, ui_state, b, t, k, w);
#endif
    }
    ui_state->mode = UI_MODE_NORMAL;
}

/* Fresh tavern with default starting stats, supplied by merchant_id.
   day is only used to schedule the first rent payment. */
static Tavern make_starter_tavern(int day, int merchant_id, const Merchant* m)
{
    Tavern b = {0};
    b.money = 700.0f;
    b.drinks[DRINK_ALE].price = 5.0f;
    b.drinks[DRINK_WINE_APPLE].price = 120.0f;
    b.drinks[DRINK_WINE_GRAPE].price = 120.0f;
    b.drinks[DRINK_ALE].inventory.amount = 10;
    b.drinks[DRINK_WINE_APPLE].inventory.amount = 2;
    b.drinks[DRINK_WINE_GRAPE].inventory.amount = 2;
    b.last_drink_price[DRINK_ALE] = 1.0f;
    b.last_drink_price[DRINK_WINE_APPLE] = 1.0f;
    b.last_drink_price[DRINK_WINE_GRAPE] = 1.0f;
    b.fruits[FRUIT_APPLE].inventory.expiration_date = 30;
    b.fruits[FRUIT_GRAPE].inventory.expiration_date = 30;
    b.fruits[FRUIT_APPLE].inventory.amount = 1;
    b.fruits[FRUIT_GRAPE].inventory.amount = 1;
    b.quality_actual = m->quality;
    b.quality_perceived = 0.5f;
    b.rumor = 0.5f;
    b.consistency = 1.0f;
    b.handsomeness = 0.6f;
    b.reputation = 0.5f;
    b.supplier_id = merchant_id;
    b.last_pathway_clean_day = 0;
    b.rent.pay_period = 30;
    b.rent.next_payment_day = day + b.rent.pay_period;
    b.rent.rent_amount = 1500;
    b.rent.base_rent = 1500;
    b.rent.next_wage_day = day + b.rent.pay_period;
    b.employees = 0;
    b.employees_wage = 500.0f;
    b.tavern_size = 1;
    return b;
}

static void init_new_game(World* w)
{
    w->day = 0;

    world_kingdoms_init(w, MAX_KINGDOMS);

    Kingdom kingdom = {0};
    kingdom.inflation_rate = 1.0f;
    kingdom.money_supply_prev = 0.0f;
    kingdom_towns_init(&kingdom, MAX_TOWNS);

    Town town = {0};
    town.last_advertised_day = 0;
    town_merchants_init(&town, MAX_MERCHANTS);
    town_taverns_init(&town, MAX_TAVERNS);
    town_cats_init(&town, ANIMALS_DEFAULT_CAPACITY);
    /* Seed a small starting colony - cats_tick() only makes kittens from
       existing mature pairs, so the town needs a handful to start with
       or it would never have any cats at all. */
    for (int i = 0; i < 4; i++) cat_spawn(&town.cats);
    population_init(&town.population, 100000);
    for (int i = 0; i < 150; i++) citizen_spawn(&town.population);

    Merchant m_init = {0};
    m_init.drink_price[DRINK_ALE] = 5.0f;
    m_init.drink_price[DRINK_WINE_APPLE] = 90.0f;
    m_init.drink_price[DRINK_WINE_GRAPE] = 90.0f;
    m_init.quality = 0.7f;
    m_init.instability = 0.2f;
    merchant_init_default_stock(&m_init);
    int merchant_id = town_add_merchant(&town, m_init);

    Tavern b_init = make_starter_tavern(w->day, merchant_id, &m_init);
    town.player_tavern_id = town_add_tavern(&town, b_init);

    /* Rival's supplier: cheaper on ale but riskier and lower quality,
       so the two starter taverns draw from genuinely different
       merchants instead of sharing one. */
    Merchant m_rival = {0};
    m_rival.drink_price[DRINK_ALE] = 4.5f;
    m_rival.drink_price[DRINK_WINE_APPLE] = 90.0f;
    m_rival.drink_price[DRINK_WINE_GRAPE] = 90.0f;
    m_rival.quality = 0.6f;
    m_rival.instability = 0.35f;
    merchant_init_default_stock(&m_rival);
    int rival_merchant_id = town_add_merchant(&town, m_rival);

    Tavern rival = make_starter_tavern(w->day, rival_merchant_id, &m_rival);
    rival.money = 500.0f;
    town_add_tavern(&town, rival);

    town_relink_suppliers(&town);

    kingdom.player_town_id = kingdom_add_town(&kingdom, town);
    w->player_kingdom_id = world_add_kingdom(w, kingdom);
}

int main(void)
{
    srand(time(NULL));

    World w = {0};

    if (!load_game(SAVE_PATH, &w)) {
        init_new_game(&w);
        save_game(SAVE_PATH, &w);
    }

    Kingdom* k = world_player_kingdom(&w);
    Town* t = world_player_town(&w);
    Tavern* b = world_player_tavern(&w);

    initscr();
#ifdef _WIN32
    windows_shrink_console_font();
    windows_grow_console_buffer();
    windows_disable_quick_edit();
#endif
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
#ifndef PDCURSES
    /* ncurses holds a lone ESC for up to ESCDELAY ms (often ~1000) in
       case it's the start of an escape sequence, which makes every ESC
       cancel/close in this game feel like it needs a second press to
       actually register. PDCurses reads console key events directly
       and has no such delay, so it doesn't need (or have) this call. */
    set_escdelay(25);
#endif
    init_colors();
    curs_set(0);

    int game_running = 1;
    char version[64];
    snprintf(version, sizeof(version), "%s", VERSION_STRING);
    log_message(&w.log, version, LOG_IMPORTANT);
    log_message(&w.log, "Welcome! Press a key to start the best tavern simulation ever...", LOG_IMPORTANT);

    char pool_buf[64];
    snprintf(pool_buf, sizeof(pool_buf), "Taverns in town: %d | Merchants: %d",
             t->tavern_count, t->merchant_count);
    log_message(&w.log, pool_buf, LOG_INFO);

    UiState ui_state;
    ui_state_init(&ui_state);
    ui_state.war.our_kingdom_attack = k->our_kingdom_attack;

    while (game_running) {
        /* Recomputed each day since hiring can change it mid-game */
        int actions_per_day = tavern_actions_per_day(b);

        /* Allow multiple actions per day */
        for (int action_num = 1;
             action_num <= actions_per_day && game_running;
             action_num++) {

            while (1) {
                draw_ui(b, w.day, action_num, actions_per_day, t, k, &w, &ui_state, &ui_state.war);

                int ch = getch();
                napms(16);

#ifdef _WIN32
                windows_poll_resize();
                if (ch != KEY_RESIZE && ch != ERR)
                    ui_handle_input(ch, &ui_state, b, t, k, &w);
#else
                if (ch == KEY_RESIZE)
                    resize_term(0, 0);
                else if (ch != ERR)
                    ui_handle_input(ch, &ui_state, b, t, k, &w);
#endif

                if (ui_state.number_input.is_confirmed != 0) {
                    ui_process_action(&ui_state, b, t, k, &w);
                    break;
                }

                if (ui_state.mode != UI_MODE_NORMAL)
                    continue;

                if (ch == 's' || ch == 'S') {
                    ui_state.mode = UI_MODE_SUPPLIER;
                    ui_state.supplier.selected = b->supplier_id;
                    continue;
                }

                if (ch == 'd' || ch == 'D') {
                    ui_state.mode = UI_MODE_DETAIL;
                    continue;
                }

                Action choice = read_action(ch);

                if (choice == (Action)-1) {
                    game_running = 0;
                    break;
                }
                else if (choice == (Action)-2)
                    continue;
                else if (choice == ACT_BUY_WINE || choice == ACT_ADJUST_WINE_PRICE) {
                    ui_state.pending_action = choice;
                    ui_state.mode = UI_MODE_WINE_VARIETY;
                }
                else if (find_action_input_spec(choice) != NULL) {
                    const ActionInputSpec* spec = find_action_input_spec(choice);
                    ui_state.pending_action = choice;
                    ui_start_number_input(&ui_state, spec->prompt, spec->min_val, spec->max_val, spec->is_float);
                }
                else if (choice == ACT_CLEAN_PATHWAY) {
                    apply_action(b, choice, t, k, &w, 0);
                    log_message(&w.log, "Cleaned pathway.", LOG_INFO);
                    break;
                }
                else if (choice == ACT_COLLECT_FRUIT) {
                    int cmax_x, cmax_y;
                    getmaxyx(stdscr, cmax_y, cmax_x);
                    collect_state_start(&ui_state.collect, cmax_x, cmax_y);
                    event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_COLLECT, &ui_state.collect.resolved);
                    log_message(&w.log, "Went out to pick fruit.", LOG_INFO);
                    break;
                }
                else {
                    apply_action(b, choice, t, k, &w, 0);
                    /* hire/expand already log their own outcome (success or
                       failure) inside apply_action, so logging a generic
                       "completed" here would contradict a failure message */
                    if (choice != ACT_HIRE_EMPLOYEES && choice != ACT_EXPAND_TAVERN)
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
        if (k->at_war && w.pending_event == EVENT_NONE)
            random_war_event(k, &w);

        /* Check if war ends */
        if (k->at_war && w.day >= k->war_end_day) {
            k->at_war = 0;
            log_message(&w.log, "The war has ended. Peace returns to the land.", LOG_IMPORTANT);
        }

        if (w.pending_event == EVENT_FIGHT)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_FIGHT, &ui_state.fight.resolved);
        else if (w.pending_event == EVENT_VOMIT)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_VOMIT, &ui_state.vomit.resolved);
        else if (w.pending_event == EVENT_STEAL)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_STEAL, &ui_state.steal.resolved);
        else if (w.pending_event == EVENT_CAT_TROUBLE)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_CAT_TROUBLE, &ui_state.cat_trouble.resolved);
        else if (w.pending_event == EVENT_WAR) {
            ui_state.war.our_kingdom_attack = k->our_kingdom_attack;
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_WAR, &ui_state.war.resolved);
        } else if (w.pending_event == EVENT_WAR_SOLDIERS)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_WAR_SOLDIERS, &ui_state.war_soldiers.resolved);
        else if (w.pending_event == EVENT_WAR_REFUGEES)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_WAR_REFUGEES, &ui_state.war_refugees.resolved);
        else if (w.pending_event == EVENT_WAR_ATTACK)
            event_handler(b, t, k, &w, &ui_state, actions_per_day, UI_MODE_WAR_ATTACK, &ui_state.war_attack.resolved);

        save_game(SAVE_PATH, &w);

        int total_wine = b->drinks[DRINK_WINE_APPLE].inventory.amount + b->drinks[DRINK_WINE_GRAPE].inventory.amount;
        char buf_l[256];
        snprintf(buf_l, sizeof(buf_l),
                 "End of day %d: %d sales | Money: $%.2f | Ale: %d | Wine: %d | Rep: %.2f",
                 w.day, sales, b->money, b->drinks[DRINK_ALE].inventory.amount,
                 total_wine, b->reputation);
        log_message(&w.log, buf_l, LOG_IMPORTANT);
    }

    world_kingdoms_free(&w);
    endwin();

    return 0;
}
