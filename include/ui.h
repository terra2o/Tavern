#ifndef UI_H
#define UI_H

#include "../include/sim.h"
#include "../include/game_state.h"
#include "../include/event.h"

/* UI state machine modes */
typedef enum {
    UI_MODE_NORMAL,
    UI_MODE_NUMBER_INPUT,
    UI_MODE_FIGHT,
    UI_MODE_VOMIT,
    UI_MODE_STEAL,
    UI_MODE_WAR,
    UI_MODE_WAR_SOLDIERS,
    UI_MODE_WAR_REFUGEES,
    UI_MODE_WAR_ATTACK
} UiMode;

/* Describes the number-input prompt an action needs before it can
   run. Look these up with find_action_input_spec() instead of
   hardcoding per-action branches. */
typedef struct {
    Action action;
    const char* prompt;
    float min_val;
    float max_val;
    int is_float;
} ActionInputSpec;

/* ----- STATES ------ */

/* Number input state */
typedef struct {
    char buffer[32];
    int buffer_idx;
    int min_val;
    int max_val;
    float float_min_val;
    float float_max_val;
    const char* prompt;
    int result;
    float float_result;
    int is_float;
    int is_confirmed; /* 1 = confirmed, -1 = cancelled, 0 = pending */
} NumberInputState;

/* Global UI state for non-blocking input */
typedef struct {
    UiMode mode;
    int log_scroll_offset;
    NumberInputState number_input;
    Action pending_action;
    FightState fight;
    VomitState vomit;
    StealState steal;
    WarState war;
    WarSoldiersState war_soldiers;
    WarRefugeesState war_refugees;
    WarAttackState war_attack;
} UiState;

/* Initialize ncurses color pairs */
void init_colors(void);

/* Initialize UI state */
void ui_state_init(UiState* state);

/* Draw the message log */
void draw_log(const MessageLog* log, int max_x, int max_y, int scroll_offset);

/* Draw the entire UI screen with status, actions, stats, and input dialogs */
void draw_ui(Tavern* b, int day, int action_num, int actions_per_day, 
             World *w, UiState* ui_state, WarState* war);

/* Update UI state based on a single character of input (non-blocking) */
void ui_handle_input(int ch, UiState* ui_state, Tavern* b, World* w);

/* Start number input mode. Set is_float=1 to allow decimal input. */
void ui_start_number_input(UiState* ui_state, const char* prompt,
                           float min_val, float max_val, int is_float);

/* Process a confirmed action with its parameter */
void ui_process_action(UiState* ui_state, Tavern* b, World* w);

/* Read an action key press (1-9 or Q). Only called in NORMAL mode. */
Action read_action(int ch);

/* Look up the input spec for an action, or NULL if it runs instantly */
const ActionInputSpec* find_action_input_spec(Action a);

#endif /* UI_H */
