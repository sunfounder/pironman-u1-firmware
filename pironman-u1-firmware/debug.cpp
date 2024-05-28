#include "debug.h"
#include <stdarg.h>

void debug(const char *msg, ...)
{
#if (DEBUG_LEVEL >= CAM_DEBUG_LEVEL_DEBUG)
    Serial.print(F("[D] "));

    va_list args;
    va_start(args, msg);
    char buffer[DEBUG_MAX_STR_LEN];
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    Serial.print(buffer);
#endif
}

void info(const char *msg, ...)
{
#if (DEBUG_LEVEL >= CAM_DEBUG_LEVEL_INFO)
    Serial.print(F("[I] "));

    va_list args;
    va_start(args, msg);
    char buffer[DEBUG_MAX_STR_LEN];
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    Serial.print(buffer);
#endif
}

void error(const char *msg, ...)
{
#if (DEBUG_LEVEL >= CAM_DEBUG_LEVEL_ERROR)
    Serial.print(F("[E] "));

    va_list args;
    va_start(args, msg);
    char buffer[DEBUG_MAX_STR_LEN];
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    Serial.print(buffer);
#endif
}