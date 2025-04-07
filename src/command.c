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
 #include "dungeon.h"
 #include "keyboard.h"
 #include "string.h"
 #include "speaker.h"
 #include <stdint.h>
 #include <stdarg.h>
 #include <stdbool.h>
 
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

#define MELODY_QUEUE_SIZE 128

int loopStartNote = -1;
int loopEndNote = -1;
bool loopEnabled = false;
int loopCount = 0; // 0 = infinite loops 

void reset() {
    clear_screen();
    set_color(GREEN_COLOR, WHITE_COLOR);
    repaint_screen(GREEN_COLOR, WHITE_COLOR);

    col = 0;
    row = 0;
    setup_mode = 0;
    retain_clock = 1;
    rainbow_running = 0;
    input_len = 0;

    println("Copyright (c) 2025 Turrnut Open Source Organization.");
    println("");
    println("Type a command (type 'help' for a list):");

    curs_row = 3;
    curs_col = 0;
    update_cursor();

    int loop_counter = 0;

    start();
}

typedef struct {
    uint32_t freq;
    uint32_t duration;
    uint8_t fg; // text color
    uint8_t bg; // background color   
} MelodyNote;

MelodyNote melodyQueue[MELODY_QUEUE_SIZE];
int melodyQueueSize = 0;

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

        if (arg_count == 0) {
            println("Usage: melody <freq> <duration> [fg] [bg] | play | delete | clear | show [last] | loop <count> [start] [end] | save | load | random <count> | edit <index> <freq> <duration> [fg] [bg]");
        }
        // PLAY COMMAND
        else if (strcmp(args[0], "play") == 0) {
            if (melodyQueueSize == 0) {
                println("Queue's empty, ya muppet. Nothing tae play.");
            } else {
                println("Playing melody queue...");
                
                int i = 0;
        
                // STEP 1: play up to loopStartNote (if loop is enabled and start > 0)
                if (loopEnabled && loopStartNote > 0) {
                    for (i = 0; i < loopStartNote; i++) {
                        set_color(melodyQueue[i].fg, melodyQueue[i].bg);
                        repaint_screen(melodyQueue[i].fg, melodyQueue[i].bg);
        
                        char buf[64];
                        snprintf(buf, sizeof(buf), "Beep! %d Hz, %d ms. Colors: fg %d, bg %d",
                                 melodyQueue[i].freq,
                                 melodyQueue[i].duration,
                                 melodyQueue[i].fg,
                                 melodyQueue[i].bg);
                        println(buf);
        
                        beep(melodyQueue[i].freq, melodyQueue[i].duration);
                    }
                }
        
                // STEP 2: loop between loopStartNote and loopEndNote if loopEnabled
                if (loopEnabled) {
                    int currentLoop = 0;
                    while (loopCount == 0 || currentLoop < loopCount) {
                        for (int j = loopStartNote; j <= loopEndNote; j++) {
                            set_color(melodyQueue[j].fg, melodyQueue[j].bg);
                            repaint_screen(melodyQueue[j].fg, melodyQueue[j].bg);
        
                            char buf[64];
                            snprintf(buf, sizeof(buf), "Beep! %d Hz, %d ms. Colors: fg %d, bg %d",
                                     melodyQueue[j].freq,
                                     melodyQueue[j].duration,
                                     melodyQueue[j].fg,
                                     melodyQueue[j].bg);
                            println(buf);
        
                            beep(melodyQueue[j].freq, melodyQueue[j].duration);
                        }
                        currentLoop++;
                    }
        
                    i = loopEndNote + 1; // STEP 3: continue after the loop ends
                }
        
                // STEP 4: play the rest of the melody
                for (; i < melodyQueueSize; i++) {
                    set_color(melodyQueue[i].fg, melodyQueue[i].bg);
                    repaint_screen(melodyQueue[i].fg, melodyQueue[i].bg);
        
                    char buf[64];
                    snprintf(buf, sizeof(buf), "Beep! %d Hz, %d ms. Colors: fg %d, bg %d",
                             melodyQueue[i].freq,
                             melodyQueue[i].duration,
                             melodyQueue[i].fg,
                             melodyQueue[i].bg);
                    println(buf);
        
                    beep(melodyQueue[i].freq, melodyQueue[i].duration);
                }
        
                set_color(2, 15);
                repaint_screen(2, 15);
                println("Finished playing melody. Yer ears knackered yet?");
            }
        }

        // RANDOM COMMAND
        else if (strcmp(args[0], "random") == 0) {
            if (arg_count < 2) {
                println("Usage: melody random <count>. Ye fuckin donut.");
            } else {
                int count = atoi(args[1]);
                if (count <= 0) {
                    println("Count must be greater than 0, ya muppet.");
                } else if (melodyQueueSize + count > MELODY_QUEUE_SIZE) {
                    println("Queue cannae handle that many notes, ya greedy bastard.");
                } else {
                    for (int i = 0; i < count; i++) {
                        int freq = rand(14001) + 400; // 400 to 14400
                        int duration = rand(4751) + 250; // 250 to 5000
                        int fg = rand(16); // 0 to 15
                        int bg = rand(16); // 0 to 15

                        melodyQueue[melodyQueueSize].freq = freq;
                        melodyQueue[melodyQueueSize].duration = duration;
                        melodyQueue[melodyQueueSize].fg = fg;
                        melodyQueue[melodyQueueSize].bg = bg;

                        melodyQueueSize++;
                    }

                    char buf[64];
                    snprintf(buf, sizeof(buf), "Added %d random notes tae the queue. Yer playlist's now a mess.", count);
                    println(buf);
                }
            }
        }

        // DELETE COMMAND
        else if (strcmp(args[0], "delete") == 0) {
            if (melodyQueueSize == 0) {
                println("Queue's already empty, ya tube.");
            } else {
                melodyQueueSize--;
                println("Deleted last note from the queue.");
            }
        }
    
        // CLEAR COMMAND
        else if (strcmp(args[0], "clear") == 0) {
            melodyQueueSize = 0;
            println("Cleared the entire queue. Wiped cleaner than yer browsing history.");
        }
    
        // SHOW COMMAND
        else if (strcmp(args[0], "show") == 0) {
            if (arg_count == 2 && strcmp(args[1], "last") == 0) {
                if (melodyQueueSize == 0) {
                    println("Queue's empty. No last note tae show, ya dafty.");
                } else {
                    char buf[128]; // make the buffer bigger ya lazy sod
                    snprintf(buf, sizeof(buf), "Last note: %d Hz, %d ms, fg %d, bg %d",
                             melodyQueue[melodyQueueSize - 1].freq,
                             melodyQueue[melodyQueueSize - 1].duration,
                             melodyQueue[melodyQueueSize - 1].fg,
                             melodyQueue[melodyQueueSize - 1].bg);
                    println(buf);
                }
            } else {
                if (melodyQueueSize == 0) {
                    println("Queue's empty. What the fuck are ye tryin' to look at?");
                } else {
                    println("Melody queue:");
                    for (int i = 0; i < melodyQueueSize; i++) {
                        char buf[128]; // bigger buffer, cos yer adding text ya numpty
        
                        // start with the note info
                        snprintf(buf, sizeof(buf), "%d: %d Hz, %d ms, fg %d, bg %d",
                                 i,
                                 melodyQueue[i].freq,
                                 melodyQueue[i].duration,
                                 melodyQueue[i].fg,
                                 melodyQueue[i].bg);
        
                        // add loop start/end markers if this is the one
                        if (loopEnabled) {
                            if (i == loopStartNote) {
                                strncat(buf, " [LOOP START]", sizeof(buf) - strlen(buf) - 1);
                            }
                            if (i == loopEndNote) {
                                strncat(buf, " [LOOP END]", sizeof(buf) - strlen(buf) - 1);
                            }
                        }
        
                        println(buf);
                        curs_row++;
                        update_cursor();
                    }
                }
            }
        }
        
    
        // LOOP COMMAND
        else if (strcmp(args[0], "loop") == 0) {
            if (arg_count < 2) {
                println("Usage: melody loop <count> [start note] [end note]");
            } else {
                loopCount = atoi(args[1]);
                loopStartNote = 0;
                loopEndNote = melodyQueueSize - 1;
        
                if (arg_count >= 3) loopStartNote = atoi(args[2]);
                if (arg_count >= 4) loopEndNote = atoi(args[3]);
        
                if (melodyQueueSize == 0) {
                    println("Queue's empty ya bawbag. Nothin tae loop.");
                    loopEnabled = false;
                } else if (loopStartNote < 0 || loopStartNote >= melodyQueueSize || loopEndNote < 0 || loopEndNote >= melodyQueueSize) {
                    println("Start or end note is outta range, ya donkey.");
                    loopEnabled = false;
                } else if (loopStartNote > loopEndNote) {
                    println("Start note's after end note. Did ye mash yer keyboard again?");
                    loopEnabled = false;
                } else {
                    println("Loop points set. Play will loop now. Don't say I didnae warn ye.");
                    loopEnabled = true;
                }
            }
        }
        
    
        // EDIT COMMAND
        else if (strcmp(args[0], "edit") == 0) {
            if (arg_count < 4) {
                println("Usage: melody edit <index> <freq> <duration> [fg] [bg]");
            } else {
                int index = atoi(args[1]);
                if (index < 0 || index >= melodyQueueSize) {
                    println("Index out of range, ya absolute melt.");
                    return;
                }

                int freq = atoi(args[2]);
                int duration = atoi(args[3]);

                if (freq <= 0 || duration <= 0) {
                    println("Invalid freq or duration. This ain't rocket science ya cabbage.");
                    return;
                }

                int fg = melodyQueue[index].fg; // use existing values if not provided
                int bg = melodyQueue[index].bg;

                if (arg_count >= 5) fg = atoi(args[4]); // this is fine, fg is args[4]
                if (arg_count >= 6) bg = atoi(args[5]); // bg is args[5]

                if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
                    println("Colour values must be between 0 and 15, ya speccy tube.");
                    return;
                }

                melodyQueue[index].freq = freq;
                melodyQueue[index].duration = duration;
                melodyQueue[index].fg = fg;
                melodyQueue[index].bg = bg;

                char buf[64];
                snprintf(buf, sizeof(buf), "Edited note #%d: %d Hz, %d ms, fg %d, bg %d",
                        index, freq, duration, fg, bg);
                println(buf);
            }
        }

    
        // ADD NOTE COMMAND
        else if (arg_count >= 2) {
            int freq = atoi(args[0]);
            int duration = atoi(args[1]);

            int fg = 15;
            int bg = 0;

            if (arg_count >= 3) fg = atoi(args[2]); // was >= 4, should be >= 3!
            if (arg_count >= 4) bg = atoi(args[3]); // was >= 5, should be >= 4!

            if (freq <= 0 || duration <= 0) {
                println("Invalid; discarded. Ye tryin' tae crash this thing?");
            } else if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
                println("Colour values must be between 0 and 15, ya blind tosser.");
            } else if (melodyQueueSize >= MELODY_QUEUE_SIZE) {
                println("Queue's full, ya greedy bastard.");
            } else {
                melodyQueue[melodyQueueSize].freq = freq;
                melodyQueue[melodyQueueSize].duration = duration;
                melodyQueue[melodyQueueSize].fg = fg;
                melodyQueue[melodyQueueSize].bg = bg;
                melodyQueueSize++;
                println("Added tae melody queue. It's like yer makin' yer own funeral music.");
            }
        }

    
        // INVALID COMMAND
        else {
            println("Invalid; discarded. Ye want tae try speakin' English next time?");
        }
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
            println("2 - Music / Entertainment commands");
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
            println("help - Display this help message.");
            println("reboot - Reboot the system.");
            println("reset - Reset the screen to default.");
            println("shutdown - Shutdown the system.");
            curs_row += 7;
            update_cursor();
        } else if (strcmp(args[0], "2") == 0){
            println("Available music commands");
            println("beep - <frequency> <duration> - Beep at a certain frequency and duration.");
            println("melody - <frequency> <duration> | [play/delete/clear/show] - Make a melody!");
            println("dungeon - Start the Dungeon Game.");
            curs_row += 3;
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
        if (arg_count != 3 && setup_ran == 1) {
            println("Usage: settime <hour (24h time)> <minute> <second>");
        } else if (arg_count != 3 && setup_ran == 0) {
            curs_row += 1;
            update_cursor();
            return;
        } else {
            int hour = atoi(args[0]);
             int minute = atoi(args[1]);
             int second = atoi(args[2]);
 
             if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59 && setup_ran == 1) {
                 println("Invalid time values. Use 24-hour format, dumbass.");
             } else if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59 && setup_ran == 0) {
                return;
             }  else {
                 set_rtc_time((uint8_t)hour, (uint8_t)minute, (uint8_t)second);
                 if (setup_mode == 0) {
                 println("RTC time updated. If it's wrong, that's on you.");
                 }
             }
         }
    } else if (strcmp(cmd, "dungeon") == 0) {
        println("Loading Dungeon Game... Press a key to begin.");
        getch();  // if not already declared, make sure getch() is in keyboard.h
        clear_screen();
        srand(rand(99999999));  // seed it good and dirty
        start_dungeon();  // launch the game
    } else if (strcmp(cmd, "setdate") == 0) {
        if (arg_count != 3 && setup_ran == 1) {
            println("Usage: setdate <day(DD)> <month(MM)> <year(YY)>");
        } else if (arg_count != 3 && setup_ran == 0) {
            curs_row += 1;
            update_cursor();
            return;
        } else {
            int day = atoi(args[0]);
            int month = atoi(args[1]);
            int year = atoi(args[2]) % 100; // CMOS stores last two digits
    
            if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99 && setup_ran == 1) {
                println("Invalid date values, ya donkey.");
            } if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99 && setup_ran == 0) {
                return;
            } else {
                set_rtc_date((uint8_t)day, (uint8_t)month, (uint8_t)year);
                if (setup_mode == 0) {
                println("RTC date updated. If it's wrong, you're the moron who typed it.");
                }
            }
        }
    }     else if (strcmp(cmd, "beep") == 0) {
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
 