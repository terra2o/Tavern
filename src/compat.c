/*
 *
 * compat.c for Tavern
 *
 * Copyright 2026 terra2o and contributors
 *
 * Licensed under GPLv3
 *
 */

#include "compat.h"
#include <curses.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* plenty for the status/log lines this is used for */
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

#ifdef _WIN32
#include <windows.h>

/* getcurrentconsolefontex/setcurrentconsolefontex were added in vista and
   don't exist in xp's kernel32.dll. dynamically resolve to keep xp working. */
typedef BOOL(WINAPI *GetCurrentConsoleFontEx_t)(HANDLE, BOOL,
                                                PCONSOLE_FONT_INFOEX);
typedef BOOL(WINAPI *SetCurrentConsoleFontEx_t)(HANDLE, BOOL,
                                                PCONSOLE_FONT_INFOEX);

static void windows_shrink_console_font(void)
{
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    GetCurrentConsoleFontEx_t pGetCurrentConsoleFontEx;
    SetCurrentConsoleFontEx_t pSetCurrentConsoleFontEx;
    HANDLE con;
    CONSOLE_FONT_INFOEX font;

    if (!k32)
        return;

    pGetCurrentConsoleFontEx = (GetCurrentConsoleFontEx_t)GetProcAddress(
        k32, "GetCurrentConsoleFontEx");
    pSetCurrentConsoleFontEx = (SetCurrentConsoleFontEx_t)GetProcAddress(
        k32, "SetCurrentConsoleFontEx");
    if (!pGetCurrentConsoleFontEx || !pSetCurrentConsoleFontEx)
        return;

    con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return;

    memset(&font, 0, sizeof(font));
    font.cbSize = sizeof(font);
    if (pGetCurrentConsoleFontEx(con, FALSE, &font)) {
        if (font.dwFontSize.X > 8)
            font.dwFontSize.X = 8;
        if (font.dwFontSize.Y > 12)
            font.dwFontSize.Y = 12;
        pSetCurrentConsoleFontEx(con, FALSE, &font);
    }

    CloseHandle(con);
}

/* keep screen buffer sized to largest console window size to avoid conhost
 * glitches */
static void windows_grow_console_buffer(void)
{
    HANDLE con;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return;

    if (GetConsoleScreenBufferInfo(con, &csbi)) {
        COORD size = GetLargestConsoleWindowSize(con);
        SHORT win_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        SHORT win_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (size.X < win_x)
            size.X = win_x;
        if (size.Y < win_y)
            size.Y = win_y;

        if (size.X > 0 && size.Y > 0 &&
            (size.X != csbi.dwSize.X || size.Y != csbi.dwSize.Y))
            SetConsoleScreenBufferSize(con, size);
    }

    CloseHandle(con);
}

/* check if console window disagrees with pdcurses lines/cols */
static int windows_console_size_changed(void)
{
    HANDLE con;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int win_x, win_y;

    con = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                      0, NULL);
    if (con == INVALID_HANDLE_VALUE)
        return 1;

    if (!GetConsoleScreenBufferInfo(con, &csbi)) {
        CloseHandle(con);
        return 1;
    }

    win_x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    win_y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    CloseHandle(con);
    return win_x != COLS || win_y != LINES;
}

/* poll console size to detect dragged resizes on older windows console hosts */
static void windows_poll_resize(void)
{
    if (windows_console_size_changed()) {
        resize_term(0, 0);
        windows_grow_console_buffer();
    }
}

/* disable quick edit mode so mouse clicks don't stall console input */
static void windows_disable_quick_edit(void)
{
    HANDLE con = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;

    if (con == INVALID_HANDLE_VALUE)
        return;

    if (GetConsoleMode(con, &mode)) {
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        mode |= ENABLE_EXTENDED_FLAGS;
        SetConsoleMode(con, mode);
    }
}
#endif

void compat_console_init(void)
{
#ifdef _WIN32
    windows_shrink_console_font();
    windows_grow_console_buffer();
    windows_disable_quick_edit();
#endif
}

void compat_handle_resize(int ch)
{
#ifdef _WIN32
    (void)ch;
    windows_poll_resize();
#else
    if (ch == KEY_RESIZE)
        resize_term(0, 0);
#endif
}
