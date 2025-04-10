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
 #include <stdint.h>
 
 char input_buffer[INPUT_BUFFER_SIZE];
 size_t input_len = 0;
 int shift = 0;  // fixed this garbage

 static int shift_count = 0;
 
 // these exist in keyboard.asm now, not here
 extern volatile uint8_t key_buffer[16];    // must match asm .space 16
 extern volatile size_t buffer_head;
 extern volatile size_t buffer_tail;
 
 volatile uint8_t key_states[128] = {0}; // tracks if a key is currently pressed
 
 #define COMMAND_HISTORY_SIZE 5
 extern char command_history[COMMAND_HISTORY_SIZE][INPUT_BUFFER_SIZE];
 extern int command_history_index;
 extern int current_history_index;
 
 // ----------------------------------------------------------------
 // Add a scancode to the buffer (optional, since asm does it)
 // ----------------------------------------------------------------
 void buffer_add(uint8_t scancode) {
     size_t next_head = (buffer_head + 1) % 16;
     if (next_head != buffer_tail) {
         key_buffer[buffer_head] = scancode;
         buffer_head = next_head;
     }
 }
 
 // ----------------------------------------------------------------
 // Get a scancode from the buffer
 // ----------------------------------------------------------------
 int buffer_get(uint8_t* scancode) {
     if (buffer_head == buffer_tail) {
         return 0; // Buffer is empty
     }
     *scancode = key_buffer[buffer_tail];
     buffer_tail = (buffer_tail + 1) % 16;
     return 1;
 }
 
 // ----------------------------------------------------------------
 // Convert scancode to ascii or control code
 // ----------------------------------------------------------------
 char scancode_to_ascii(uint8_t scancode) {
     static const uint8_t scancode_map[128] = {
         0,  '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
         0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
         'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
         'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' ', 0,
         // the rest is zero, you're lazy and so am i
     };
 
     // handle special keys
     if (scancode == 0x48) return UP_ARROW;
     if (scancode == 0x50) return DOWN_ARROW;
     if (scancode == 0x4B) return LEFT_ARROW;
     if (scancode == 0x4D) return RIGHT_ARROW;

    // shift key pressed
    if (scancode == 0x2A || scancode == 0x36) {
        if (!key_states[scancode]) {  // only count if not already pressed
            key_states[scancode] = 1;
            shift_count++;
            shift = 1;
        }
        return 0;
    }


    // shift key released
    if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
        key_states[scancode & 0x7F] = 0; // clear state
        if (shift_count > 0)
            shift_count--;
        if (shift_count == 0)
            shift = 0;
        return 0;
    }


 
     if (scancode < 128) {
         return scancode_map[scancode];
     }
 
     return 0;
 }
 
 // ----------------------------------------------------------------
 // Shift-modified ascii (shift = 1)
 // ----------------------------------------------------------------
 char capitalize_if_shift(char scancode) {
     if (shift == 1) {
         if (scancode >= 'a' && scancode <= 'z') {
             return scancode - ('a' - 'A');
         } else {
             switch (scancode) {
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
 
     return scancode;
 }
 
 // ----------------------------------------------------------------
 // Handles an individual key press event
 // ----------------------------------------------------------------
 void handle_keypress(uint8_t scancode) {
    // process shift key events even if they're release events
    if (scancode == 0x2A || scancode == 0x36 ||
        scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
        scancode_to_ascii(scancode);
        return;
    }

    // for all other keys, if it's a release, clear state and return
    if (scancode & 0x80) { 
        key_states[scancode & 0x7F] = 0;
        return;
    }

    // if key already pressed, ignore repeat presses
    if (key_states[scancode]) {
        return; // already handled this keypress
    }
    key_states[scancode] = 1;
    
    // process key press
    char ascii = capitalize_if_shift(scancode_to_ascii(scancode));
    if (!ascii) return; // not a printable character
 
     // BACKSPACE HANDLING
     if (ascii == '\b') {
             if (input_len > 0) {
                 input_len--;
 
                 if (curs_col == 0 && curs_row > 0) {
                     curs_row--;
                     curs_col = NUM_COLS - 1;
                 } else {
                     curs_col--;
                 }
 
                 for (size_t i = curs_col; i < input_len; i++) {
                     input_buffer[i] = input_buffer[i + 1];
                 }
 
                 input_buffer[input_len] = '\0';
 
                 size_t screen_index = curs_row * NUM_COLS + curs_col;
                 for (size_t i = screen_index; i < (NUM_ROWS * NUM_COLS) - 1; i++) {
                     vga_buffer[i] = vga_buffer[i + 1];
                 }
 
                 vga_buffer[NUM_ROWS * NUM_COLS - 1] = (struct Char){' ', default_color};
 
                 update_cursor();
             }
     }
 
     // ARROW KEYS
     else if (ascii == UP_ARROW) {
         col = 0;
         if (current_history_index < 0) {
             current_history_index = command_history_index - 1;
         } else if (current_history_index > 0) {
             current_history_index--;
         }
 
         if (current_history_index >= 0) {
             strncpy(input_buffer, command_history[current_history_index], INPUT_BUFFER_SIZE);
             input_len = strlen(input_buffer);
         } else {
             input_buffer[0] = '\0';
             input_len = 0;
         }
 
         for (size_t i = 0; i < NUM_COLS; i++) {
             vga_buffer[curs_row * NUM_COLS + i] = (struct Char){' ', default_color};
         }
 
         move_cursor_back();
         print(input_buffer);
         curs_col = input_len;
         update_cursor();
     }
 
     else if (ascii == DOWN_ARROW) {
         col = 0;
         if (current_history_index < command_history_index - 1) {
             current_history_index++;
             strncpy(input_buffer, command_history[current_history_index], INPUT_BUFFER_SIZE);
             input_len = strlen(input_buffer);
         } else {
             current_history_index = -1;
             input_buffer[0] = '\0';
             input_len = 0;
         }
 
         for (size_t i = 0; i < NUM_COLS; i++) {
             vga_buffer[curs_row * NUM_COLS + i] = (struct Char){' ', default_color};
         }
 
         move_cursor_back();
         print(input_buffer);
         curs_col = input_len;
         update_cursor();
     }
 
     else if (ascii == LEFT_ARROW) {
         if (curs_col > 0) {
             curs_col--;
             update_cursor();
         }
     }
 
     else if (ascii == RIGHT_ARROW) {
         if (curs_col < input_len) {
             curs_col++;
             update_cursor();
         }
     }
 
     // NEWLINE (ENTER)
     else if (ascii == '\n') {
         input_buffer[input_len] = '\0';
 
         if (input_len > 0) {
             strncpy(command_history[command_history_index], input_buffer, INPUT_BUFFER_SIZE);
             command_history_index = (command_history_index + 1) % COMMAND_HISTORY_SIZE;
         }
 
         println("");
         process_command(input_buffer);
 
         input_len = 0;
         curs_row++;
         curs_col = 0;
 
         if (curs_row >= NUM_ROWS) {
             scroll_screen();
             curs_row = NUM_ROWS - 1;
         }
 
         update_cursor();
     }
 
     // NORMAL CHARACTERS
     else {
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
 
    // ----------------------------------------------------------------
    // Get a character from the keyboard buffer (blocking)
    // ----------------------------------------------------------------

    int getch() {
        uint8_t scancode = 0;
        int code;
    
        while (1) {
            if (buffer_get(&scancode)) {
                code = capitalize_if_shift(scancode_to_ascii(scancode));
    
                // return ALL valid codes including arrows
                if (code != 0) {
                    return code;
                }
            }
        }
    }
    
