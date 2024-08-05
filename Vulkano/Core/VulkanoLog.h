#pragma once
#include <chrono>
#include <cstdio>
#include <windows.h>
#include <cstdarg>
#include <iomanip>
#include <sstream>

// Enum for log types
enum LogType {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_SUCCESS,
};

inline std::string GetCurrentTimestamp()
{
    // Get the current time
    auto now = std::chrono::system_clock::now();
    
    // Convert to time_t, which is a time representation used by the C library
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    
    // Convert to a tm structure
    std::tm now_tm;
    localtime_s(&now_tm, &now_time_t);
    
    // Get the milliseconds part
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    // Format the timestamp
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "[%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << now_ms.count() << "] ";
    
    return oss.str();
}


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
        printf("%s%s", reset, GetCurrentTimestamp().c_str());
        break;
    case LOG_WARNING:
        printf("%s%s WARNING: ", yellow, GetCurrentTimestamp().c_str());
        break;
    case LOG_ERROR:
        printf("%s%s ERROR: ", red, GetCurrentTimestamp().c_str());
        break;
    case LOG_SUCCESS:
        printf("%s%s SUCCEESS: ", green, GetCurrentTimestamp().c_str());
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
}

#define VK_LOG(type, ...) PrintLog(type, __VA_ARGS__)