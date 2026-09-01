/*
*
* compat.h for Tavern
*
* Copyright 2026 terra2o and contributors
*
* Licensed under GPLv3
*
*/

#ifndef COMPAT_H
#define COMPAT_H

#include <stddef.h>

/* snprintf isn't part of C89, so old compilers (DJGPP 2.x, Watcom, etc)
   don't have it. Behaves like the real thing: truncates to size-1 chars
   plus a NUL, and returns the length the formatted string would've had
   if it fit. */
int tavern_snprintf(char *dst, size_t size, const char *fmt, ...);

#endif /* COMPAT_H */
