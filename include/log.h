#ifndef LOG_H
#define LOG_H

#define LOG_HEIGHT 10
#define LOG_LINES 100
#define LOG_LINE_LEN 128

typedef enum LogSeverity
{
    LOG_INFO, LOG_IMPORTANT, LOG_WARN, LOG_ERROR
} LogSeverity;

typedef struct LogLine
{
    char text[LOG_LINE_LEN];
    int color_pair;
} LogLine;

typedef struct MessageLog
{
    LogLine lines[LOG_LINES];
    int count;
} MessageLog;

void log_message(MessageLog *log, const char *msg, LogSeverity s);

#endif