/*
*
* log.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <string.h>
#include <stdarg.h>
#include "../include/log.h"

void log_message(MessageLog* log, const char* msg, LogSeverity s)
{
    int idx;

    if (log->count < LOG_LINES) {
        idx = log->count++;
    } else {
        memmove(log->lines,
                log->lines + 1,
                sizeof(LogLine) * (LOG_LINES - 1));
        idx = LOG_LINES - 1;
    }

    strncpy(log->lines[idx].text, msg, LOG_LINE_LEN - 1);
    log->lines[idx].text[LOG_LINE_LEN - 1] = '\0';
    log->lines[idx].color_pair = s;
}
