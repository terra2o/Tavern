/*
*
* ui.c for "Tavern"
*
* Copyright 2026 terra2o
*
*/

#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/ui.h"
#include "../include/log.h"
#include "../include/game_state.h"
#include "../include/event.h"

#define COLOR_MONEY 1
#define COLOR_WARNING 2
#define COLOR_YELLOW 3
#define COLOR_NORMAL 4
#define COLOR_IMPORTANT 5

#define STRING_ARRAY_MAX 32
#define STRING_ARRAY_LEN 256
#define PUSH_STR(arr, count, s) \
    strncpy((arr)[(count)++], (s), STRING_ARRAY_LEN - 1)
/*
 * Global string array for
 * typing stuff inside boxes.
 * Gets cleared everytime we
 * call `draw_centered_box()`
 */
char string_array[STRING_ARRAY_MAX][STRING_ARRAY_LEN];
int  string_array_count = 0;

/* Initialize UI state */
void ui_state_init(UiState* state)
{
    state->mode = UI_MODE_NORMAL;
    state->log_scroll_offset = 0;
    state->pending_action = (Action)0;
    memset(&state->number_input, 0, sizeof(state->number_input));
    state->fight.resolved = 0;
    state->vomit.resolved = 0;
    state->steal.resolved = 0;
    state->supplier.selected = 0;
}

int color_for_severity(LogSeverity s) 
{
    switch (s) {
        case LOG_INFO:  return 4;
        case LOG_IMPORTANT: return 5;
        case LOG_WARN:  return 3;
        case LOG_ERROR: return 3;
    }
    return 0;
}

void draw_log(const MessageLog* log, int max_x, int max_y, int scroll_offset) 
{
    int start_y = max_y - LOG_HEIGHT;

    // separator
    for (int x = 0; x < max_x; x++)
        mvaddch(start_y, x, ACS_HLINE);

    int lines_to_show = LOG_HEIGHT - 1;
    int start = log->count - lines_to_show - scroll_offset;
    if (start < 0) start = 0;
    int max_start = log->count - lines_to_show;
    if (max_start < 0) max_start = 0;
    if (start > max_start) start = max_start;

    int y = start_y + 1;

    // int color_for_severity(LogSeverity s)
    // {
    //     switch (s) {
    //         case LOG_INFO:  return 4;
    //         case LOG_IMPORTANT: return 5;
    //         case LOG_WARN:  return 3;
    //         case LOG_ERROR: return 3;
    //     }
    //     return 0;
    // }

    for (int i = start; i < log->count && y < max_y; i++) {
        attron(COLOR_PAIR(log->lines[i].color_pair));
        mvprintw(y++, 2, "%.*s", max_x - 4, log->lines[i].text);
        attroff(COLOR_PAIR(log->lines[i].color_pair));
    }
}

void init_colors(void)
{
    start_color();
#ifdef TAVERN_DEFAULT_COLORS
    use_default_colors();
#endif // TAVERN_DEFAULT_COLORS
    init_pair(COLOR_MONEY, COLOR_GREEN, -1);
    init_pair(COLOR_WARNING, COLOR_RED, -1);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, -1);
    init_pair(COLOR_NORMAL, COLOR_WHITE, -1);
    init_pair(COLOR_IMPORTANT, COLOR_BLUE, -1);
}

void string_array_clear(void)
{
    memset(string_array, 0, sizeof(string_array));
    string_array_count = 0;
}

void draw_centered_box(int box_w, 
                    int box_h, 
                    int max_x, 
                    int max_y,
                    char* title)
{
    int box_x = (max_x - box_w) / 2;
    int box_y = (max_y - LOG_HEIGHT - box_h) / 2;

    /* Box border */
    mvaddch(box_y, box_x, ACS_ULCORNER);
    mvaddch(box_y, box_x + box_w - 1, ACS_URCORNER);
    mvaddch(box_y + box_h - 1, box_x, ACS_LLCORNER);
    mvaddch(box_y + box_h - 1, box_x + box_w - 1, ACS_LRCORNER);

    for (int x = box_x + 1; x < box_x + box_w - 1; x++) {
        mvaddch(box_y, x, ACS_HLINE);
        mvaddch(box_y + box_h - 1, x, ACS_HLINE);
    }
    for (int y = box_y + 1; y < box_y + box_h - 1; y++) {
        mvaddch(y, box_x, ACS_VLINE);
        mvaddch(y, box_x + box_w - 1, ACS_VLINE);
    }

    /* Clear interior */
    for (int y = box_y + 1; y < box_y + box_h - 1; y++)
        for (int x = box_x + 1; x < box_x + box_w - 1; x++)
            mvaddch(y, x, ' ');

    attron(A_BOLD | COLOR_PAIR(COLOR_WARNING));
    mvprintw(box_y + 1, box_x + 2, "%s", title);
    attroff(A_BOLD | COLOR_PAIR(COLOR_WARNING));

    for (int i = 0; i < string_array_count; i++)
        mvprintw(box_y + 3 + i, box_x + 2, "%.*s", box_w - 4, string_array[i]);

    string_array_clear();
}

void draw_ui(Tavern *b, int day, int action_num, int actions_per_day, World *w, UiState* ui_state, WarState* war)
{
    int max_x, max_y;
    getmaxyx(stdscr, max_y, max_x);
    int usable_height = max_y - LOG_HEIGHT;

    erase();

    int left_width = max_x / 2 - 1;
    int right_start = left_width + 1;

    /* --- LEFT PANEL: Status --- */
    attron(A_BOLD);
    mvprintw(0, 2, "DAY %d", day);
    attroff(A_BOLD);

    int color = (b->money >= 0) ? COLOR_MONEY : COLOR_WARNING;
    attron(COLOR_PAIR(color));
    mvprintw(2, 2, "Money: $%.2f", b->money);
    attroff(COLOR_PAIR(color));

    mvprintw(3, 2, "Ale: %d mugs", b->drinks[DRINK_ALE].inventory.amount);
    mvprintw(4, 2, "Ale Price: $%.2f", b->drinks[DRINK_ALE].price);
    mvprintw(5, 2, "Wine: %d glasses", b->drinks[DRINK_WINE].inventory.amount);
    mvprintw(6, 2, "Wine Price: $%.2f", b->drinks[DRINK_WINE].price);

    color = (b->reputation < 0.3f) ? COLOR_YELLOW : COLOR_NORMAL;
    attron(COLOR_PAIR(color));
    mvprintw(7, 2, "Reputation: %.2f", b->reputation);
    attroff(COLOR_PAIR(color));

    mvprintw(8, 2, "Quality (actual): %.2f", b->quality_actual);
    mvprintw(9, 2, "Quality (perceived): %.2f", b->quality_perceived);
    mvprintw(10, 2, "Rumor: %.2f", b->rumor);
    mvprintw(11, 2, "Consistency: %.2f", b->consistency);
    mvprintw(12, 2, "Handsomeness: %.2f", b->handsomeness);
    mvprintw(13, 2, "Population: %d", w->population.alive_count);
    mvprintw(14, 2, "Pathway dirtiness: %d/7", (w->day - b->last_pathway_clean_day));

    float avg_thirst, avg_addiction;
    population_stats(&w->population, &avg_thirst, &avg_addiction);
    mvprintw(17, 2, "Town thirst: %.0f%%", avg_thirst * 100.0f);
    mvprintw(18, 2, "Town addiction: %.0f%%", avg_addiction * 100.0f);

    if (w->at_war) {
        attron(A_BOLD | COLOR_PAIR(COLOR_WARNING));
        if (w->our_kingdom_attack)
            mvprintw(16, 2, "** AT WAR (your kingdom is attacking) **");
        else
            mvprintw(16, 2, "** AT WAR (your kingdom is defending) **");
        attroff(A_BOLD | COLOR_PAIR(COLOR_WARNING));
    }

    float inf_pct = (w->inflation_rate - 1.0f) * 100.0f;
    int inf_color = (inf_pct >= 25.0f) ? COLOR_WARNING : (inf_pct >= 10.0f) ? COLOR_YELLOW : COLOR_NORMAL;
    attron(COLOR_PAIR(inf_color));
    mvprintw(15, 2, "Inflation: +%.1f%%", inf_pct);
    attroff(COLOR_PAIR(inf_color));

    /* Draw left panel border */
    for (int y = 0; y < usable_height; y++)
        mvaddch(y, left_width, ACS_VLINE);

    /* --- RIGHT PANEL: Actions --- */
    attron(A_BOLD);
    mvprintw(0, right_start + 2, "ACTIONS (%d/%d)", action_num, actions_per_day);
    attroff(A_BOLD);

    mvprintw(2, right_start + 2, "1 - Skin care");
    mvprintw(3, right_start + 2, "2 - Clean shop");
    mvprintw(4, right_start + 2, "3 - Talk to townsfolk");
    mvprintw(5, right_start + 2, "4 - Check drink quality");
    mvprintw(6, right_start + 2, "5 - Advertise");
    mvprintw(7, right_start + 2, "6 - Clean pathway");
    mvprintw(8, right_start + 2, "7 - Buy ale stock");
    mvprintw(9, right_start + 2, "   (%.2f per mug)", merchant_quote_price(b->supplier, b->id, DRINK_ALE));
    mvprintw(10, right_start + 2, "8 - Buy wine stock");
    mvprintw(11, right_start + 2, "   (%.2f per glass)", merchant_quote_price(b->supplier, b->id, DRINK_WINE));
    mvprintw(12, right_start + 2, "W - Adjust ale price");
    mvprintw(13, right_start + 2, "E - Adjust wine price");
    mvprintw(14, right_start + 2, "S - View/switch suppliers");
    mvprintw(15, right_start + 2, "Q - Quit game");

    /* --- BOTTOM LOG AREA (always drawn with scroll_offset support) --- */
    draw_log(&w->log, max_x, max_y, ui_state->log_scroll_offset);

    /* --- NUMBER INPUT OVERLAY (if in input mode) --- */
    if (ui_state->mode == UI_MODE_NUMBER_INPUT) {
        NumberInputState* ni = &ui_state->number_input;
        int input_y = max_y - 8;
        
        /* Semi-transparent overlay effect using windows */
        attron(A_DIM);
        mvprintw(input_y, 2, "%s", ni->prompt);
        mvprintw(input_y + 1, 2, "Value: ");

        /* Print buffer with highlight */
        attron(A_REVERSE);
        mvprintw(input_y + 1, 9, "%-20s", ni->buffer);
        attroff(A_REVERSE);
        attroff(A_DIM);

        mvprintw(input_y + 3, 2, "Enter number and press ENTER");
        mvprintw(input_y + 4, 2, "Press ESC to cancel");
        mvprintw(input_y + 5, 2, "UP/DOWN: scroll log");
    }

    /* --- FIGHT EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_FIGHT) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "Two patrons are throwing fists.");
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1 - Call the guard   ($50, rep -0.15)");
        PUSH_STR(string_array, string_array_count, "2 - Break it up      (rep +0.30, risky)");
        PUSH_STR(string_array, string_array_count, "3 - Ignore it        (rep -0.30)");

        draw_centered_box(52, 11, max_x, max_y, "!! A BRAWL HAS BEGUN !!");
    }
    /* --- VOMIT EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_VOMIT) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1 - Clean it up yourself    (-20 handsomeness, rep -0.30)");
        PUSH_STR(string_array, string_array_count, "2 - Pay someone to clean it ($500, rep +0.30)");
        PUSH_STR(string_array, string_array_count, "3 - Ignore it               (-50 customers next day, rep -0.30)");

        draw_centered_box(52, 11, max_x, max_y, "!! SOMEONE JUST PUKED EVERYWHERE !!");
    }
    /* --- STEAL EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_STEAL) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1 - Punch him in the face (rep +0.30, risky)");
        PUSH_STR(string_array, string_array_count, "2 - Call the guard        ($50, rep -0.15)");
        PUSH_STR(string_array, string_array_count, "3 - Ignore it             ($200)");

        draw_centered_box(52, 11, max_x, max_y, "!! SOMEONE IS TRYING TO STEAL SOME BOOZE !!");
    }
    /* --- WAR SOLDIERS EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_WAR_SOLDIERS) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "A squad of soldiers marches in demanding free drinks.");
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1 - Give them free drinks  ($200, rep +0.20)");
        PUSH_STR(string_array, string_array_count, "2 - Charge them half price ($100, rep +0.05)");
        PUSH_STR(string_array, string_array_count, "3 - Refuse them            (rep -0.30, risky)");

        draw_centered_box(60, 11, max_x, max_y, "!! SOLDIERS DEMAND DRINKS !!");
    }
    /* --- WAR REFUGEES EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_WAR_REFUGEES) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "A crowd of war refugees seeks shelter in your tavern.");
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1 - Welcome them (rep +0.25, pop +20)");
        PUSH_STR(string_array, string_array_count, "2 - Charge entry ($300, rep -0.10)");
        PUSH_STR(string_array, string_array_count, "3 - Turn them away (rep -0.40)");

        draw_centered_box(60, 11, max_x, max_y, "!! WAR REFUGEES ARRIVE !!");
    }
    /* --- WAR EVENT OVERLAY --- */
    if (ui_state->mode == UI_MODE_WAR && war->our_kingdom_attack == 1) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1. Support your kingdom  (risky)");
        PUSH_STR(string_array, string_array_count, "2. Support the defenders (uncertain)");
        PUSH_STR(string_array, string_array_count, "3. Ignore it             (rep -0.50, not risky)");

        draw_centered_box(60, 11, max_x, max_y, "!! YOUR KINGDOM JUST STARTED ATTACKING ANOTHER KINGDOM !!");
    } else if (ui_state->mode == UI_MODE_WAR && war->our_kingdom_attack == 0) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1. Support your kingdom  (not THAT risky)");
        PUSH_STR(string_array, string_array_count, "2. Support the attackers (risky)");
        PUSH_STR(string_array, string_array_count, "3. Ignore it             (rep -0.50, not risky)");

        draw_centered_box(60, 11, max_x, max_y, "!! YOUR KINGDOM JUST STARTED BEING ATTACKED BY ANOTHER KINGDOM !!");
    }

    /* --- SUPPLIER SCREEN --- */
    if (ui_state->mode == UI_MODE_SUPPLIER) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "UP/DOWN to pick, ENTER to switch, ESC to cancel.");
        PUSH_STR(string_array, string_array_count, "");
        for (int i = 0; i < w->merchant_count; i++) {
            const Merchant* m = &w->merchants[i];
            char line[160];
            char marker = (i == ui_state->supplier.selected) ? '>' : ' ';
            char current = (i == b->supplier_id) ? '*' : ' ';
            snprintf(line, sizeof(line),
                     "%c%c #%d  qual %.2f  risk %.2f  ale $%.2f (stock %d)  wine $%.2f (stock %d)  favor %.2f",
                     marker, current, i, m->quality, m->instability,
                     merchant_quote_price(m, b->id, DRINK_ALE), merchant_available_stock(m, DRINK_ALE),
                     merchant_quote_price(m, b->id, DRINK_WINE), merchant_available_stock(m, DRINK_WINE),
                     m->tavern_favor[b->id]);
            PUSH_STR(string_array, string_array_count, line);
        }
        draw_centered_box(78, 8 + w->merchant_count, max_x, max_y, "SUPPLIERS  (* = current)");
    }

    if (ui_state->mode == UI_MODE_WAR_ATTACK) {
        string_array_count = 0;
        PUSH_STR(string_array, string_array_count, "What do you do?");
        PUSH_STR(string_array, string_array_count, "");
        PUSH_STR(string_array, string_array_count, "1. Play it cool       (risky)");
        PUSH_STR(string_array, string_array_count, "2. Go and attack them (very risky)");
    
        draw_centered_box(60, 11, max_x, max_y, "!! THE OTHER SIDE'S TROOPS COME BY YOUR TAVERN !!");
    }

    refresh();
}

/* Start number input mode, initializing the state. */
void ui_start_number_input(UiState* ui_state, const char* prompt,
                           float min_val, float max_val, int is_float)
{
    ui_state->mode = UI_MODE_NUMBER_INPUT;
    ui_state->number_input.prompt = prompt;
    ui_state->number_input.float_min_val = min_val;
    ui_state->number_input.float_max_val = max_val;
    ui_state->number_input.min_val = (int)min_val;
    ui_state->number_input.max_val = (int)max_val;
    ui_state->number_input.result = 0;
    ui_state->number_input.float_result = 0.0f;
    ui_state->number_input.is_float = is_float;
    ui_state->number_input.is_confirmed = 0;
    ui_state->number_input.buffer_idx = 0;
    memset(ui_state->number_input.buffer, 0, sizeof(ui_state->number_input.buffer));
}

/* Handle input while in number input mode */
static void ui_handle_number_input(int ch, UiState* ui_state, World* w)
{
    int max_scroll = w->log.count - (LOG_HEIGHT - 1);
    if (max_scroll < 0) max_scroll = 0;

    if (ui_state->log_scroll_offset > max_scroll)
        ui_state->log_scroll_offset = max_scroll;

    NumberInputState* ni = &ui_state->number_input;

    if (ch == 27) { /* ESC */
        ni->is_confirmed = -1; /* cancelled */
        ui_state->mode = UI_MODE_NORMAL;
    }
    else if (ch == '\n' || ch == '\r') {
        if (ni->buffer_idx == 0) {
            /* Empty input, ignore */
            return;
        }
        ni->buffer[ni->buffer_idx] = '\0';
        if (ni->is_float) {
            float val = (float)atof(ni->buffer);
            if (val >= ni->float_min_val && val <= ni->float_max_val) {
                ni->float_result = val;
                ni->is_confirmed = 1;
                ui_state->mode = UI_MODE_NORMAL;
            }
            else {
                ni->buffer_idx = 0;
                memset(ni->buffer, 0, sizeof(ni->buffer));
            }
        }
        else {
            int val = atoi(ni->buffer);
            if (val >= ni->min_val && val <= ni->max_val) {
                ni->result = val;
                ni->is_confirmed = 1; /* confirmed */
                ui_state->mode = UI_MODE_NORMAL;
            }
            else {
                /* Invalid range, reset for re-entry */
                ni->buffer_idx = 0;
                memset(ni->buffer, 0, sizeof(ni->buffer));
            }
        }
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (ni->buffer_idx > 0) {
            ni->buffer_idx--;
            ni->buffer[ni->buffer_idx] = '\0';
        }
    }
    else if (isdigit(ch) && ni->buffer_idx < (int)sizeof(ni->buffer) - 1)
        ni->buffer[ni->buffer_idx++] = ch;
    else if (ch == '.' && ni->is_float && ni->buffer_idx < (int)sizeof(ni->buffer) - 1
             && strchr(ni->buffer, '.') == NULL) {
        ni->buffer[ni->buffer_idx++] = ch;
    }
    else if (ch == KEY_UP || ch == KEY_DOWN) {
        /* Allow scrolling even in input mode */
        if (ch == KEY_UP)
            ui_state->log_scroll_offset++;
        else
            ui_state->log_scroll_offset--;
        
        if (ui_state->log_scroll_offset < 0)
            ui_state->log_scroll_offset = 0;
    }
}

static void ui_handle_fight(int ch, UiState* ui_state, Tavern* b, World* w)
{
    // NOTE: maybe i should refactor this? or ui.c in general because
    // i don't want logic in ui.c
    switch (ch) {
    case '1':
        if (b->money >= 50.0f) {
            b->money -= 50.0f;
            b->reputation -= 0.15f;
            b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
            log_message(&w->log, "You called the guard. The brawlers were removed. Cost: $50.", LOG_INFO);
        } else {
            b->reputation -= 0.15;
            b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
            log_message(&w->log, "You called the guard but couldn't cover the fee. Rep took a hit.", LOG_WARN);
        }
        ui_state->fight.resolved = 1;
        break;
    case '2':
        event_fight_break_up(b, w);
        ui_state->fight.resolved = 1;
        break;
    case '3':
        b->reputation -= 0.30f;
        b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
        log_message(&w->log, "You ignored the brawl. Several customers left in disgust.", LOG_WARN);
        ui_state->fight.resolved = 1;
        break;
    }
}

static void ui_handle_vomit(int ch, UiState* ui_state, Tavern* b, World* w)
{
    switch (ch) {
    case '1':
        b->handsomeness -= 0.20;
        b->reputation -= 0.30;
        b->rumor -= 0.30;
        ui_state->vomit.resolved = 1;
        break;
    case '2':
        b->money -= 500;
        b->reputation += 0.30;
        b->rumor += 0.30;
        ui_state->vomit.resolved = 1;
        break;
    case '3':
        b->reputation -= 0.30f;
        b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
        log_message(&w->log, "You ignored it. People are disgusted.", LOG_WARN);
        ui_state->vomit.resolved = 1;
        break;
    }
}

static void ui_handle_war(int ch, UiState* ui_state, Tavern* b, World* w)
{
    if (ch < '1' || ch > '3') return;
    handle_war_declaration(ch - '0', b, w);
    ui_state->war.resolved = 1;
}

static void ui_handle_war_soldiers(int ch, UiState* ui_state, Tavern* b, World* w)
{
    if (ch < '1' || ch > '3') return;
    handle_war_soldiers(ch - '0', b, w);
    ui_state->war_soldiers.resolved = 1;
}

static void ui_handle_war_refugees(int ch, UiState* ui_state, Tavern* b, World* w)
{
    if (ch < '1' || ch > '3') return;
    handle_war_refugees(ch - '0', b, w);
    ui_state->war_refugees.resolved = 1;
}

static void ui_handle_war_attack(int ch, UiState* ui_state, Tavern* b, World* w)
{
    if (ch < '1' || ch > '2') return;
    handle_war_attack(ch - '0', b, w);
    ui_state->war_attack.resolved = 1;
}

static void ui_handle_steal(int ch, UiState* ui_state, Tavern* b, World* w)
{
    switch (ch) {
    case '1':
        handle_steal(1, b, w);
        ui_state->steal.resolved = 1;
        break;
    case '2':
        handle_steal(2, b, w);
        ui_state->steal.resolved = 1;
        break;
    case '3':
        handle_steal(3, b, w);
        ui_state->steal.resolved = 1;
        break;
    }
}
static void ui_handle_supplier(int ch, UiState* ui_state, Tavern* b, World* w)
{
    SupplierState* s = &ui_state->supplier;

    if (ch == 27) { /* ESC */
        ui_state->mode = UI_MODE_NORMAL;
    }
    else if (ch == KEY_UP) {
        if (s->selected > 0) s->selected--;
    }
    else if (ch == KEY_DOWN) {
        if (s->selected < w->merchant_count - 1) s->selected++;
    }
    else if (ch == '\n' || ch == '\r') {
        if (s->selected != b->supplier_id && s->selected >= 0 && s->selected < w->merchant_count) {
            char buf[128];
            b->supplier_id = s->selected;
            b->supplier = &w->merchants[s->selected];
            snprintf(buf, sizeof(buf), "Switched suppliers to merchant #%d.", s->selected);
            log_message(&w->log, buf, LOG_INFO);
        }
        ui_state->mode = UI_MODE_NORMAL;
    }
}

/* Updates UI state based on input, handling mode-specific logic. */
void ui_handle_input(int ch, UiState* ui_state, Tavern* b, World* w)
{
    int max_scroll = w->log.count - (LOG_HEIGHT - 1);
    if (max_scroll < 0) max_scroll = 0;

    if (ui_state->log_scroll_offset > max_scroll)
        ui_state->log_scroll_offset = max_scroll;

    if (ui_state->mode == UI_MODE_NUMBER_INPUT) {
        ui_handle_number_input(ch, ui_state, w);
    } else if (ui_state->mode == UI_MODE_FIGHT) {
        ui_handle_fight(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_VOMIT) {
        ui_handle_vomit(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_STEAL) {
        ui_handle_steal(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_WAR) {
        ui_handle_war(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_WAR_SOLDIERS) {
        ui_handle_war_soldiers(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_WAR_REFUGEES) {
        ui_handle_war_refugees(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_WAR_ATTACK) {
        ui_handle_war_attack(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_SUPPLIER) {
        ui_handle_supplier(ch, ui_state, b, w);
    } else if (ui_state->mode == UI_MODE_NORMAL) {
        /* In normal mode, handle scrolling */
        if (ch == KEY_UP)
            ui_state->log_scroll_offset++;
        else if (ch == KEY_DOWN)
            ui_state->log_scroll_offset--;

        if (ui_state->log_scroll_offset < 0)
            ui_state->log_scroll_offset = 0;
    }
}

/* Actions that need a number/text input before they take effect.
   Single source of truth for whether an action needs input and what
   the prompt looks like. Adding an input-driven action means adding
   one row here, not a new branch in main.c's dispatch. */
static const ActionInputSpec ACTION_INPUT_SPECS[] = {
    { ACT_ADVERTISE,          "Advertise budget (1-10000): ", 1,    10000, 0 },
    { ACT_ADJUST_ALE_PRICE,   "New ale price (e.g. 3.50): ",  0.1f, 500.0f, 1 },
    { ACT_ADJUST_WINE_PRICE,  "New wine price (e.g. 5.00): ", 0.1f, 500.0f, 1 },
    { ACT_BUY_ALE,            "Buy how many mugs? ",          1,    10000, 0 },
    { ACT_BUY_WINE,           "Buy how many glasses? ",       1,    10000, 0 },
};

const ActionInputSpec* find_action_input_spec(Action a)
{
    for (size_t i = 0; i < sizeof(ACTION_INPUT_SPECS) / sizeof(ACTION_INPUT_SPECS[0]); i++) {
        if (ACTION_INPUT_SPECS[i].action == a)
            return &ACTION_INPUT_SPECS[i];
    }
    return NULL;
}

/* Explicit key -> action bindings. Order here has no relation to
   Action's enum order, so adding/reordering actions can't silently
   shift which key does what. */
static const struct { int key; Action action; } ACTION_KEYS[] = {
    { '1', ACT_SKINCARE },
    { '2', ACT_CLEAN },
    { '3', ACT_TALK },
    { '4', ACT_CHECK_QUALITY },
    { '5', ACT_ADVERTISE },
    { '6', ACT_CLEAN_PATHWAY },
    { '7', ACT_BUY_ALE },
    { '8', ACT_BUY_WINE },
    { 'w', ACT_ADJUST_ALE_PRICE },
    { 'W', ACT_ADJUST_ALE_PRICE },
    { 'e', ACT_ADJUST_WINE_PRICE },
    { 'E', ACT_ADJUST_WINE_PRICE },
};

/* Convert a character to an action (only valid in NORMAL mode) */
Action read_action(int ch)
{
    if (ch == 'q' || ch == 'Q')
        return (Action)-1; /* Signal to quit */

    for (size_t i = 0; i < sizeof(ACTION_KEYS) / sizeof(ACTION_KEYS[0]); i++) {
        if (ACTION_KEYS[i].key == ch)
            return ACTION_KEYS[i].action;
    }

    return (Action)-2; /* Invalid input */
}

/* Process a confirmed action with its parameter */
void ui_process_action(UiState* ui_state, Tavern* b, World* w)
{
    if (ui_state->number_input.is_confirmed == 0)
        return;

    int input_value = ui_state->number_input.result;
    int was_cancelled = (ui_state->number_input.is_confirmed == -1);

    if (!was_cancelled) {
        switch (ui_state->pending_action) {
            case ACT_ADJUST_ALE_PRICE:
            {
                float fval = ui_state->number_input.float_result;
                b->drinks[DRINK_ALE].price = CLAMP(fval, 0.1f, 500.0f);
                char buf[128];
                snprintf(buf, sizeof(buf), "Price adjusted to $%.2f", b->drinks[DRINK_ALE].price);
                log_message(&w->log, buf, LOG_INFO);
                break;
            }
            case ACT_ADJUST_WINE_PRICE:
            {
                float fval = ui_state->number_input.float_result;
                b->drinks[DRINK_WINE].price = CLAMP(fval, 0.1f, 500.0f);
                char buf[128];
                snprintf(buf, sizeof(buf), "Price adjusted to $%.2f", b->drinks[DRINK_WINE].price);
                log_message(&w->log, buf, LOG_INFO);
                break;
            }
            case ACT_ADVERTISE:
            {
                apply_action(b, ACT_ADVERTISE, w, input_value);
                char buf[128];
                snprintf(buf, sizeof(buf), "Advertised with budget $%d", input_value);
                log_message(&w->log, buf, LOG_INFO);
                break;
            }

            case ACT_BUY_ALE:
            {
                int in_stock = merchant_available_stock(b->supplier, DRINK_ALE);
                float unit_price = merchant_quote_price(b->supplier, b->id, DRINK_ALE);
                int want = input_value < in_stock ? input_value : in_stock;

                if (want <= 0) {
                    log_message(&w->log, "The merchant is out of ale today.", LOG_INFO);
                    break;
                }

                float cost = want * unit_price;

                if (b->money < cost) {
                    int affordable = (int)(b->money / unit_price);
                    if (affordable > want) affordable = want;
                    if (affordable <= 0)
                        log_message(&w->log, "Cannot afford any ale.", LOG_INFO);
                    else {
                        b->money -= affordable * unit_price;
                        b->drinks[DRINK_ALE].inventory.amount += affordable;
                        b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
                        merchant_record_purchase(b->supplier, b->id, DRINK_ALE, affordable);

                        char buf[128];
                        snprintf(buf, sizeof(buf), "Bought %d mugs", affordable);
                        log_message(&w->log, buf, LOG_INFO);
                    }
                }
                else {
                    b->money -= cost;
                    b->drinks[DRINK_ALE].inventory.amount += want;
                    b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
                    merchant_record_purchase(b->supplier, b->id, DRINK_ALE, want);

                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Bought %d mugs for $%.2f",
                             want, cost);
                    log_message(&w->log, buf, LOG_INFO);
                }
                break;
            }

            case ACT_BUY_WINE:
            {
                int in_stock = merchant_available_stock(b->supplier, DRINK_WINE);
                float unit_price = merchant_quote_price(b->supplier, b->id, DRINK_WINE);
                int want = input_value < in_stock ? input_value : in_stock;

                if (want <= 0) {
                    log_message(&w->log, "The merchant is out of wine today.", LOG_INFO);
                    break;
                }

                float cost = want * unit_price;

                if (b->money < cost) {
                    int affordable = (int)(b->money / unit_price);
                    if (affordable > want) affordable = want;
                    if (affordable <= 0)
                        log_message(&w->log, "Cannot afford any wine.", LOG_INFO);
                    else {
                        b->money -= affordable * unit_price;
                        b->drinks[DRINK_WINE].inventory.amount += affordable;
                        b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
                        merchant_record_purchase(b->supplier, b->id, DRINK_WINE, affordable);

                        char buf[128];
                        snprintf(buf, sizeof(buf), "Bought %d glasses", affordable);
                        log_message(&w->log, buf, LOG_INFO);
                    }
                }
                else {
                    b->money -= cost;
                    b->drinks[DRINK_WINE].inventory.amount += want;
                    b->total_inventory = b->drinks[DRINK_ALE].inventory.amount + b->drinks[DRINK_WINE].inventory.amount;
                    merchant_record_purchase(b->supplier, b->id, DRINK_WINE, want);

                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Bought %d glasses for $%.2f",
                             want, cost);
                    log_message(&w->log, buf, LOG_INFO);
                }
                break;
            }

            default:
                break;
        }
    }
    else {
        switch (ui_state->pending_action) {
            case ACT_ADJUST_ALE_PRICE:
                log_message(&w->log, "Price adjustment cancelled.", LOG_INFO);
                break;
            case ACT_ADJUST_WINE_PRICE:
                log_message(&w->log, "Price adjustment cancelled.", LOG_INFO);
                break;
            case ACT_ADVERTISE:
                log_message(&w->log, "Advertising cancelled.", LOG_INFO);
                break;
            case ACT_BUY_ALE:
                log_message(&w->log, "Stock purchase cancelled.", LOG_INFO);
                break;
            case ACT_BUY_WINE:
                log_message(&w->log, "Stock purchase cancelled.", LOG_INFO);
                break;
            default:
                break;
        }
    }

    ui_state->number_input.is_confirmed = 0;
    ui_state->pending_action = (Action)0;
}
