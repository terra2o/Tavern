/*
*
* compat.c for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "../include/compat.h"

/* Plenty for the status/log lines this is used for - none of them come
   close, since the %s args are always short static strings (drink
   names and the like), never arbitrary user input. */
#define SCRATCH_SIZE 1024

int tavern_snprintf(char *dst, size_t size, const char *fmt, ...)
{
    char scratch[SCRATCH_SIZE];
    va_list args;
    size_t len;
    size_t copy_len;

    va_start(args, fmt);
    vsprintf(scratch, fmt, args);
    va_end(args);

    len = strlen(scratch);

    if (size > 0) {
        copy_len = len < size - 1 ? len : size - 1;
        memcpy(dst, scratch, copy_len);
        dst[copy_len] = '\0';
    }

    return (int)len;
}
