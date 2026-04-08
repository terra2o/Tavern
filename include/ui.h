#ifndef UI_H
#define UI_H

#include "../include/sim.h"

/* UI state machine modes */
typedef enum {
    UI_MODE_NORMAL,
    UI_MODE_NUMBER_INPUT
} UiMode;

/* Number input state */
typedef struct {
    char buffer[32];
    int buffer_idx;
    int min_val;
    int max_val;
    const char* prompt;
    int result;
    int is_confirmed; /* 1 = confirmed, -1 = cancelled, 0 = pending */
} NumberInputState;

/* Global UI state for non-blocking input */
typedef struct {
    UiMode mode;
    int log_scroll_offset;
    NumberInputState number_input;
    Action pending_action;
} UiState;

/* Initialize ncurses color pairs */
void init_colors(void);

/* Initialize UI state */
void ui_state_init(UiState* state);

/* Draw the message log */
void draw_log(const MessageLog* log, int max_x, int max_y, int scroll_offset);

/* Draw the entire UI screen with status, actions, stats, and input dialogs */
void draw_ui(Tavern* b, int day, int action_num, int actions_per_day, 
             World *w, UiState* ui_state);

/* Update UI state based on a single character of input (non-blocking) */
void ui_handle_input(int ch, UiState* ui_state, World* w);

/* Start number input mode */
void ui_start_number_input(UiState* ui_state, const char* prompt, 
                           int min_val, int max_val);

/* Process a confirmed action with its parameter */
void ui_process_action(UiState* ui_state, Tavern* b, World* w);

/* Read an action key press (1-9 or Q) - only called in NORMAL mode */
Action read_action(int ch);

#endif /* UI_H */
