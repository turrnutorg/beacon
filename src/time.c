/* time.c */

/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 */

 #include "time.h"
 #include "port.h"
 #include "screen.h"
 #include "string.h"
 #include "stdlib.h"
 #include <stdint.h>
 
 #define PIT_CHANNEL0  0x40
 #define PIT_COMMAND   0x43
 #define PIT_MODE_RATE 0x34   /* Channel 0 | Access lobyte/hibyte | Mode 2 | Binary */
 #define PIT_FREQUENCY 1193182
 #define PIT_READBACK  0xE2   /* Latch status of channel 0 */
 
 static const int month_days_norm[12] = {
     31, 28, 31, 30, 31, 30,
     31, 31, 30, 31, 30, 31
 };
 static const int month_days_leap[12] = {
     31, 29, 31, 30, 31, 30,
     31, 31, 30, 31, 30, 31
 };
 
 /*─ PIT/Delay routines ──────────────────────────────────────────────────────*/
 
 void pit_init_for_polling(void) {
     uint16_t divisor = PIT_FREQUENCY / 1000; /* 1 ms tick */
     outb(PIT_COMMAND, PIT_MODE_RATE);
     outb(PIT_CHANNEL0, divisor & 0xFF);
     outb(PIT_CHANNEL0, (divisor >> 8));
 }
 
// Variable to store the number of PIT ticks
static uint32_t pit_ticks = 0;

// This function is called in PIT interrupt handler
// Increment the pit_ticks counter every time the PIT generates a tick (1ms)
void pit_tick_increment(void) {
    pit_ticks++;
}

// Function to get the current time in milliseconds
uint32_t get_time_ms(void) {
    return pit_ticks;
}

 int pit_out_high(void) {
     outb(PIT_COMMAND, PIT_READBACK);
     return inb(PIT_CHANNEL0) & 0x80; /* OUT is bit 7 */
 }
 
 void delay_ms(uint32_t ms) {
     uint32_t changes = 0;
     int prev = pit_out_high();
     while (changes < ms) {
         int now = pit_out_high();
         if (now != prev) {
             changes++;
             prev = now;
         }
     }
 }
 
 /*─ CMOS/RTC Helpers ────────────────────────────────────────────────────────*/
 
 extern uint8_t default_color;
 
 const char* get_month_name(uint8_t month) {
     static const char* months[] = {
         "Invalid", "January", "February", "March", "April", "May", "June",
         "July", "August", "September", "October", "November", "December"
     };
     if (month < 1 || month > 12) return "Invalid";
     return months[month];
 }
 
 uint8_t cmos_read(uint8_t reg) {
     outb(0x70, reg);
     return inb(0x71);
 }
 
 uint8_t bcd_to_bin(uint8_t val) {
     return (val & 0x0F) + ((val >> 4) * 10);
 }
 
 void read_rtc_datetime(uint8_t* hour, uint8_t* minute, uint8_t* second,
                        uint8_t* day, uint8_t* month, uint8_t* year) {
     /* wait until Update In Progress flag is clear */
     while (cmos_read(0x0A) & 0x80);
 
     *second = cmos_read(0x00);
     *minute = cmos_read(0x02);
     *hour   = cmos_read(0x04);
     *day    = cmos_read(0x07);
     *month  = cmos_read(0x08);
     *year   = cmos_read(0x09);
 
     *second = bcd_to_bin(*second);
     *minute = bcd_to_bin(*minute);
     *hour   = bcd_to_bin(*hour);
     *day    = bcd_to_bin(*day);
     *month  = bcd_to_bin(*month);
     *year   = bcd_to_bin(*year);
 }
 
 void display_datetime(void) {
     uint8_t hour, minute, second, day, month, year;
     char datetime_str[64] = {0};
     const char* month_name;
 
     read_rtc_datetime(&hour, &minute, &second, &day, &month, &year);
     month_name = get_month_name(month);
 
     uint8_t display_hour = hour % 12;
     if (display_hour == 0) display_hour = 12;
     const char* am_pm = (hour >= 12) ? "PM" : "AM";
 
     char hour_str[3] = {0}, min_str[3] = {0}, sec_str[3] = {0};
     char day_str[3] = {0}, year_str[5] = {0};
 
     hour_str[0] = (display_hour < 10) ? '0' : ('0' + (display_hour / 10));
     hour_str[1] = '0' + (display_hour % 10);
 
     min_str[0] = (minute < 10) ? '0' : ('0' + (minute / 10));
     min_str[1] = '0' + (minute % 10);
 
     sec_str[0] = (second < 10) ? '0' : ('0' + (second / 10));
     sec_str[1] = '0' + (second % 10);
 
     day_str[0] = (day < 10) ? '0' : ('0' + (day / 10));
     day_str[1] = '0' + (day % 10);
 
     itoa(2000 + year, year_str);
 
     memset(datetime_str, 0, sizeof(datetime_str));
     strcat(datetime_str, month_name);
     strcat(datetime_str, " ");
     strcat(datetime_str, day_str);
     strcat(datetime_str, ", ");
     strcat(datetime_str, year_str);
     strcat(datetime_str, ". ");
     strcat(datetime_str, hour_str);
     strcat(datetime_str, ":");
     strcat(datetime_str, min_str);
     strcat(datetime_str, ":");
     strcat(datetime_str, sec_str);
     strcat(datetime_str, " ");
     strcat(datetime_str, am_pm);
 
     int row = 1, col = 0;
     int offset = row * 80 + col;
     for (int i = 0; datetime_str[i] != '\0'; i++) {
         vga_buffer[offset + i].character = datetime_str[i];
         vga_buffer[offset + i].color = default_color;
     }
 }
 
 void set_rtc_time(uint8_t hour, uint8_t minute, uint8_t second) {
     uint8_t status_b = cmos_read(0x0B);
     status_b &= ~0x04; /* Use BCD */
     outb(0x70, 0x0B);
     outb(0x71, status_b);
 
     outb(0x70, 0x00);
     outb(0x71, ((second / 10) << 4) | (second % 10));
     outb(0x70, 0x02);
     outb(0x71, ((minute / 10) << 4) | (minute % 10));
     outb(0x70, 0x04);
     outb(0x71, ((hour   / 10) << 4) | (hour   % 10));
 }
 
 void set_rtc_date(uint8_t day, uint8_t month, uint8_t year) {
     /* inhibit updates */
     outb(0x70, 0x0B);
     uint8_t prev_b = inb(0x71);
     outb(0x70, 0x0B);
     outb(0x71, prev_b | 0x80);
 
     /* write day/month/year in BCD */
     outb(0x70, 0x07);
     outb(0x71, ((day   / 10) << 4) | (day   % 10));
     outb(0x70, 0x08);
     outb(0x71, ((month / 10) << 4) | (month % 10));
     outb(0x70, 0x09);
     outb(0x71, ((year  / 10) << 4) | (year  % 10));
 
     /* resume updates */
     outb(0x70, 0x0B);
     outb(0x71, prev_b & ~0x80);
 }
 
 /*─ helper: is leap year ─────────────────────────────────────────────────────*/
 static int is_leap(int year) {
     year += 1900;
     return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
 }
 
 /*─ helper: days in month ───────────────────────────────────────────────────*/
 static int days_in_month(int year, int month) {
     if (month < 0 || month > 11) return 0;
     return is_leap(year) ? month_days_leap[month] : month_days_norm[month];
 }
 
 /*─ convert struct tm to time_t (seconds since Jan 1 1970) ────────────────────*/
 time_t mktime(struct tm* t) {
     int year = t->tm_year;   /* years since 1900 */
     int mon  = t->tm_mon;    /* 0-based */
     int day  = t->tm_mday;   /* 1-based */
     int yday = 0;
     /* compute days since Jan 1 1970 */
     /* sum full years */
     int days = 0;
     for (int y = 70; y < year; y++) {
         days += ( ( (y + 1900) % 4 == 0 && ( (y + 1900) % 100 != 0 || (y + 1900) % 400 == 0 ) ) ? 366 : 365 );
     }
     /* sum months before current */
     for (int m = 0; m < mon; m++) {
         days += days_in_month(year, m);
     }
     /* day of month */
     days += (day - 1);
     yday = days;
 
     t->tm_yday = yday;
     /* compute weekday: Jan 1 1970 was Thursday (4) */
     t->tm_wday = ( (yday + 4) % 7 );
 
     /* seconds within day */
     int secs_of_day = t->tm_hour * 3600 + t->tm_min * 60 + t->tm_sec;
     return (time_t)( (time_t)days * 86400 + secs_of_day );
 }
 
 /*─ convert time_t to struct tm in UTC ───────────────────────────────────────*/
 struct tm* gmtime(const time_t* timer) {
     static struct tm gm;
     time_t t = *timer;
     int seconds = t % 60;
     t /= 60;
     int minutes = t % 60;
     t /= 60;
     int hours = t % 24;
     t /= 24; /* now t = days since Jan 1 1970 */
 
     /* compute year */
     int year = 70;
     while (1) {
         int days_in_year = ( ( (year + 1900) % 4 == 0 && ( (year + 1900) % 100 != 0 || (year + 1900) % 400 == 0 ) ) ? 366 : 365 );
         if (t < days_in_year) break;
         t -= days_in_year;
         year++;
     }
     int yday = (int)t;
 
     /* compute month & day */
     int mon = 0;
     while (1) {
         int dim = days_in_month(year, mon);
         if (t < dim) break;
         t -= dim;
         mon++;
     }
     int mday = (int)t + 1;
 
     gm.tm_sec   = seconds;
     gm.tm_min   = minutes;
     gm.tm_hour  = hours;
     gm.tm_mday  = mday;
     gm.tm_mon   = mon;
     gm.tm_year  = year;
     gm.tm_yday  = yday;
     gm.tm_wday  = ( (gm.tm_yday + 4) % 7 );  /* Jan 1 1970 was Thursday (4) */
     gm.tm_isdst = 0;
     return &gm;
 }
 
 /*─ same as gmtime (no timezone support) ────────────────────────────────────*/
 struct tm* localtime(const time_t* timer) {
     return gmtime(timer);
 }
 
 /*─ get calendar‐time as time_t ───────────────────────────────────────────── */
 time_t time(time_t* t) {
     uint8_t hour, minute, second, day, month, year;
     /* read RTC BCD values, convert them */
     read_rtc_datetime(&hour, &minute, &second, &day, &month, &year);
     struct tm tm_now;
     tm_now.tm_sec   = second;
     tm_now.tm_min   = minute;
     tm_now.tm_hour  = hour;
     tm_now.tm_mday  = day;
     tm_now.tm_mon   = (int)month - 1;       /* convert 1–12 to 0–11 */
     tm_now.tm_year  = (int)year + 100;      /* 2000→100 */
     /* tm_wday and tm_yday will be set in mktime */
     tm_now.tm_isdst = 0;
     time_t seconds = mktime(&tm_now);
     if (t) *t = seconds;
     return seconds;
 }
 
 /*─ difference in seconds as double ────────────────────────────────────────*/
 double difftime(time_t end, time_t start) {
     return (double)((int64_t)end - (int64_t)start);
 }
 
 /*─ format "Www Mmm dd hh:mm:ss yyyy\n" ────────────────────────────────────*/
 char* asctime(const struct tm* tptr) {
     static char buf[26];
     static const char* wday_name[7] = {
         "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
     };
     static const char* mon_name[12] = {
         "Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
     };
     /* tm is assumed filled by gmtime or mktime */
     int w = tptr->tm_wday;
     int m = tptr->tm_mon;
     int d = tptr->tm_mday;
     int hh = tptr->tm_hour;
     int mm = tptr->tm_min;
     int ss = tptr->tm_sec;
     int yr = tptr->tm_year + 1900;
 
     /* format with leading zeros where needed */
     snprintf(buf, sizeof(buf), "%s %s %2d %02d:%02d:%02d %4d\n",
         wday_name[w], mon_name[m], d, hh, mm, ss, yr);
     return buf;
 }
 
 /*─ convert time_t to calendar string by asctime(gmtime()) ────────────────*/
 char* ctime(const time_t* timer) {
     if (!timer) return NULL;
     struct tm* t = gmtime(timer);
     return asctime(t);
 }
 