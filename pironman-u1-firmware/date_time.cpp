#include "date_time.h"

time_t getTimeSample()
{
    time_t timesample;
    time(&timesample);
    return timesample;
}

void getTimeSampleAndDate(time_t timesample, tm timeinfo)
{
    time(&timesample);

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");

        Serial.println("set default time");
        // setTime(0, 0, 0, 1, 1, 2024);
        timeinfo.tm_year = 2024 - 1900;   // 年份从1900年开始计算
        timeinfo.tm_mon = 1 - 1;          // 月份从0开始计算（0表示1月）
        timeinfo.tm_mday = 1;             // 日期
        timeinfo.tm_hour = 0;             // 小时
        timeinfo.tm_min = 0;              // 分钟
        timeinfo.tm_sec = 0;              // 秒钟
        time_t epoch = mktime(&timeinfo); // 将 struct tm 转换为时间戳

        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr); // 设置系统时间

        getLocalTime(&timeinfo);
    }
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");

        // return;

        Serial.println("set default time");
        // setTime(0, 0, 0, 1, 1, 2024);
        timeinfo.tm_year = 2024 - 1900;   // 年份从1900年开始计算
        timeinfo.tm_mon = 1 - 1;          // 月份从0开始计算（0表示1月）
        timeinfo.tm_mday = 1;             // 日期
        timeinfo.tm_hour = 0;             // 小时
        timeinfo.tm_min = 0;              // 分钟
        timeinfo.tm_sec = 0;              // 秒钟
        time_t epoch = mktime(&timeinfo); // 将 struct tm 转换为时间戳

        struct timeval tv;
        tv.tv_sec = epoch;
        tv.tv_usec = 0;
        settimeofday(&tv, nullptr); // 设置系统时间

        getLocalTime(&timeinfo);
    }

    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    Serial.printf("timesample: %d\n", getTimeSample);

    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.println(timeStr);
}

void timeavailable(struct timeval *t)
{
    Serial.println();
    Serial.println("Got time adjustment from NTP!");
    printLocalTime();
}

void ntpTimeInit()
{
    Serial.println("\nntpTimeInit");
    // setenv("TZ", "CST-8", 1);
    setenv("TZ", config.timezone, 1); // POSIX Format: UTC+8 -> "UTC-8", UTC-8 -> "UTC8",
    tzset();
    // setTimeZone(-GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC);
    printLocalTime();
    sntp_set_time_sync_notification_cb(timeavailable);
    sntp_servermode_dhcp(1);
    // configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER);
    configTime(0, 0, config.ntpServe);
    // setenv("TZ", "CST-8", 1);
    setenv("TZ", config.timezone, 1);
    tzset();
}
