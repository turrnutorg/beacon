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

// External variables from os.c or other modules
extern size_t curs_row;
extern size_t curs_col;

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

static void newRowAfterCommand(){
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
        // Save command to history
        strncpy(command_history[command_history_index], command, INPUT_BUFFER_SIZE - 1);
        command_history[command_history_index][INPUT_BUFFER_SIZE - 1] = '\0'; // Ensure null-termination
        command_history_index = (command_history_index + 1) % COMMAND_HISTORY_SIZE;
        current_history_index = -1; // Reset history navigation
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
    }
    else if (strcmp(cmd, "reboot") == 0) {
        println("Rebooting the system...");
        delay_ms(1000); // Add a short delay for the message to be seen
        reboot();
    } else if (strcmp(cmd, "shutdown") == 0) {
        println("Shutting down the system...");
        delay_ms(1000); // Add a short delay for the message to be seen
        shutdown();
    }
    else if (strcmp(cmd, "") == 0) {
        move_cursor_back();
        update_cursor();
    }
    else {
        move_cursor_back();
        print("\"");
        print(cmd);
        println("\" is not a known command or executable program.");

    }
    newRowAfterCommand();
}
