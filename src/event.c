/*
*
* event.c for "Tavern"
*
* Copyright 2026 terra2o
*
*/

#include <stdlib.h>
#include "../include/event.h"
#include "../include/sim.h"

#define FIGHT_MEDICAL_COST 200.0f

/* these are just "simple" events, for example, war isn't here */
static char array_of_events[4][32] = {"fight", "vomit", "steal"};
#define NUM_EVENTS (sizeof(array_of_events) / sizeof(array_of_events[0]))

void event_fight(World* w)
{
    w->pending_event = EVENT_FIGHT;
}

void event_vomit(World* w)
{
    w->pending_event = EVENT_VOMIT;
}

void event_steal(World* w)
{
    w->pending_event = EVENT_STEAL;
}

void event_war(World* w)
{
    w->pending_event = EVENT_WAR;
    w->at_war = 1;
    w->our_kingdom_attack = rand() % 2;
    w->war_end_day = w->day + 30 + rand() % 61; /* war lasts 30-90 days */
}

void random_war_event(World* w)
{
    if (!w->at_war) return;

    int roll = rand() % 3;
    if (roll == 0)
        w->pending_event = EVENT_WAR_SOLDIERS;
    else if (roll == 1)
        w->pending_event = EVENT_WAR_REFUGEES;
    /* roll == 2: nothing happens this day */
}

int event_fight_break_up(Tavern* b, World* w)
{
    if (rand() % 2 == 0) {
        b->reputation += 0.30f;
        b->rumor += 0.30f;
        b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
        b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
        log_message(&w->log, "You stepped in and broke up the fight. Patrons are impressed.", LOG_INFO);
        return 1;
    } else {
        b->money -= FIGHT_MEDICAL_COST;
        log_message(&w->log, "You tried to break up the fight but got hurt. Medical bill: $200.", LOG_WARN);
        return 0;
    }
}

int handle_steal(int ch, Tavern* b, World* w)
{
    switch (ch)
    {
        case(1):
            if (rand() % 2 == 0) {
                b->reputation += 0.30f;
                b->rumor += 0.30f;
                b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
                b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
                log_message(&w->log, "You punched him and he ran away. Folk are impressed", LOG_INFO);
                return 1;
            } else {
                b->money -= FIGHT_MEDICAL_COST;
                b->reputation -= 0.30f;
                b->rumor -= 0.30f;
                b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
                b->rumor = CLAMP(b->rumor, 0.0f, 1.0f);
                log_message(&w->log, "Turns out, the son of a bitch is stronger than you! You pay for medical expenses, people make fun of you", LOG_WARN);
                return 0;
            }
            break; 
        case(2):
            b->money -= 50;
            b->reputation -= 0.15f;
            b->rumor -= 0.15f;
            log_message(&w->log, "You called the guards. People think you're a pussy", LOG_INFO);
            return 1;
            break; 
        case(3):
            b->money -= 100;
            b->ale.amount -= 5;
            log_message(&w->log, "You ignored the thief. People didn't see anything. You obviously lost some booze, and money.", LOG_INFO);
            return 1;
            break; 
    }
    return 0;
}

void handle_war_declaration(int choice, Tavern* b, World* w)
{
    if (w->our_kingdom_attack) {
        /* Our kingdom is the aggressor */
        switch (choice) {
        case 1: /* back the conquest */
            if (rand() % 2 == 0) {
                b->reputation += 0.20f;
                log_message(&w->log, "You cheered on the conquest. Nationalists love you for it.", LOG_INFO);
            } else {
                b->reputation -= 0.15f;
                log_message(&w->log, "You backed the war. Several pacifist regulars walk out in disgust.", LOG_WARN);
            }
            break;
        case 2: /* side with the defenders */
            if (rand() % 4 == 0) {
                b->reputation += 0.10f;
                log_message(&w->log, "You spoke against the war. A few moralists quietly respect you.", LOG_INFO);
            } else {
                b->reputation -= 0.40f;
                log_message(&w->log, "Traitor! You sided with the enemy. Locals want you out.", LOG_WARN);
            }
            break;
        case 3: /* stay out of it */
            if (rand() % 2 == 0) {
                b->reputation -= 0.10f;
                log_message(&w->log, "You stayed neutral. Warmongers call you a coward.", LOG_WARN);
            } else {
                b->reputation -= 0.25f;
                log_message(&w->log, "Neutrality in a war of aggression. Many regulars think you lack spine.", LOG_WARN);
            }
            break;
        }
    } else {
        /* Our kingdom is being attacked */
        switch (choice) {
        case 1: /* defend the kingdom */
            if (rand() % 3 < 2) { /* 66% */
                b->reputation += 0.30f;
                log_message(&w->log, "You rallied for the kingdom's defense. Patriots are proud to drink here.", LOG_INFO);
            } else {
                b->reputation += 0.05f;
                log_message(&w->log, "You pledged to defend. Most approve, though a few war-weary patrons sigh.", LOG_INFO);
            }
            break;
        case 2: /* side with the attackers */
            if (rand() % 5 == 0) {
                b->reputation -= 0.20f;
                log_message(&w->log, "You sided with the invaders. A few foreign merchants nod. Everyone else seethes.", LOG_WARN);
            } else {
                b->reputation -= 0.50f;
                log_message(&w->log, "Collaborator! Word spreads like fire. Half your regulars swear never to return.", LOG_WARN);
            }
            break;
        case 3: /* stay out of it */
            if (rand() % 2 == 0) {
                b->reputation -= 0.20f;
                log_message(&w->log, "You refused to take sides while your kingdom burns. People are disgusted.", LOG_WARN);
            } else {
                b->reputation -= 0.10f;
                log_message(&w->log, "You stayed neutral. Some understand, most don't.", LOG_WARN);
            }
            break;
        }
    }
    b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
}

void handle_war_soldiers(int choice, Tavern* b, World* w)
{
    switch (choice) {
    case 1: /* give free drinks */
        b->money -= 200.0f;
        b->reputation += 0.20f;
        log_message(&w->log, "Free drinks for the troops. They'll remember your generosity.", LOG_INFO);
        break;
    case 2: /* charge half */
        b->money -= 100.0f;
        if (rand() % 2 == 0) {
            b->reputation += 0.05f;
            log_message(&w->log, "Half price it is. The soldiers pay without complaint.", LOG_INFO);
        } else {
            b->reputation -= 0.10f;
            log_message(&w->log, "Half price. The sergeant mutters you're a greedy innkeeper.", LOG_WARN);
        }
        break;
    case 3: /* refuse */
        if (rand() % 2 == 0) {
            b->reputation -= 0.10f;
            log_message(&w->log, "You refused them. They leave grumbling but cause no trouble.", LOG_WARN);
        } else {
            b->money -= 150.0f;
            b->reputation -= 0.30f;
            log_message(&w->log, "You refused them. The soldiers trash the place on their way out.", LOG_WARN);
        }
        break;
    }
    b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
}

void handle_war_refugees(int choice, Tavern* b, World* w)
{
    switch (choice) {
    case 1: /* welcome them */
        if (rand() % 10 < 6) { /* 60% */
            b->reputation += 0.25f;
            w->population += 15;
            log_message(&w->log, "You sheltered the refugees. Word of your kindness spreads.", LOG_INFO);
        } else {
            b->reputation -= 0.10f;
            log_message(&w->log, "Regulars are annoyed by the crowded tavern and stop coming.", LOG_WARN);
        }
        break;
    case 2: /* charge entry */
        if (rand() % 10 < 7) { /* 70% */
            b->money += 300.0f;
            b->reputation -= 0.05f;
            log_message(&w->log, "You charged the refugees. Business is business.", LOG_INFO);
        } else {
            b->reputation -= 0.25f;
            log_message(&w->log, "Word got out you squeezed desperate refugees for coin. People are disgusted.", LOG_WARN);
        }
        break;
    case 3: /* turn away */
        if (rand() % 2 == 0) {
            b->reputation += 0.05f;
            log_message(&w->log, "A few regulars agree: no room for more. Rep holds.", LOG_INFO);
        } else {
            b->reputation -= 0.40f;
            log_message(&w->log, "Turning away refugees in wartime. Word spreads you're heartless.", LOG_WARN);
        }
        break;
    }
    b->reputation = CLAMP(b->reputation, 0.0f, 1.0f);
}
void random_event(World* w)
{
    int chance = rand() % 4; /* 25% chance */
    int chance_war = rand() % 10; /* 10% chance */

    if (chance == 3) /* the 25% chance was "true" and now we do random events */ { 
        int event_index = rand() % NUM_EVENTS;

        switch (event_index) {
        case 0:
            event_fight(w);
            break;
        case 1:
            event_vomit(w);
            break;
        case 2:
            event_steal(w);
            break;
        }
    }
    /* only start a new war if not already at war */
    else if (chance_war == 9 && !w->at_war) {
        event_war(w);
    }
}
