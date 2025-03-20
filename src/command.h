/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * command.h
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "stdtypes.h"

// Constants
#define INPUT_BUFFER_SIZE 256
#define COMMAND_HISTORY_SIZE 5
#define MAX_ARGS 10

// Function Declarations

void shutdown();

/**
 * Parses the input command into the base command and its arguments.
 * 
 * @param command The input command string.
 * @param cmd The buffer to store the base command.
 * @param args The 2D array to store the arguments.
 * @return The number of arguments parsed.
 */
int parse_command(const char* command, char* cmd, char args[MAX_ARGS][INPUT_BUFFER_SIZE]);

/**
 * Processes the entered command.
 * 
 * @param command The command string to process.
 */
void process_command(const char* command);

void set_rtc_date(uint8_t day, uint8_t month, uint8_t year);
void set_rtc_time(uint8_t hour, uint8_t minute, uint8_t second);
void repaint_screen(uint8_t fg_color, uint8_t bg_color);

#endif // COMMAND_H
