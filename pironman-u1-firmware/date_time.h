#pragma once

#include <Arduino.h>
#include "time.h"
#include "sntp.h"
#include "config.h"

#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600
#define DAY_LIGHT_OFFSET_SEC 3600
#define TIME_ZONE "UTC-8:00"

time_t getTimeSample();
void getTimeSampleAndDate(time_t timesample, tm timeinfo);
void timeavailable(struct timeval *t);
void ntpTimeInit();
void ntpTimeSync();
