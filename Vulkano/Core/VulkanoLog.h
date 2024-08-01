#pragma once
#include <cstdio>
#include <windows.h>
#include <cstdarg>

// Enum for log types
enum LogType {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_SUCCESS,
};

inline void SetConsoleColor(LogType type)
{
    // ANSI escape codes for colors
    const char* red = "\033[31m";
    const char* green = "\033[32m";
    const char* yellow = "\033[33m";
    const char* reset = "\033[0m";
    
    switch (type)
    {
    case LOG_INFO:
        printf("%s", reset);
        break;
    case LOG_WARNING:
        printf("%s WARNING: ", yellow);
        break;
    case LOG_ERROR:
        printf("%s ERROR: ", red);
        break;
    case LOG_SUCCESS:
        printf("%s SUCCEESS: ", green);
        break;
    }
}

inline void PrintLog(LogType Type, const char* format, ...)
{
    SetConsoleColor(Type);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    fflush(stdout); 
    // Reset to default color
    SetConsoleColor(LOG_INFO);
}

#define VK_LOG(type, ...) PrintLog(type, __VA_ARGS__)