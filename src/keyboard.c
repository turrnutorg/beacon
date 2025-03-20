/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * keyboard.c
 */

#include "keyboard.h"
#include "os.h"
#include "screen.h"
#include "console.h"
#include "string.h"
#include "command.h"

#define UP_ARROW    -1
#define DOWN_ARROW  -2
#define LEFT_ARROW  -3
#define RIGHT_ARROW -4

char input_buffer[INPUT_BUFFER_SIZE];
size_t input_len = 0;
int shift = -1;

volatile uint8_t key_buffer[KEY_BUFFER_SIZE];
volatile size_t buffer_head = 0;
volatile size_t buffer_tail = 0;
volatile uint8_t key_states[128] = {0}; // tracks if a key is currently pressed

// Command history
#define COMMAND_HISTORY_SIZE 5
extern char command_history[COMMAND_HISTORY_SIZE][INPUT_BUFFER_SIZE];
extern int command_history_index;
extern int current_history_index;

// Import replacement functions
extern char* strncpy(char* dest, const char* src, size_t n);
extern size_t strlen(const char* str);
extern int strcmp(const char* s1, const char* s2);
extern char* strchr(const char* str, char c);

// Add a scancode to the buffer
void buffer_add(uint8_t scancode) {
    size_t next_head = (buffer_head + 1) % KEY_BUFFER_SIZE;
    if (next_head != buffer_tail) { // Don't overwrite
        key_buffer[buffer_head] = scancode;
        buffer_head = next_head;
    }
}

// Get a scancode from the buffer
int buffer_get(uint8_t* scancode) {
    if (buffer_head == buffer_tail) {
        return 0; // Buffer is empty
    }
    *scancode = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEY_BUFFER_SIZE;
    return 1;
}

char scancode_to_ascii(uint8_t scancode) {
    static const uint8_t scancode_map[128] = {
        0,  '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
        // Rest is zeros for now
    };

    if (scancode == 0x48) return UP_ARROW;     // Up arrow
    if (scancode == 0x50) return DOWN_ARROW;   // Down arrow
    if (scancode == 0x4B) return LEFT_ARROW;   // Left arrow
    if (scancode == 0x4D) return RIGHT_ARROW;  // Right arrow
    if (scancode == 0x2A) {
        shift = -shift;
    }

    if (scancode < 128) {
        return scancode_map[scancode];
    }
    return 0; // Invalid scancode
}

char capitalize_if_shift(char scncode) {
    // Convert to uppercase if Shift is active
    if (shift == 1) {
    // Check if the scancode represents a lowercase letter
        if (scncode >= 'a' && scncode <= 'z') {
            return scncode - ('a' - 'A');
            
        } else {
            // other
            switch (scncode) {
                case ';': return ':';
                case '\'': return '\"';
                case '/': return '?';
                case '\\': return '|';
                case ',': return '<';
                case '.': return '>';
                case '[': return '{';
                case ']': return '}';
                case '=': return '+';
                case '-': return '_';
                case '`': return '~';
                case '1': return '!';
                case '2': return '@';
                case '3': return '#';
                case '4': return '$';
                case '5': return '%';
                case '6': return '^';
                case '7': return '&';
                case '8': return '*';
                case '9': return '(';
                case '0': return ')';
            }
        }
    }
    // If not a lowercase letter or Shift is -1, return the original scancode
    return scncode;
}


void handle_keypress(uint8_t scancode) {
    if (scancode & 0x80) { // key release
        key_states[scancode & 0x7F] = 0; // mark as released
        return;
    }

    if (key_states[scancode]) {
        return; // ignore if key is already pressed
    }
    key_states[scancode] = 1; // mark as pressed

    char ascii = capitalize_if_shift(scancode_to_ascii(scancode));
    if (!ascii) return; // ignore invalid scancodes

    if (ascii == '\b') { // Handle backspace
        if (setup_mode == 0) {
        if (input_len > 0) {
            input_len--;

            // Handle cursor position (across rows or within the same row)
            if (curs_col == 0 && curs_row > 0) {
                // Move to the end of the previous row
                curs_row--;
                curs_col = NUM_COLS - 1;
            } else {
                // Move cursor back within the same row
                curs_col--;
            }
            // Shift the input buffer left to fill the gap
            for (size_t i = curs_col; i < input_len; i++) {
                input_buffer[i] = input_buffer[i + 1];
            }
            input_buffer[input_len] = '\0'; // Null-terminate the buffer
            // Update the screen buffer
            size_t screen_index = curs_row * NUM_COLS + curs_col;
            for (size_t i = screen_index; i < (NUM_ROWS * NUM_COLS) - 1; i++) {
                vga_buffer[i] = vga_buffer[i + 1]; // Shift characters left on screen
            }
            // Clear the last character in the last row
            vga_buffer[NUM_ROWS * NUM_COLS - 1] = (struct Char){' ', default_color};

            // Update cursor position visually
            update_cursor();
        }
        }
    } else if (ascii == UP_ARROW) {
        col = 0;
        if (current_history_index < 0) {
            current_history_index = command_history_index - 1; // Start from the last command
        } else if (current_history_index > 0) {
            current_history_index--; // Go to the previous command
        }

        if (current_history_index >= 0) {
            // Load the command from history
            strncpy(input_buffer, command_history[current_history_index], INPUT_BUFFER_SIZE);
            input_len = strlen(input_buffer);
        } else {
            input_buffer[0] = '\0'; // No command to load
            input_len = 0;
        }

        // Clear the current line by overwriting it
        for (size_t i = 0; i < NUM_COLS; i++) {
            vga_buffer[curs_row * NUM_COLS + i] = (struct Char){' ', default_color};
        }

        // Print the new command and move the cursor to the end
        move_cursor_back();
        print(input_buffer);
        curs_col = input_len; // Set cursor to the end of the command
        update_cursor();
    } else if (ascii == DOWN_ARROW) {
        col = 0;
        if (current_history_index < command_history_index - 1) {
            current_history_index++; // Go to the next command
            strncpy(input_buffer, command_history[current_history_index], INPUT_BUFFER_SIZE);
            input_len = strlen(input_buffer);
        } else {
            current_history_index = -1; // Clear input if at the latest
            input_buffer[0] = '\0';
            input_len = 0;
        }

        // Clear the current line by overwriting it
        for (size_t i = 0; i < NUM_COLS; i++) {
            vga_buffer[curs_row * NUM_COLS + i] = (struct Char){' ', default_color};
        }

        // Print the new command and move the cursor to the end
        move_cursor_back();
        print(input_buffer);
        curs_col = input_len; // Set cursor to the end of the command
        update_cursor();
    } else if (ascii == LEFT_ARROW) {
        if (curs_col > 0) {
            curs_col--;
            update_cursor();
        }
    } else if (ascii == RIGHT_ARROW) {
        if (curs_col < input_len) {
            curs_col++;
            update_cursor();
        }
    } else if (ascii == '\n') { // Handle newline
        input_buffer[input_len] = '\0';

        // Save the command to history
        if (input_len > 0) {
            strncpy(command_history[command_history_index], input_buffer, INPUT_BUFFER_SIZE);
            command_history_index = (command_history_index + 1) % COMMAND_HISTORY_SIZE;
        }

        println(""); // Move to a new line
        process_command(input_buffer);
        input_len = 0;
        curs_row++;
        curs_col = 0;
        if (curs_row >= NUM_ROWS) {
            scroll_screen();
            curs_row = NUM_ROWS - 1;
        }
        update_cursor();
    } else { // Handle regular characters
        if (input_len < INPUT_BUFFER_SIZE - 1) {
            input_buffer[input_len++] = ascii;
        }
        vga_buffer[curs_row * NUM_COLS + curs_col] = (struct Char){ascii, default_color};
        curs_col++;
        if (curs_col >= NUM_COLS) {
            curs_col = 0;
            curs_row++;
        }
        if (curs_row >= NUM_ROWS) {
            scroll_screen();
            curs_row = NUM_ROWS - 1;
        }
        update_cursor();
    }
}
