/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * os.c
 */

 #include "os.h"
 #include "screen.h"
 #include "keyboard.h"
 #include "console.h"
 #include "stdtypes.h"
 #include "command.h"
 #include "string.h"
 
 // External variables from screen.c
 extern volatile struct Char* vga_buffer;
 extern uint8_t default_color;
 
 // External variables from keyboard.c
 extern char input_buffer[INPUT_BUFFER_SIZE];
 extern size_t input_len;
 
 // Cursor position from screen.c
 extern size_t curs_row;
 extern size_t curs_col;

 int setup_mode = 0;  // this has to EXIST somewhere, ya radge
 int retain_clock = 1;
 int setup_ran = 0;

 const char* get_month_name(uint8_t month) {
    static const char* months[] = {
        "Invalid", "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    if (month < 1 || month > 12)
        return "Invalid";

    return months[month];
}
 
 /**
  * Delays execution for a specified number of milliseconds.
  */
 void delay_ms(int milliseconds) {
     volatile int i = 0;
     while (i < milliseconds * 1000) {
         i++;
     }
 }
 
 void run_initial_setup() {
    setup_mode = 1;  // DATE SETUP

    println("Enter current date (DD MM YYYY):");
    input_len = strlen("setdate ");
    strncpy(input_buffer, "setdate ", INPUT_BUFFER_SIZE);
    move_cursor_back();
    curs_col = 0;
    curs_row = 3;
    update_cursor();

    // Loop until date entry is done
    while (setup_mode == 1) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            handle_keypress(scancode);

            char ascii = scancode_to_ascii(scancode);
            if (ascii == '\n') {
                input_buffer[input_len] = '\0';
                println("");
                process_command(input_buffer);

                // Move to time setup
                setup_mode = 2;
                curs_row++;
                println("Enter current time (HH MM SS):");
                input_len = strlen("settime ");
                strncpy(input_buffer, "settime ", INPUT_BUFFER_SIZE);
                move_cursor_back();
                curs_col = 0;
                update_cursor();
            }
        }
    }

    // Loop until time entry is done
    while (setup_mode == 2) {
        if (inb(0x64) & 0x01) {
            uint8_t scancode = inb(0x60);
            handle_keypress(scancode);

            char ascii = scancode_to_ascii(scancode);
            if (ascii == '\n') {
                input_buffer[input_len] = '\0';
                println("");
                process_command(input_buffer);

                // Setup complete
                setup_mode = 0;
            }
        }
    }
    setup_ran = 1;
    setup_mode = 0;
    delay_ms(500);
    clear_screen();
}


 
 /**
  * Reboots the system by sending a reset command to the keyboard controller.
  */
 void reboot() {
     asm volatile ("cli"); // Disable interrupts
     while (inb(0x64) & 0x02); // Wait until the keyboard controller is ready
     outb(0x64, 0xFE); // Send the reset command to the keyboard controller
     asm volatile ("hlt"); // Halt the CPU if reboot fails
 }
 
 /**
  * Moves the cursor to the beginning of the current line.
  */
 void move_cursor_back() {
     curs_col = 0;
     update_cursor();
 }
 
 /**
  * RTC - Read CMOS data
  */
 uint8_t cmos_read(uint8_t reg) {
     outb(0x70, reg);
     return inb(0x71);
 }
 
 /**
  * Convert BCD to binary
  */
 uint8_t bcd_to_bin(uint8_t val) {
     return (val & 0x0F) + ((val / 16) * 10);
 }
 
 /**
  * Reads time from RTC
  */
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

    // buffer for zero-padded numbers
    char hour_str[3] = {0};
    char min_str[3] = {0};
    char sec_str[3] = {0};
    char day_str[3] = {0};
    char year_str[5] = {0};

    // manually pad numbers < 10 with a zero
    hour_str[0] = (display_hour < 10) ? '0' : '0' + (display_hour / 10);
    hour_str[1] = '0' + (display_hour % 10);

    min_str[0] = (minute < 10) ? '0' : '0' + (minute / 10);
    min_str[1] = '0' + (minute % 10);

    sec_str[0] = (second < 10) ? '0' : '0' + (second / 10);
    sec_str[1] = '0' + (second % 10);

    day_str[0] = (day < 10) ? '0' : '0' + (day / 10);
    day_str[1] = '0' + (day % 10);

    itoa(2000 + year, year_str);

    // build the datetime string properly
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

    // slap it on the screen
    int row = 1;
    int col = 0;
    int offset = row * 80 + col;

    for (int i = 0; datetime_str[i] != '\0'; i++) {
        vga_buffer[offset + i].character = datetime_str[i];
        vga_buffer[offset + i].color = default_color;
    }
}

 /**
  * Entry point of the OS.
  */
  void start() {
    clear_screen();
    set_color(GREEN_COLOR, WHITE_COLOR);
    repaint_screen(GREEN_COLOR, WHITE_COLOR);

    enable_cursor(0, 15);

    if (setup_ran == 0) {
    println("The Beacon Operating System. Starting initial setup...");
    println("Copyright (c) 2025 Turrnut Open Source Organization.");

    run_initial_setup();  // <<<<<< all the dumb shit is in here now
    }
    col = 0;
    row = 0;
    setup_mode = 0;

    retain_clock = 1;
    println("Welcome to Beacon OS.");
    println("");
    display_datetime();
    println("Type a command (type 'help' for a list):");

    curs_row = 3;
    curs_col = 0;
    update_cursor();

    int loop_counter = 0;

    while (1) {
        uint8_t scancode;

        if (inb(0x64) & 0x01) {
            scancode = inb(0x60);
            handle_keypress(scancode);
        }

        if (retain_clock == 1) {
        loop_counter++;
        if (loop_counter >= 50000) {
            display_datetime();
            loop_counter = 0;
        }
        }
        update_rainbow();
    }
}

 