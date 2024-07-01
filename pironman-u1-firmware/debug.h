#pragma once

#include <Arduino.h>
/* Set the Debug Level */
#define DEBUG_LEVEL CAM_DEBUG_LEVEL_ERROR
// #define DEBUG_LEVEL CAM_DEBUG_LEVEL_DEBUG
#define CAM_DEBUG_LEVEL_OFF 0
#define CAM_DEBUG_LEVEL_ERROR 1
#define CAM_DEBUG_LEVEL_INFO 2
#define CAM_DEBUG_LEVEL_DEBUG 3
#define CAM_DEBUG_LEVEL_ALL 4

#define DEBUG_MAX_STR_LEN 256
void debug(const char *msg, ...);
void info(const char *msg, ...);
void error(const char *msg, ...);
