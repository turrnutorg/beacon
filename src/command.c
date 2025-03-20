/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * command.c
 */

 #include "command.h"
 #include "console.h"
 #include "screen.h"
 #include "os.h"          // For delay_ms and reboot functions
 #include "stdtypes.h"
 #include "string.h"
 #include <stdint.h>
 #include <stdarg.h>

 
 // External variables from os.c or other modules
 extern size_t curs_row;
 extern size_t curs_col;
 
 // External functions ye need declared or they’ll cry
 uint8_t cmos_read(uint8_t reg);

 // Command history buffer
 char command_history[COMMAND_HISTORY_SIZE][INPUT_BUFFER_SIZE];
 int command_history_index = 0;     // Index to store the next command
 int current_history_index = -1;    // Index for navigating history
 
 /**
  * Converts a string to lowercase.
  */
 void to_lowercase(char* str) {
     while (*str) {
         if (*str >= 'A' && *str <= 'Z') {
             *str += ('a' - 'A');
         }
         str++;
     }
 }
 
 /**
  * Parses the input command into the base command and its arguments.
  */
 int parse_command(const char* command, char* cmd, char args[MAX_ARGS][INPUT_BUFFER_SIZE]) {
     int arg_count = 0;
     const char* token = command;
     char* next_space = NULL;
 
     // Extract base command
     next_space = strchr(token, ' ');
     if (next_space) {
         strncpy(cmd, token, next_space - token);
         cmd[next_space - token] = '\0';
         token = next_space + 1;
     } else {
         strcpy(cmd, token);
         to_lowercase(cmd);
         return 0; // No arguments
     }
 
     to_lowercase(cmd);
 
     // Extract arguments
     while ((next_space = strchr(token, ' ')) != NULL && arg_count < MAX_ARGS) {
         strncpy(args[arg_count], token, next_space - token);
         args[arg_count][next_space - token] = '\0';
         token = next_space + 1;
         to_lowercase(args[arg_count]);
         arg_count++;
     }
 
     if (*token != '\0' && arg_count < MAX_ARGS) {
         strcpy(args[arg_count], token);
         to_lowercase(args[arg_count]);
         arg_count++;
     }
 
     return arg_count;
 }
 
 /**
  * Shuts down the system using ACPI.
  */
 void shutdown() {
     asm volatile ("cli"); // Disable interrupts
     while (inb(0x64) & 0x02); // Wait until the keyboard controller is ready
     outb(0x64, 0xFE); // Send the ACPI shutdown command (0xFE is used for reset, 0x2002 for ACPI)
     asm volatile ("hlt"); // Halt the CPU if shutdown fails
 }
 
 /**
  * Converts string to integer (shit version)
  */
 int atoi(const char* str) {
     int res = 0;
     int sign = 1;
     int i = 0;
 
     if (str[0] == '-') {
         sign = -1;
         i++;
     }
 
     for (; str[i] != '\0'; ++i) {
         if (str[i] < '0' || str[i] > '9')
             return 0; // not a number? yer getting zero
         res = res * 10 + str[i] - '0';
     }
 
     return sign * res;
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

 
 /**
  * Advance cursor after command
  */
 static void newRowAfterCommand() {
     curs_row++;
     if (curs_row >= NUM_ROWS) {
         scroll_screen();
         curs_row = NUM_ROWS - 1;
     }
     update_cursor();
 }
 
 /**
  * Processes the entered command.
  */
 void process_command(const char* command) {
     if (command[0] != '\0') {
         strncpy(command_history[command_history_index], command, INPUT_BUFFER_SIZE - 1);
         command_history[command_history_index][INPUT_BUFFER_SIZE - 1] = '\0'; // null-terminate
         command_history_index = (command_history_index + 1) % COMMAND_HISTORY_SIZE;
         current_history_index = -1;
     }
 
     char cmd[INPUT_BUFFER_SIZE] = {0};
     char args[MAX_ARGS][INPUT_BUFFER_SIZE] = {{0}};
     int arg_count = parse_command(command, cmd, args);
 
     to_lowercase(cmd);
 
     if (strcmp(cmd, "test") == 0) {
         if (arg_count > 0) {
             if (strcmp(args[0], "argument") == 0) {
                 println("You passed the magic argument 'argument'. Congrats, I guess.");
             } else {
                 println("Unrecognized argument. Try harder.");
             }
         } else {
             println("This is a test command, but you didn't even give me any arguments. Nice one.");
         }
 
     } else if (strcmp(cmd, "echo") == 0) {
         for (int i = 0; i < arg_count; i++) {
             print(args[i]);
             print(" ");
         }
         println("");
 
     } else if (strcmp(cmd, "clear") == 0) {
         clear_screen();
         curs_row = 0;
         row = 0;
         println("");
         move_cursor_back();
         update_cursor();
 
     } else if (strcmp(cmd, "reboot") == 0) {
         println("Rebooting the system...");
         delay_ms(1000);
         reboot();
 
     } else if (strcmp(cmd, "shutdown") == 0) {
         println("Shutting down the system...");
         delay_ms(1000);
         shutdown();
 
     } else if (strcmp(cmd, "hello") == 0) {
        println("World!");
     } else if (strcmp(cmd, "settime") == 0) {
         if (arg_count != 3) {
             println("Usage: settime <hour> <minute> <second>");
         } else {
             int hour = atoi(args[0]);
             int minute = atoi(args[1]);
             int second = atoi(args[2]);
 
             if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
                 println("Invalid time values. Use 24-hour format, dumbass.");
             } else {
                 set_rtc_time((uint8_t)hour, (uint8_t)minute, (uint8_t)second);
                 if (setup_mode == 0) {
                 println("RTC time updated. If it's wrong, that's on you.");
                 }
             }
         }
     } else if (strcmp(cmd, "setdate") == 0) {
        if (arg_count != 3) {
            println("Usage: setdate <day> <month> <year>");
        } else {
            int day = atoi(args[0]);
            int month = atoi(args[1]);
            int year = atoi(args[2]) % 100; // CMOS stores last two digits
    
            if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99) {
                println("Invalid date values, ya donkey.");
            } else {
                set_rtc_date((uint8_t)day, (uint8_t)month, (uint8_t)year);
                if (setup_mode == 0) {
                println("RTC date updated. If it's wrong, you're the moron who typed it.");
                }
            }
        }
    } else if (strcmp(cmd, "") == 0) {
         move_cursor_back();
         update_cursor();
 
     } else {
         move_cursor_back();
         print("\"");
         print(cmd);
         println("\" is not a known command or executable program.");
     }
 
     newRowAfterCommand();
 }
 