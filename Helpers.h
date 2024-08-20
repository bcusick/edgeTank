#pragma once

#include <Arduino.h>
#include <mbed.h>
#include <mbed_mktime.h>


// Convert compile time to system time
time_t buildDateTimeToSystemTime(const String date, const String time, bool local_time = true, int tz = 0)
{
    char s_month[5];
    int year;

    tm t;
    time_t seconds;

    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    sscanf(date.c_str(), "%s %d %d", s_month, &t.tm_mday, &year);
    sscanf(time.c_str(), "%2d %*c %2d %*c %2d", &t.tm_hour, &t.tm_min, &t.tm_sec);

    // Find where is s_month in month_names. Deduce month value.
    t.tm_mon = (strstr(month_names, s_month) - month_names) / 3;
    t.tm_year = year - 1900;
    _rtc_maketime(&t, &seconds, RTC_FULL_LEAP_YEAR_SUPPORT);

    if (!local_time) {
        if (tz > 200) {
            tz = 0x100 - tz; // Handle negative values
            seconds += (3600UL) * tz;
        } else {
            seconds -= (3600UL) * tz;
        }
    }
    return seconds;
}

String getLocalhour()
{
    char buffer[32];
    tm t;
    _rtc_localtime(time(NULL), &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    strftime(buffer, 32, "%k:%M:%S", &t);
    return String(buffer);
}

int getHour(unsigned long epoch) {
    tm t;
    _rtc_localtime(epoch, &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    return t.tm_hour;
}

int getMinute(unsigned long epoch) {
    tm t;
    _rtc_localtime(epoch, &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    return t.tm_min;
}

String getDay()
{
    tm t;
    _rtc_localtime(time(NULL), &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    
    // Get the day of the week (0 = Sunday, 1 = Monday, ..., 6 = Saturday)
    int dayOfWeek = t.tm_wday;

    // Print the day of the week
  switch(dayOfWeek) {
    case 0:
      return ("SUN");
      break;
    case 1:
      return ("MON");
      break;
    case 2:
      return ("TUE");
      break;
    case 3:
      return ("WED");
      break;
    case 4:
      return ("THU");
      break;
    case 5:
      return ("FRI");
      break;
    case 6:
      return ("SAT");
      break;
    default:
      return ("ERR");
      break;
  }   
}

String getLocaltime()
{
    char buffer[32];
    tm t;
    _rtc_localtime(time(NULL), &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    strftime(buffer, 32, "%Y-%m-%d %k:%M:%S", &t);
    return String(buffer);
}

String getLocaltime(const time_t& build_time)
{
    char buffer[32];
    tm t;
    _rtc_localtime(build_time, &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    strftime(buffer, 32, "%Y-%m-%d %k:%M:%S", &t);
    return String(buffer);
}

/**
 * Set system clock from compile datetime or RTC
 */
void setSystemClock(String buildDate, String buildTime)
{
    // Retrieve clock time from compile date...
    auto buildDateTime = buildDateTimeToSystemTime(buildDate, buildTime, true, 2);
    //Serial.println(buildDateTime);
    // ... ore use the one from integrated RTC.
    auto rtcTime = time(NULL);
    //Serial.println(rtcTime);
    // Remember to connect at least the CR2032 battery
    // to keep the RTC running.
    auto actualTime = rtcTime > buildDateTime ? rtcTime : buildDateTime;

    // Set both system time
    set_time(actualTime);
}
