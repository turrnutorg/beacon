#ifndef TIME_H
#define TIME_H

#include <stdint.h>

typedef uint32_t time_t;
typedef uint16_t clock_t;

struct tm {
    int tm_sec;    // seconds after the minute - [0, 60] including leap second
    int tm_min;    // minutes after the hour - [0, 59]
    int tm_hour;   // hours since midnight - [0, 23]
    int tm_mday;   // day of the month - [1, 31]
    int tm_mon;    // months since january - [0, 11]
    int tm_year;   // years since 1900
    int tm_wday;   // days since sunday - [0, 6]
    int tm_yday;   // days since january 1 - [0, 365]
    int tm_isdst;  // daylight savings time flag
};

void pit_init_for_polling(void);
int pit_out_high(void);
void delay_ms(uint32_t ms);

const char* get_month_name(uint8_t month);
uint8_t cmos_read(uint8_t reg);
uint8_t bcd_to_bin(uint8_t val);

void read_rtc_datetime(uint8_t* hour, uint8_t* minute, uint8_t* second,
                       uint8_t* day, uint8_t* month, uint8_t* year);
void display_datetime(void);
void set_rtc_time(uint8_t hour, uint8_t minute, uint8_t second);
void set_rtc_date(uint8_t day, uint8_t month, uint8_t year);

// posix-style stubs if yer feelin’ brave
time_t time(time_t* t);
double difftime(time_t end, time_t start);
time_t mktime(struct tm* timeptr);
struct tm* gmtime(const time_t* timer);
struct tm* localtime(const time_t* timer);
char* asctime(const struct tm* timeptr);
char* ctime(const time_t* timer);
uint32_t get_time_ms(void);

#endif
