#pragma once
#include <iostream>
#include <stdio.h>
#include "Core/VulkanoLog.h"

#define check(condition) \
    do { \
    if (!(condition)) { \
    VK_LOG(LOG_ERROR, "Check Failed"); \
    std::terminate(); \
    } \
    } while (false)

#define checkf(condition, text, ...) \
    do { \
    if (!(condition)) { \
    VK_LOG(LOG_ERROR, text, __VA_ARGS__); \
    std::terminate(); \
    } \
    } while (false)

#define fatal(text, ...) \
    VK_LOG(LOG_ERROR, text, __VA_ARGS__); \
    std::terminate()
    