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
 #include "keyboard.h"
 #include "string.h"
 #include "speaker.h"
 #include "time.h"
 #include "csa.h"
 #include "serial.h"
 #include <stdint.h>
 #include <stdarg.h>
 #include <stdbool.h>
 
 // External variables from os.c or other modules
 extern size_t curs_row;
 extern size_t curs_col;

 extern void* g_mb_info;

 extern int simas_main(void);
 // Command history buffer
 char command_history[COMMAND_HISTORY_SIZE][INPUT_BUFFER_SIZE];
 int command_history_index = 0;     // Index to store the next command
 int current_history_index = -1;    // Index for navigating history
 
int rainbow_running = 0; // 0 means stopped, 1 means running
int rainbow_mode = 0; // 0 for both, 1 for text, 2 for background
unsigned long last_rainbow_update = 0; // For timing the updates

void reset() {
    start();
}

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
        // uncomment this to make all args lowercase
        //  to_lowercase(args[arg_count]);
        arg_count++;
    }
    
    if (*token != '\0' && arg_count < MAX_ARGS) {
        strcpy(args[arg_count], token);
        // uncomment this to make all args lowercase
        //  to_lowercase(args[arg_count]);
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
        if (arg_count < 1) {
            println("Usage: echo [repeat_count] \"text\"");
        } else {
            long long repeat_count = atoi(args[0]);
            if (repeat_count <= 0) {
                println("Repeat count must be a positive number.");
            } else {
                for (int i = 0; i < repeat_count; i ++) {
                    for (int i = 1; i < arg_count; i++) {
                        print(args[i]);
                        print(" ");
                    }
                    println("");
                    curs_row ++;
                    curs_col = 0;
                }
                println("");

            }
        }
    } else if (strcmp(cmd, "clear") == 0) {
         clear_screen();
         curs_row = 0;
         curs_col = 0;
         row = 0;
         col = 0;
         retain_clock = 1;
         println("Copyright (c) 2025 Turrnut Open Source Organization.");
         println("");
         display_datetime();
         update_cursor();
 
    } else if (strcmp(cmd, "reboot") == 0) {
         println("Rebooting the system...");
         delay_ms(1000);
         reboot();
 
    } else if (strcmp(cmd, "reset") == 0) {
        println("Resetting the screen...");
        delay_ms(1000);
        reset();

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
    } else if (strcmp(cmd, "shutdown") == 0) {
         println("Shutting down the system...");
         delay_ms(1000);
         shutdown();
 
    } else if (strcmp(cmd, "program") == 0) {
    if (arg_count == 0) {
        println("Usage: program [load|run|clear]");
    } else if (strcmp(args[0], "load") == 0) {
        command_load(NULL);
    } else if (strcmp(args[0], "run") == 0) {
        if (csa_entrypoint == NULL) {
            println("Error: No program loaded, ya fuckin goblin.");
        } else {
            println("Executing loaded program...");
            execute_csa();
        }
    } else if (strcmp(args[0], "clear") == 0) {
        csa_clear();
    } else {
        println("Invalid argument. Use 'load' or 'run', ya eejit.");
    }
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
    } else if (strcmp(cmd, "melody") == 0) {
        melody_main();
    } else if (strcmp(cmd, "poem") == 0) {
        retain_clock = 0;
        // Clear the screen
        clear_screen();
        row = 0;
        col = 0;
        curs_row = 0;
        curs_col = 0;
    
        // Display the poem
        println("life is like a door");
        println("never trust a cat");
        println("because the moon can't swim\n");
        println("but they live in your house");
        println("even though they don't like breathing in");
        println("dead oxygen that's out of warranty\n");
        println("when the gods turn to glass");
        println("you'll be drinking lager out of urns");
        println("and eating peanut butter with mud\n");
        println("bananas wear socks in the basement");
        println("because time can't tie its own shoes");
        println("and the dead spiders are unionizing\n");
        println("and a microwave is just a haunted suitcase");
        println("henceforth gravity owes me twenty bucks");
        println("because the couch is plotting against the fridge\n");
        println("when pickles dream in binary");
        println("the mountain dew solidifies");
        println("into a 2007 toyota corolla\n");
        curs_row += 21;

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
        if(arg_count == 0){
            println("Available command categories:");
            println("1 - General commands");
            println("2 - Music commands");
            println("3 - Settings commands");
            println("4 - Test commands");
            println("");
            println("To inspect one category in more detail, use \"help [number]\"");
            curs_row += 6;
            update_cursor();
        } else if (strcmp(args[0], "1") == 0){
            println("Available general commands");
            println("clear - Clear the screen (does not clear visual effects).");
            println("color <text color> <background color> - Change text and background colors.");
            println("echo <repetions> <\"text\"> - Echo the text back to the console. /n for new line.");
            println("program [load|run|clear] - Load, run, or de-load a program from Serial (COM1).");
            println("simas - Run the SIMAS interpreter.");
            println("help - Display this help message.");
            println("reboot - Reboot the system.");
            println("reset - Reset the screen to default.");
            println("shutdown - Shutdown the system.");
            curs_row += 9;
            update_cursor();
        } else if (strcmp(args[0], "2") == 0){
            println("Available music commands");
            println("beep - <frequency> <duration> - Beep at a certain frequency and duration.");
            println("melody - <frequency> <duration> | [play/delete/clear/show] - Make a melody!");
            curs_row += 2;
            update_cursor();
        } else if (strcmp(args[0], "3") == 0){
            println("Available settings commands");
            println("settime <hour> <minute> <second> - Set the RTC time.");
            println("setdate <day> <month> <year> - Set the RTC date.");
            curs_row += 2;
            update_cursor();
        } else if (strcmp(args[0], "4") == 0){
            println("Available test commands");
            println("macos - no need for ths command... this is not MacOS...");
            println("poem - display the Beacon Poem");
            println("test [argument] - Test command with optional argument.");
            curs_row += 3;
            update_cursor();
        } else {
            println("Usage: help [number]");
        }
    } else if (strcmp(cmd, "settime") == 0) {
        if (arg_count != 3) {
            println("Usage: settime <hour (24h time)> <minute> <second>");
        } else {
            int hour = atoi(args[0]);
             int minute = atoi(args[1]);
             int second = atoi(args[2]);
 
             if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
                 println("Invalid time values. Use 24-hour format, dumbass.");
             } else {
                 set_rtc_time((uint8_t)hour, (uint8_t)minute, (uint8_t)second);
                 println("RTC time updated. If it's wrong, that's on you.");
             }
         }
    } else if (strcmp(cmd, "setdate") == 0) {
        if (arg_count != 3) {
            println("Usage: setdate <day(DD)> <month(MM)> <year(YY)>");
        } else {
            int day = atoi(args[0]);
            int month = atoi(args[1]);
            int year = atoi(args[2]) % 100; // CMOS stores last two digits
    
            if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99) {
                println("Invalid date values, ya donkey.");
            } else {
                set_rtc_date((uint8_t)day, (uint8_t)month, (uint8_t)year);
                println("RTC date updated. If it's wrong, you're the moron who typed it.");
            }
        }
    } else if (strcmp(cmd, "simas") == 0) {
        simas_main();
    } else if (strcmp(cmd, "beep") == 0) {
        if (arg_count != 2) {
            println("Usage: beep <frequency in Hz> <duration in ms>");
        } else {
            int freq = atoi(args[0]);
            int duration = atoi(args[1]);

            if (freq <= 0 || duration <= 0) {
                println("Frequency and duration must be positive numbers, ya dafty.");
            } else {
                char msg[64];

                println("Beepin'...");
                beep(freq, duration);
            }
        }
    } else if (strcmp(cmd, "serial") == 0) {
        if (arg_count == 0) {
            println("Usage: serial [toggle|cmdenable|cmddisable]");
        } else if (strcmp(args[0], "cmdenable") == 0) {
            set_serial_command(1);
            println("Serial command processing enabled.");
        } else if (strcmp(args[0], "cmddisable") == 0) {
            set_serial_command(0);
            println("Serial command processing disabled.");
        } else if (strcmp(args[0], "toggle") == 0) {
            serial_toggle();
        } else {
            println("Invalid argument.");
        }
    }
    else if (strcmp(cmd, "") == 0) {
        return; // Do nothing if the command is empty
     } else {
         move_cursor_back();
         print("\"");
         print(cmd);
         println("\" is not a known command or executable program.");
     }
     curs_row++;
     if (curs_row >= NUM_ROWS) {
         scroll_screen();
         curs_row = NUM_ROWS - 1;
     }
     update_cursor();
 }
 