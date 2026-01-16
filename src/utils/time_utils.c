#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../../include/multi_gnss_sim.h"

// GPS time conversion functions
#define GPS_EPOCH_JULIAN 2444244.5    // January 6, 1980 00:00:00 UTC
#define SECONDS_PER_WEEK 604800.0     // 7 * 24 * 3600
#define SECONDS_PER_DAY 86400.0

// Leap seconds (as of 2024)
static int leap_seconds = 18;

// Day of year calculation
static int day_of_year(int year, int month, int day)
{
    static const int days_per_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int doy = days_per_month[month - 1] + day;
    
    // Add leap day if after February in a leap year
    if (month > 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
        doy++;
    
    return doy;
}

// Julian date calculation
static double julian_date(int year, int month, int day, int hour, int minute, double second)
{
    if (month <= 2) {
        year--;
        month += 12;
    }
    
    int a = year / 100;
    int b = 2 - a + a / 4;
    
    return (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + b - 1524.5 +
           (hour + minute / 60.0 + second / 3600.0) / 24.0;
}

// Convert UTC time to GPS time
double gps_time_from_utc(double utc_time)
{
    // Get current time
    time_t t = (time_t)utc_time;
    struct tm *tm_utc = gmtime(&t);
    
    int year = tm_utc->tm_year + 1900;
    int month = tm_utc->tm_mon + 1;
    int day = tm_utc->tm_mday;
    int hour = tm_utc->tm_hour;
    int minute = tm_utc->tm_min;
    double second = tm_utc->tm_sec + fmod(utc_time, 1.0);
    
    // Calculate Julian date
    double jd = julian_date(year, month, day, hour, minute, second);
    
    // Calculate GPS time
    double gps_time = (jd - GPS_EPOCH_JULIAN) * SECONDS_PER_DAY + leap_seconds;
    
    return gps_time;
}

// Convert GPS time to UTC time
double utc_time_from_gps(double gps_time)
{
    double jd = GPS_EPOCH_JULIAN + (gps_time - leap_seconds) / SECONDS_PER_DAY;
    
    // Convert Julian date to UTC
    jd += 0.5;
    int z = (int)jd;
    double f = jd - z;
    
    int a = z;
    if (z >= 2299161) {
        int alpha = (int)((z - 1867216.25) / 36524.25);
        a = z + 1 + alpha - alpha / 4;
    }
    
    int b = a + 1524;
    int c = (int)((b - 122.1) / 365.25);
    int d = (int)(365.25 * c);
    int e = (int)((b - d) / 30.6001);
    
    double day = b - d - (int)(30.6001 * e) + f;
    int month = (e < 14) ? e - 1 : e - 13;
    int year = (month > 2) ? c - 4716 : c - 4715;
    
    // Convert to seconds since epoch
    struct tm tm_utc = {0};
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon = month - 1;
    tm_utc.tm_mday = (int)day;
    tm_utc.tm_hour = (int)((day - (int)day) * 24);
    tm_utc.tm_min = (int)(((day - (int)day) * 24 - tm_utc.tm_hour) * 60);
    tm_utc.tm_sec = (int)(((day - (int)day) * 24 * 60 - tm_utc.tm_hour * 60 - tm_utc.tm_min) * 60);
    
    return (double)mktime(&tm_utc) + fmod(day, 1.0) * 86400.0;
}

// Convert GPS time to Galileo time
double galileo_time_from_gps(double gps_time)
{
    // Galileo System Time (GST) is aligned with GPS time within a few nanoseconds
    // For simulation purposes, we consider them identical
    return gps_time;
}

// Convert GPS time to GLONASS time
double glonass_time_from_gps(double gps_time)
{
    // GLONASS time differs from GPS time by the number of leap seconds
    // and has a 3-hour offset for Moscow time
    static const double glonass_gps_offset = 18.0 - 3.0 * 3600.0;
    return gps_time + glonass_gps_offset;
}

// Convert GPS time to BeiDou time
double beidou_time_from_gps(double gps_time)
{
    // BeiDou Time (BDT) is aligned with GPS time within a few nanoseconds
    // BDT started at 00:00:00 UTC on January 1, 2006
    static const double bdt_gps_offset = -6.0 * SECONDS_PER_DAY;
    return gps_time + bdt_gps_offset;
}

// Get GPS week number and seconds of week
void gps_week_seconds(double gps_time, int *week, double *seconds)
{
    *week = (int)(gps_time / SECONDS_PER_WEEK);
    *seconds = fmod(gps_time, SECONDS_PER_WEEK);
}

// Get day of year from GPS time
int gps_day_of_year(double gps_time)
{
    time_t t = (time_t)utc_time_from_gps(gps_time);
    struct tm *tm_utc = gmtime(&t);
    return day_of_year(tm_utc->tm_year + 1900, tm_utc->tm_mon + 1, tm_utc->tm_mday);
}

// Check if year is leap year
int is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Get number of days in month
int days_in_month(int year, int month)
{
    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year))
        return 29;
    return dim[month - 1];
}

// Format time string
void format_time(double time_sec, char *buffer, size_t buffer_size)
{
    int hours = (int)(time_sec / 3600);
    int minutes = (int)((time_sec - hours * 3600) / 60);
    double seconds = time_sec - hours * 3600 - minutes * 60;
    
    snprintf(buffer, buffer_size, "%02d:%02d:%06.3f", hours, minutes, seconds);
}

// Parse time string in format YYYY/MM/DD,hh:mm:ss
double parse_time_string(const char *time_str)
{
    int year, month, day, hour, minute;
    double second;
    
    if (sscanf(time_str, "%d/%d/%d,%d:%d:%lf", 
               &year, &month, &day, &hour, &minute, &second) != 6) {
        return -1.0;
    }
    
    struct tm tm = {0};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = (int)second;
    
    return (double)mktime(&tm) + fmod(second, 1.0);
}