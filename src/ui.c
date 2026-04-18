/*
*
* ui.c for "Tavern"
*
* Copyright 2026 terra2o
*
*/

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "../include/ui.h"
#include "../include/log.h"
#include "../include/game_state.h"

#define COLOR_MONEY 1
#define COLOR_WARNING 2
#define COLOR_YELLOW 3
#define COLOR_NORMAL 4
#define COLOR_IMPORTANT 5

/* Initialize UI state */
void ui_state_init(UiState* state)
{
    state->mode = UI_MODE_NORMAL;
    state->log_scroll_offset = 0;
    state->pending_action = (Action)0;
    memset(&state->number_input, 0, sizeof(state->number_input));
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

    for (int i = start; i < log->count && y < max_y; i++) 
    {
        attron(COLOR_PAIR(log->lines[i].color_pair));
        mvprintw(y++, 2, "%.*s", max_x - 4, log->lines[i].text);
        attroff(COLOR_PAIR(log->lines[i].color_pair));
    }
}

void init_colors(void)
{
    start_color();
    init_pair(COLOR_MONEY, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_WARNING, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(COLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_IMPORTANT, COLOR_BLUE, COLOR_BLACK);
}

void draw_ui(Tavern *b, int day, int action_num, int actions_per_day, World *w, UiState* ui_state)
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

    mvprintw(3, 2, "Ale: %d mugs", b->ale.amount);
    mvprintw(4, 2, "Ale Price: $%.2f", b->ale_price);
    mvprintw(5, 2, "Wine: %d glasses", b->wine.amount);
    mvprintw(6, 2, "Wine Price: $%.2f", b->wine_price);

    color = (b->reputation < 0.3f) ? COLOR_YELLOW : COLOR_NORMAL;
    attron(COLOR_PAIR(color));
    mvprintw(7, 2, "Reputation: %.2f", b->reputation);
    attroff(COLOR_PAIR(color));

    mvprintw(8, 2, "Quality (actual): %.2f", b->quality_actual);
    mvprintw(9, 2, "Quality (perceived): %.2f", b->quality_perceived);
    mvprintw(10, 2, "Rumor: %.2f", b->rumor);
    mvprintw(11, 2, "Consistency: %.2f", b->consistency);
    mvprintw(12, 2, "Handsomeness: %.2f", b->handsomeness);

    /* Draw left panel border */
    for (int y = 0; y < usable_height; y++)
    {
        mvaddch(y, left_width, ACS_VLINE);
    }

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
    mvprintw(9, right_start + 2, "   (%.2f per mug)", b->supplier->price_per_ale);
    mvprintw(10, right_start + 2, "8 - Buy wine stock");
    mvprintw(11, right_start + 2, "   (%.2f per glass)", b->supplier->price_per_wine);
    mvprintw(12, right_start + 2, "W - Adjust ale price");
    mvprintw(13, right_start + 2, "E - Adjust wine price");
    mvprintw(14, right_start + 2, "Q - Quit game");

    /* --- BOTTOM LOG AREA (always drawn with scroll_offset support) --- */
    draw_log(&w->log, max_x, max_y, ui_state->log_scroll_offset);

    /* --- NUMBER INPUT OVERLAY (if in input mode) --- */
    if (ui_state->mode == UI_MODE_NUMBER_INPUT)
    {
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

    refresh();
}

/* Start number input mode - initializes the state */
void ui_start_number_input(UiState* ui_state, const char* prompt, 
                           int min_val, int max_val)
{
    ui_state->mode = UI_MODE_NUMBER_INPUT;
    ui_state->number_input.prompt = prompt;
    ui_state->number_input.min_val = min_val;
    ui_state->number_input.max_val = max_val;
    ui_state->number_input.result = 0;
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

    if (ch == 27) /* ESC */
    {
        ni->is_confirmed = -1; /* cancelled */
        ui_state->mode = UI_MODE_NORMAL;
    }
    else if (ch == '\n' || ch == '\r')
    {
        if (ni->buffer_idx == 0)
        {
            /* Empty input, ignore */
            return;
        }
        ni->buffer[ni->buffer_idx] = '\0';
        int val = atoi(ni->buffer);
        if (val >= ni->min_val && val <= ni->max_val)
        {
            ni->result = val;
            ni->is_confirmed = 1; /* confirmed */
            ui_state->mode = UI_MODE_NORMAL;
        }
        else
        {
            /* Invalid range, reset for re-entry */
            ni->buffer_idx = 0;
            memset(ni->buffer, 0, sizeof(ni->buffer));
        }
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
    {
        if (ni->buffer_idx > 0)
        {
            ni->buffer_idx--;
            ni->buffer[ni->buffer_idx] = '\0';
        }
    }
    else if (isdigit(ch) && ni->buffer_idx < (int)sizeof(ni->buffer) - 1)
    {
        ni->buffer[ni->buffer_idx++] = ch;
    }
    else if (ch == KEY_UP || ch == KEY_DOWN)
    {
        /* Allow scrolling even in input mode */
        if (ch == KEY_UP)
            ui_state->log_scroll_offset++;
        else
            ui_state->log_scroll_offset--;
        
        if (ui_state->log_scroll_offset < 0)
            ui_state->log_scroll_offset = 0;
    }
}

/* Update UI state based on input - handles mode-specific logic */
void ui_handle_input(int ch, UiState* ui_state, World* w)
{
    int max_scroll = w->log.count - (LOG_HEIGHT - 1);
    if (max_scroll < 0) max_scroll = 0;

    if (ui_state->log_scroll_offset > max_scroll)
        ui_state->log_scroll_offset = max_scroll;

    if (ui_state->mode == UI_MODE_NUMBER_INPUT)
    {
        ui_handle_number_input(ch, ui_state, w);
    }
    else if (ui_state->mode == UI_MODE_NORMAL)
    {
        /* In normal mode, handle scrolling */
        if (ch == KEY_UP)
            ui_state->log_scroll_offset++;
        else if (ch == KEY_DOWN)
            ui_state->log_scroll_offset--;
        
        if (ui_state->log_scroll_offset < 0)
            ui_state->log_scroll_offset = 0;
    }
}

/* Convert a character to an action (only valid in NORMAL mode) */
Action read_action(int ch)
{
    if (ch == 'q' || ch == 'Q')
    {
        return (Action)-1; /* Signal to quit */
    }

    if (ch >= '1' && ch <= '8')
    {
        return (Action)(ch - '1');
    }
    if (ch == 'w')
    {
        return(Action)(ACT_ADJUST_ALE_PRICE);
    }
    if (ch == 'e')
    {
        return(Action)(ACT_ADJUST_WINE_PRICE);
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
                b->ale_price = CLAMP((float)input_value / 100.0f, 0.1f, 50.0f);
                char buf[128];
                snprintf(buf, sizeof(buf), "Price adjusted to $%.2f", b->ale_price);
                log_message(&w->log, buf, LOG_INFO);
                break;
            }
            case ACT_ADJUST_WINE_PRICE:
            {
                b->wine_price = CLAMP((float)input_value / 100.0f, 0.1f, 50.0f);
                char buf[128];
                snprintf(buf, sizeof(buf), "Price adjusted to $%.2f", b->wine_price);
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
                float cost = input_value * b->supplier->price_per_ale;

                if (b->money < cost) {
                    int affordable = (int)(b->money / b->supplier->price_per_ale);
                    if (affordable <= 0) {
                        log_message(&w->log, "Cannot afford any ale.", LOG_INFO);
                    }
                    else {
                        b->money -= affordable * b->supplier->price_per_ale;
                        b->ale.amount += affordable;
                        b->total_inventory = b->ale.amount + b->wine.amount;

                        char buf[128];
                        snprintf(buf, sizeof(buf), "Bought %d mugs", affordable);
                        log_message(&w->log, buf, LOG_INFO);
                    }
                }
                else {
                    b->money -= cost;
                    b->ale.amount += input_value;
                    b->total_inventory = b->ale.amount + b->wine.amount;

                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Bought %d mugs for $%.2f",
                             input_value, cost);
                    log_message(&w->log, buf, LOG_INFO);
                }
                break;
            }

            case ACT_BUY_WINE:
            {
                float cost = input_value * b->supplier->price_per_wine;

                if (b->money < cost) {
                    int affordable = (int)(b->money / b->supplier->price_per_wine);
                    if (affordable <= 0) {
                        log_message(&w->log, "Cannot afford any wine.", LOG_INFO);
                    }
                    else {
                        b->money -= affordable * b->supplier->price_per_wine;
                        b->wine.amount += affordable;
                        b->total_inventory = b->ale.amount + b->wine.amount;

                        char buf[128];
                        snprintf(buf, sizeof(buf), "Bought %d glasses", affordable);
                        log_message(&w->log, buf, LOG_INFO);
                    }
                }
                else {
                    b->money -= cost;
                    b->wine.amount += input_value;
                    b->total_inventory = b->ale.amount + b->wine.amount;

                    char buf[128];
                    snprintf(buf, sizeof(buf),
                             "Bought %d glasses for $%.2f",
                             input_value, cost);
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
