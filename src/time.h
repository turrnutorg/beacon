#ifndef TIME_H
#define TIME_H

#include "stdtypes.h"

void display_datetime();
void read_rtc_datetime(uint8_t* hour, uint8_t* minute, uint8_t* second,
    uint8_t* day, uint8_t* month, uint8_t* year);
void set_rtc_date(uint8_t day, uint8_t month, uint8_t year);
void set_rtc_time(uint8_t hour, uint8_t minute, uint8_t second);
const char* get_month_name(uint8_t month);
uint8_t cmos_read(uint8_t reg);
uint8_t bcd_to_bin(uint8_t val);

#endif