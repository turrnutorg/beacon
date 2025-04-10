#include "time.h"
#include "port.h"
#include "screen.h"
#include "string.h"
#include <stdint.h>

extern uint8_t default_color;

const char* get_month_name(uint8_t month) {
    static const char* months[] = {
        "Invalid", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    if (month < 1 || month > 12)
        return "Invalid";

    return months[month];
}

uint8_t cmos_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

uint8_t bcd_to_bin(uint8_t val) {
    return (val & 0x0F) + ((val / 16) * 10);
}

void read_rtc_datetime(uint8_t* hour, uint8_t* minute, uint8_t* second,
    uint8_t* day, uint8_t* month, uint8_t* year) {
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

void display_datetime() {
    uint8_t hour, minute, second, day, month, year;
    char datetime_str[64] = {0};
    const char* month_name;

    read_rtc_datetime(&hour, &minute, &second, &day, &month, &year);
    month_name = get_month_name(month);

    uint8_t display_hour = hour % 12;
    if (display_hour == 0) display_hour = 12;

    const char* am_pm = (hour >= 12) ? "PM" : "AM";

    char hour_str[3] = {0};
    char min_str[3] = {0};
    char sec_str[3] = {0};
    char day_str[3] = {0};
    char year_str[5] = {0};

    hour_str[0] = (display_hour < 10) ? '0' : '0' + (display_hour / 10);
    hour_str[1] = '0' + (display_hour % 10);

    min_str[0] = (minute < 10) ? '0' : '0' + (minute / 10);
    min_str[1] = '0' + (minute % 10);

    sec_str[0] = (second < 10) ? '0' : '0' + (second / 10);
    sec_str[1] = '0' + (second % 10);

    day_str[0] = (day < 10) ? '0' : '0' + (day / 10);
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

    int row = 1;
    int col = 0;
    int offset = row * 80 + col;

    for (int i = 0; datetime_str[i] != '\0'; i++) {
        vga_buffer[offset + i].character = datetime_str[i];
        vga_buffer[offset + i].color = default_color;
    }
}

 /**
  * Writes the RTC time directly to CMOS
  */
 void set_rtc_time(uint8_t hour, uint8_t minute, uint8_t second) {
    uint8_t status_b = cmos_read(0x0B);
    status_b &= ~0x04; // Use BCD format just to stay simple
    outb(0x70, 0x0B);
    outb(0x71, status_b);

    outb(0x70, 0x00);
    outb(0x71, ((second / 10) << 4) | (second % 10));

    outb(0x70, 0x02);
    outb(0x71, ((minute / 10) << 4) | (minute % 10));

    outb(0x70, 0x04);
    outb(0x71, ((hour / 10) << 4) | (hour % 10));
}

void set_rtc_date(uint8_t day, uint8_t month, uint8_t year) {

   // stop updates
   outb(0x70, 0x0B);
   uint8_t prev_b = inb(0x71);
   outb(0x70, 0x0B);
   outb(0x71, prev_b | 0x80); // inhibit updates

   // write values
   outb(0x70, 0x07);
   outb(0x71, ((day / 10) << 4) | (day % 10));

   outb(0x70, 0x08);
   outb(0x71, ((month / 10) << 4) | (month % 10));

   outb(0x70, 0x09);
   outb(0x71, ((year / 10) << 4) | (year % 10));

   // resume updates
   outb(0x70, 0x0B);
   outb(0x71, prev_b & ~0x80);
}