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
 #include "os.h"
 #include "stdtypes.h"
 #include "keyboard.h"
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
 
int rainbow_running = 0; // 0 means stopped, 1 means running
int rainbow_mode = 0; // 0 for both, 1 for text, 2 for background
unsigned long last_rainbow_update = 0; // For timing the updates

void update_rainbow() {
        if (!rainbow_running) return;  // Do nothing if rainbow isn't running

        for (int i = 1; i <= 15; i++) {  // Colors from 1 to 15, skipping black
            if (!rainbow_running) break; // If stopped, exit the loop

            uint8_t fg = i; 
            uint8_t bg = (rainbow_mode == 2 || rainbow_mode == 0) ? i : 0; // Cycle bg if it's "background" or "both"
            
            if (rainbow_mode == 1 || rainbow_mode == 0) {
                repaint_screen(fg, bg);  // Update text and background
            }

            if (rainbow_mode == 0) {
                repaint_screen(fg, bg);  // Update both
            }

            if (rainbow_mode == 2) {
                repaint_screen(0, bg);  // Update only background
            }

            delay_ms(100); // Speed of color change (adjustable)
        }
    }



int macos = 0; // macos mode

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


void repaint_screen(uint8_t fg_color, uint8_t bg_color) {
    uint8_t color = (bg_color << 4) | fg_color;

    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        vga_buffer[i].color = color;
    }
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
         curs_col = 0;
         row = 0;
         col = 0;
         retain_clock = 0;
         println("");
         update_cursor();
 
     } else if (strcmp(cmd, "reboot") == 0) {
         println("Rebooting the system...");
         delay_ms(1000);
         reboot();
 
     } else if (strcmp(cmd, "reset") == 0) {
        println("Resetting the screen...");
        delay_ms(1000);
        rainbow_running = 0;
        input_len = 0;
        start();

     } else if (strcmp(cmd, "rainbow") == 0) {
        if (arg_count == 0) {
            println("Usage: rainbow [text|background|both]");
        } else {
            if (strcmp(args[0], "text") == 0) {
                rainbow_mode = 1; // Text rainbow
                println("Starting rainbow text...");
            } else if (strcmp(args[0], "background") == 0) {
                rainbow_mode = 2; // Background rainbow
                println("Starting rainbow background...");
            } else if (strcmp(args[0], "both") == 0) {
                rainbow_mode = 0; // Both text and background rainbow
                println("Starting rainbow text and background...");
            } else {
                println("Invalid option. Usage: rainbow [text|background|both]");
                rainbow_mode = 0;
                return;
            }
            rainbow_running = 1; // Start rainbow cycling
        }
    }

     else if (strcmp(cmd, "shutdown") == 0) {
         println("Shutting down the system...");
         delay_ms(1000);
         shutdown();
 
     } else if (strcmp(cmd, "color") == 0) {
        if (arg_count != 2) {
            println("Usage: color <text color> <background color>");
        } else {
            int fg = atoi(args[0]);
            int bg = atoi(args[1]);
    
            if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
                println("Color values must be between 0 and 15.");
            }
                else {
    
                uint8_t color = (bg << 4) | fg;
    
                // set the default color for future prints
                set_color(fg, bg);
    
                // repaint the entire screen
                repaint_screen(fg, bg);
    
                // feedback, optional
                println("Colors updated. Try not to blind yourself.");
                }
        }
    } else if (strcmp(cmd, "hello") == 0) {
        println("World!");

     } else if (strcmp(cmd, "macos") == 0) {
        if (macos == 1) {
            set_color(GREEN_COLOR, WHITE_COLOR);
            repaint_screen(GREEN_COLOR, WHITE_COLOR);
            println("Welcome to Beacon OS. This is not macOS.");
            macos = 0;
        } else if (macos == 0) {
            set_color(WHITE_COLOR, LIGHT_BLUE_COLOR);
            repaint_screen(WHITE_COLOR, LIGHT_BLUE_COLOR);
            println("No, this is not macOS. This is Beacon OS. But I'll humor you.");
            macos = 1;
        }
        
     } else if (strcmp(cmd, "help") == 0) {
         println("Available commands:");
         println("test [argument] - Test command with optional argument.");
         println("echo [text] - Echo the text back to the console.");
         println("clear - Clear the screen.");
         println("reboot - Reboot the system.");
         println("shutdown - Shutdown the system.");
         println("hello - Print 'World!'.");
         println("help - Display this help message.");
         println("settime [hour] [minute] [second] - Set the RTC time.");
         println("setdate [day] [month] [year] - Set the RTC date.");
         println("reset - Reset the screen.");
         println("color [text color] [background color] - Change text and background colors.");
         curs_row += 11;
            update_cursor();

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
        return; // Do nothing if the command is empty
     } else {
         move_cursor_back();
         print("\"");
         print(cmd);
         println("\" is not a known command or executable program.");
     }
     newRowAfterCommand();
 }
 