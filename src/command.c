/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * command.c
 */

#include "bf.h"
#include "command.h"
#include "console.h"
#include "csa.h"
#include "ctype.h"
#include "disks.h"
#include "ff.h"
#include "keyboard.h"
#include "math.h"
#include "os.h"
#include "screen.h"
#include "serial.h"
#include "speaker.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

// external vars from os.c or other modules
extern size_t curs_row;
extern size_t curs_col;
extern void* g_mb_info;
extern int simas_main(void);
extern void main_menu_loop(void);
extern void cube_main(void);
extern void paint_main(void);
int macos = 0; // macos mode

void reset() {
    start();
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
        return 0; // No arguments
    }

    // Extract arguments
    while ((next_space = strchr(token, ' ')) != NULL && arg_count < MAX_ARGS) {
        strncpy(args[arg_count], token, next_space - token);
        args[arg_count][next_space - token] = '\0';
        token = next_space + 1;
        arg_count++;
    }

    if (*token != '\0' && arg_count < MAX_ARGS) {
        strcpy(args[arg_count], token);
        arg_count++;
    }

    return arg_count;
}

void shutdown() {
    clear_screen();
    disable_cursor();
    retain_clock = 0;
    display_prompt = 0;
    gotoxy(0, 0);
    set_color(15, 0);
    repaint_screen(15,0);
    print("It is now safe to power off your computer.");
    asm volatile ("hlt"); 
}

/**
 * Processes the entered command.
 */
void process_command(const char* command) {
    char cmd[INPUT_BUFFER_SIZE] = {0};
    char args[MAX_ARGS][INPUT_BUFFER_SIZE] = {{0}};
    int arg_count = parse_command(command, cmd, args);

    if (stricmp(cmd, "test") == 0) {
        if (arg_count > 0) {
            if (stricmp(args[0], "argument") == 0) {
                println("You passed the magic argument 'argument'. Congrats, I guess.");
            } else {
                println("Unrecognized argument. Try harder.");
            }
        } else {
            println("This is a test command, but you didn't even give me any args. Nice one.");
        }

    } else if (stricmp(cmd, "echo") == 0) {
        if (arg_count < 2) {
            println("Usage: echo [repeat_count] \"text\"");
        } else {
            long long repeat_count = atoll(args[0]);
            if (repeat_count <= 0) {
                println("Repeat count must be a positive number.");
            } else {
                for (int i = 0; i < repeat_count; i++) {
                    for (int j = 1; j < arg_count; j++) {
                        print(args[j]);
                        print(" ");
                    }
                    println("");
                    curs_row++;
                    curs_col = 0;
                }
                println("");
            }
        }

    } else if (stricmp(cmd, "clear") == 0) {
        clear_screen();
        retain_clock = 0;
        gotoxy(0, 0);
        println("");
        println("");

    } else if (stricmp(cmd, "reboot") == 0) {
        println("Rebooting the system...");
        delay_ms(1000);
        reboot();

    } else if (stricmp(cmd, "reset") == 0) {
        println("Resetting the screen...");
        delay_ms(1000);
        reset();

    } else if (stricmp(cmd, "shutdown") == 0) {
        println("Shutting down the system...");
        delay_ms(1000);
        shutdown();

    } else if (stricmp(cmd, "program") == 0) {
        if (arg_count == 0) {
            println("Usage: program [load/run]");
        } else if (stricmp(args[0], "load") == 0) {
            command_load(NULL);
        } else if (stricmp(args[0], "run") == 0) {
            if (csa_entrypoint == NULL) {
                println("Error: No program loaded, ya fuckin goblin.");
            } else {
                println("Executing loaded program...");
                execute_csa();
            }
        } else {
            println("Invalid argument. Use 'load' or 'run', ya eejit.");
        }
    } else if (stricmp(cmd, "color") == 0) {
        if (arg_count != 2) {
            println("Usage: color <text color> <background color>");
        } else {
            int fg = atoi(args[0]);
            int bg = atoi(args[1]);
            if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
                println("Color values must be between 0 and 15.");
            } else {
                uint8_t color = (bg << 4) | fg;
                set_color(fg, bg);
                repaint_screen(fg, bg);
                println("Colors updated. Try not to blind yourself.");
            }
        }

    } else if (stricmp(cmd, "melody") == 0) {
        melody_main();

    } else if (stricmp(cmd, "poem") == 0) {
        retain_clock = 0;
        clear_screen();
        gotoxy(0, 0);
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

    } else if (stricmp(cmd, "macos") == 0) {
        if (macos == 1) {
            set_color(GREEN_COLOR, WHITE_COLOR);
            repaint_screen(GREEN_COLOR, WHITE_COLOR);
            println("Welcome to Beacon OS. This is not macOS.");
            macos = 0;
        } else {
            set_color(WHITE_COLOR, LIGHT_BLUE_COLOR);
            repaint_screen(WHITE_COLOR, LIGHT_BLUE_COLOR);
            println("No, this is not macOS. This is Beacon OS. But I'll humor you.");
            macos = 1;
        }

    } else if (stricmp(cmd, "help") == 0) {
        if (arg_count == 0) {
            println("Available command categories:");
            println("1 - General commands");
            println("2 - Fun commands");
            println("3 - System commands");
            println("4 - File commands");
            println("");
            println("To inspect one category in more detail, use \"help [number]\"");
            curs_row += 6;
            update_cursor();
        } else if (stricmp(args[0], "1") == 0) {
            println("Available general commands:");
            println("Clear - Clear the screen (does not clear visual effects).");
            println("Color <text color> <background color> - Change text and background colors.");
            println("Echo <repetitions> <\"text\"> - Echo the text back to the console.");
            println("Program [load|run] - Load or run a program from serial (COM1).");
            println("SIMAS - Run the SIMAS interpreter.");
            println("Help - Display this help message.");
            curs_row += 6;
            update_cursor();
        } else if (stricmp(args[0], "2") == 0) {
            println("Available fun commands:");
            println("Beep <freq> <dur> - Beep at frequency (Hz) and duration (ms).");
            println("Bf <code> - The programming language of Brainfu-, I mean, boyfriend.");
            println("Melody - Create or play a melody.");
            println("Games - Launch the game menu.");
            println("Poem - Display the Beacon poem.");
            println("Cube - Render a basic 3D cube in the console.");
            println("Paint - Open Turrpaint.");
            println("macOS - This is not macOS. This command is pointless.");
            curs_row += 8;
            update_cursor();
        } else if (stricmp(args[0], "3") == 0) {
            println("Available system commands:");
            println("Settime <hour> <min> <sec> - Set the RTC time.");
            println("Setdate <day> <month> <year> - Set the RTC date.");
            println("Reboot - Reboot the system.");
            println("Reset - Reset the screen to default.");
            println("Shutdown - Shutdown the system.");
            println("Drives - List mounted drives.");
            println("CD <dir> - Change directory (supports drive letters).");
            println("DU [drive] - Disk usage for current or specified drive.");
            curs_row += 8;
            update_cursor();
        } else if (stricmp(args[0], "4") == 0) {
            println("Available file commands:");
            println("Ls [directory] - List files in cwd or specified dir.");
            println("Lsr [directory] - List files recursively.");
            println("Mkdir <directory> - Create a new directory.");
            println("Exists <path> - Check if file or directory exists.");
            println("Filesize <file> - Show size of a file.");
            println("Copy <src> <dst> - Copy a file.");
            println("Append <file> <text> - Append text to a file.");
            println("New <filename> - Create a new file.");
            println("Open <filename> - Open a file for reading.");
            println("Del <filename> - Delete a file.");
            println("Rename <oldname> <newname> - Rename a file.");
            println("File [send|receive] <filename> - Send/receive over serial.");
            println("Format [drive] - Format a drive (warning: destroys data).");
            println("Du [drive] - Disk usage (alias for 'Du').");
            curs_row += 11;
            update_cursor();
        } else {
            println("Usage: help [1-4]");
        }

    } else if (stricmp(cmd, "settime") == 0) {
        if (arg_count != 3) {
            println("Usage: settime <hour(0-23)> <min(0-59)> <sec(0-59)>");
        } else {
            int hour   = atoi(args[0]);
            int minute = atoi(args[1]);
            int second = atoi(args[2]);
            if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
                println("Invalid time values.");
            } else {
                set_rtc_time((uint8_t)hour, (uint8_t)minute, (uint8_t)second);
                println("RTC time updated.");
            }
        }

    } else if (stricmp(cmd, "setdate") == 0) {
        if (arg_count != 3) {
            println("Usage: setdate <day(1-31)> <month(1-12)> <year(0-99)>");
        } else {
            int day   = atoi(args[0]);
            int month = atoi(args[1]);
            int year  = atoi(args[2]) % 100;
            if (day < 1 || day > 31 || month < 1 || month > 12 || year < 0 || year > 99) {
                println("Invalid date values.");
            } else {
                set_rtc_date((uint8_t)day, (uint8_t)month, (uint8_t)year);
                println("RTC date updated.");
            }
        }

    } else if (stricmp(cmd, "cube") == 0) {
        cube_main();

    } else if (stricmp(cmd, "paint") == 0) {
        paint_main();

    } else if (stricmp(cmd, "simas") == 0) {
        simas_main();

    } else if (stricmp(cmd, "beep") == 0) {
        if (arg_count != 2) {
            println("Usage: beep <freq> <dur>");
        } else {
            int freq     = atoi(args[0]);
            int duration = atoi(args[1]);
            if (freq <= 0 || duration <= 0) {
                println("Frequency and duration must be positive numbers.");
            } else {
                println("Beepin'...");
                beep(freq, duration);
            }
        }

    } else if (stricmp(cmd, ":(){ :|:& };:") == 0 || stricmp(cmd, ":(){:|:&};:") == 0) {
        while(1) {
            print("fork");
        }
    } 
    else if (stricmp(cmd, "bf") == 0) {
        char combined_args[INPUT_BUFFER_SIZE * MAX_ARGS] = {0};
        for (int i = 0; i < arg_count; i++) {
            strcat(combined_args, args[i]);
            if (i < arg_count - 1) {
                strcat(combined_args, " ");
            }
        }
        bf(combined_args);
    } else if (stricmp(cmd, "games") == 0) {
        main_menu_loop();

    } else if (stricmp(cmd, "ls") == 0) {
        if (arg_count == 0) {
            list_directory_with_paging(NULL);
        } else if (arg_count == 1) {
            list_directory_with_paging(args[0]);
        } else {
            println("Usage: ls [directory]");
        }

    } else if (stricmp(cmd, "lsr") == 0) {
        if (arg_count > 1) {
            println("Usage: lsr [directory]");
        } else {
            if (arg_count == 0) {
                list_directory_recursive(NULL);
            } else {
                list_directory_recursive(args[0]);
            }
        }

    } else if (stricmp(cmd, "mkdir") == 0) {
        if (arg_count != 1) {
            println("Usage: mkdir <directory>");
        } else {
            FRESULT r = make_directory(args[0]);
            if (r == FR_OK) {
                println("Directory created successfully.");
            } else {
                println("Failed to create directory.");
            }
        }

    } else if (stricmp(cmd, "exists") == 0) {
        if (arg_count != 1) {
            println("Usage: exists <path>");
        } else {
            if (directory_exists(args[0])) {
                println("Directory exists.");
            } else if (file_exists(args[0])) {
                println("File exists.");
            } else {
                println("Path does not exist.");
            }
        }

    } else if (stricmp(cmd, "filesize") == 0) {
        if (arg_count != 1) {
            println("Usage: filesize <file>");
        } else {
            DWORD sz = get_file_size(args[0]);
            if (sz == 0) {
                println("Failed to get file size or file is empty.");
            } else {
                print("File size: ");
                printf("%lu", sz);
                println(" bytes.");
            }
        }

    } else if (stricmp(cmd, "copy") == 0) {
        if (arg_count != 2) {
            println("Usage: copy <source> <destination>");
        } else {
            FRESULT r = copy_file(args[0], args[1]);
            if (r == FR_OK) {
                println("File copied successfully.");
            } else {
                println("Failed to copy file.");
            }
        }

    } else if (stricmp(cmd, "append") == 0) {
        if (arg_count < 2) {
            println("Usage: append <file> <text>");
        } else {
            char combined[MAX_PATH_LEN];
            combined[0] = '\0';
            for (int i = 1; i < arg_count; i++) {
                strcat(combined, args[i]);
                if (i < arg_count - 1) {
                    strcat(combined, " ");
                }
            }
            FRESULT r = append_to_file(args[0], combined);
            if (r == FR_OK) {
                println("Data appended successfully.");
            } else {
                println("Failed to append to file.");
            }
        }

    } else if (stricmp(cmd, "format") == 0) {
        // usage: format [drive]
        if (arg_count > 1) {
            println("Usage: format [drive]");
        } else {
            char drive_spec[3];
            if (arg_count == 0) {
                // default to current drive (e.g. get_current_directory returns "2:/foo")
                const char *curr = get_current_directory(); // like "2:/some/path/"
                drive_spec[0] = curr[0];
                drive_spec[1] = ':';
                drive_spec[2] = '\0';
            } else {
                // user provided something; must be "X:" or "X:/"
                if (args[0][0] >= '0' && args[0][0] <= '0'+(MAX_LOGICAL_DRIVES-1) && args[0][1]==':') {
                    drive_spec[0] = args[0][0];
                    drive_spec[1] = ':';
                    drive_spec[2] = '\0';
                } else {
                    println("Invalid drive spec. Use e.g. '0:' through '3:'.");
                    goto skip_format_confirmation;
                }
            }
            print("Are you sure you want to format ");
            print(drive_spec);
            print("!!! DELETES ALL DATA !!! (y/n): \n");
            curs_row++;
            curs_row++;
            char confirm = getch();
            if (confirm == 'y' || confirm == 'Y') {
                print("Formatting drive ");
                println(drive_spec);
                format_disk(drive_spec);
            } else {
                println("Disk format aborted.");
            }
        }
      skip_format_confirmation: ;

    } else if (stricmp(cmd, "new") == 0) {
        if (arg_count != 1) {
            println("Usage: new <filename>");
        } else {
            FRESULT r = file_create(args[0]);
            if (r == FR_OK) {
                println("File created successfully.");
            } else {
                println("Failed to create file.");
            }
        }

    } else if (stricmp(cmd, "open") == 0) {
        if (arg_count != 1) {
            println("Usage: open <filename>");
        } else {
            if (file_exists(args[0])) {
                print("Opening file: ");
                println(args[0]);
                // Read and print in chunks
                char buf[128];
                UINT br;
                FRESULT r;
                char full[MAX_PATH_LEN];
                get_full_path(args[0], full);
                FIL fil;
                r = f_open(&fil, full, FA_READ);
                if (r != FR_OK) {
                    println("Failed to open file.");
                } else {
                    while (1) {
                        r = f_read(&fil, buf, sizeof(buf) - 1, &br);
                        if (r != FR_OK || br == 0) break;
                        buf[br] = '\0';
                        println(buf);
                    }
                    f_close(&fil);
                }
            } else {
                println("Error: file not found.");
            }
        }

    } else if (stricmp(cmd, "del") == 0) {
        if (arg_count != 1) {
            println("Usage: del <filename>");
        } else {
            if (file_exists(args[0])) {
                println("Deleting file...");
                FRESULT r = f_unlink(args[0]);
                if (r == FR_OK) {
                    println("File deleted successfully.");
                } else {
                    println("Failed to delete file.");
                }
            } else {
                println("Error: file not found.");
            }
        }

    } else if (stricmp(cmd, "rename") == 0) {
        if (arg_count != 2) {
            println("Usage: rename <oldname> <newname>");
        } else {
            FRESULT r = f_rename(args[0], args[1]);
            if (r == FR_OK) {
                println("File renamed successfully.");
            } else {
                println("Failed to rename file.");
            }
        }

    } else if (stricmp(cmd, "file") == 0) {
        if (arg_count == 0) {
            println("Usage: file [send|receive] <filename>");
        } else if (stricmp(args[0], "send") == 0) {
            if (arg_count != 2) {
                println("Usage: file send <filename>");
            } else {
                serial_send_file(args[1]);
            }
        } else if (stricmp(args[0], "receive") == 0) {
            if (arg_count != 2) {
                println("Usage: file receive <filename>");
            } else {
                serial_receive_file(args[1]);
            }
        } else {
            println("Invalid argument for 'file'. Use 'send' or 'receive'.");
        }

    } else if (stricmp(cmd, "cd") == 0) {
        if (arg_count != 1) {
            println("Usage: cd <directory|drive:>");
        } else {
            FRESULT r = change_directory(args[0]);
            if (r == FR_OK) {
                print("CWD is now ");
                println(get_current_directory());
            } else {
                print("Failed to cd to ");
                println(args[0]);
            }
        }
    } else if (stricmp(cmd, "drives") == 0) {
        // List volumes that successfully mounted
        for (int i = 0; i < MAX_LOGICAL_DRIVES; i++) {
            char drive_spec[3] = { '0' + i, ':', '\0' };
            FATFS* fs;
            DWORD fre_clust;
            FRESULT r = f_getfree(drive_spec, &fre_clust, &fs);
            if (r == FR_OK) {
                print("Drive ");
                print(drive_spec);
                println(" - mounted");
            } else {
                print("Drive ");
                print(drive_spec);
                println(" - not mounted or no media");
            }
        }

    } else if (stricmp(cmd, "du") == 0) {
        if (arg_count > 1) {
            println("Usage: du [drive]");
        } else {
            char drive_spec[3];
            if (arg_count == 0) {
                // default to current drive
                const char* curr = get_current_directory(); // e.g. "2:/foo/"
                drive_spec[0] = curr[0];
                drive_spec[1] = ':';
                drive_spec[2] = '\0';
            } else {
                if (args[0][0] >= '0' && args[0][0] <= '0' + (MAX_LOGICAL_DRIVES - 1) && args[0][1] == ':') {
                    drive_spec[0] = args[0][0];
                    drive_spec[1] = ':';
                    drive_spec[2] = '\0';
                } else {
                    println("Invalid drive spec. Use e.g. '0:' through '3:'.");
                    goto skip_du;
                }
            }
            print("Disk usage for ");
            println(drive_spec);
            // Temporarily switch cwd to that drive so f_getfree works
            char saved_cwd[16];
            strncpy(saved_cwd, get_current_directory(), sizeof(saved_cwd));
            change_directory(drive_spec);
            check_disk_usage();
            change_directory(saved_cwd);
        }
      skip_du: ;

    } else if (stricmp(cmd, "") == 0) {
        // No-op for empty command

    } /*else {
        char filename[64];
        snprintf(filename, sizeof(filename), "%s.csa", cmd);

        FIL fil;
        FRESULT res = f_open(&fil, filename, FA_READ);
        if (res == FR_OK) {
            f_close(&fil);

            // try load it as CSA
            print("found ");
            print(filename);
            println(", loading...");

            command_load(filename);  // load into csa buffer
            println("executing...");
            execute_csa();
        }*/ else {
            curs_col = 0;
            print("\"");
            print(cmd);
            println("\" is not a known command.");
        }
    //}

    if (stricmp(cmd, "") != 0) {
        curs_row++;
    }
    if (curs_row >= NUM_ROWS) {
        scroll_screen();
        curs_row = NUM_ROWS - 1;
    }
}
