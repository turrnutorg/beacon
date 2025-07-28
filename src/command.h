/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * command.h
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

// Constants
#define INPUT_BUFFER_SIZE 256
#define MAX_ARGS 10

extern int command_line;

// Function Declarations

void shutdown();

int parse_command(const char* command, char* cmd, char args[MAX_ARGS][INPUT_BUFFER_SIZE]);
void process_command(const char* command);
void command_shell_loop(void);

void reset();

#endif // COMMAND_H
