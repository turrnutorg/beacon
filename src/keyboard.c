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
 #include "disks.h"
 #include <stdint.h>

 int accept_key_presses = 1;
 
 char input_buffer[INPUT_BUFFER_SIZE];
 size_t input_len = 0;
 int shift = 0;  // fixed this garbage
 int display_prompt = 1;

 static int shift_count = 0;
 
 // these exist in keyboard.asm now, not here
 extern volatile uint8_t key_buffer[16];    // must match asm .space 16
 extern volatile size_t buffer_head;
 extern volatile size_t buffer_tail;
 
 volatile uint8_t key_states[128] = {0}; // tracks if a key is currently pressed

 #define SCANQ_SIZE 16
static volatile uint8_t scan_queue[SCANQ_SIZE];
static volatile size_t scan_head = 0;
static volatile size_t scan_tail = 0;

static void scan_enqueue(uint8_t sc) {
    size_t next = (scan_head + 1) % SCANQ_SIZE;
    if (next != scan_tail) {
        scan_queue[scan_head] = sc;
        scan_head = next;
    }
}

static int scan_dequeue(uint8_t* sc) {
    if (scan_head == scan_tail) return 0;
    *sc = scan_queue[scan_tail];
    scan_tail = (scan_tail + 1) % SCANQ_SIZE;
    return 1;
}
 
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
    // shift key events
    if (scancode == 0x2A || scancode == 0x36) {
        if (!key_states[scancode]) {
            key_states[scancode] = 1;
            shift_count++;
            shift = 1;
        }
        return;
    }

    if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) {
        key_states[scancode & 0x7F] = 0;
        if (shift_count > 0) shift_count--;
        if (shift_count == 0) shift = 0;
        return;
    }

    if (scancode & 0x80) {
        key_states[scancode & 0x7F] = 0;
        return;
    }

    if (key_states[scancode]) return;
    key_states[scancode] = 1;

    char ascii = capitalize_if_shift(scancode_to_ascii(scancode));
    if (!ascii) return;

    // add to input buffer
    if (input_len < INPUT_BUFFER_SIZE - 1) {
        input_buffer[input_len++] = ascii;
        input_buffer[input_len] = '\0';
    }

    // display printable characters only
    if (ascii >= 32 && ascii <= 126) {
        vga_buffer[curs_row * NUM_COLS + curs_col] = (struct Char){ascii, default_color};
        curs_col++;
        if (curs_col >= NUM_COLS) {
            curs_col = 0;
            curs_row++;
            if (curs_row >= NUM_ROWS) {
                scroll_screen();
                curs_row = NUM_ROWS - 1;
            }
        }
        update_cursor();
    }

    // ignore arrows, enter, backspace, etc.
    // they're still buffered — app decides what to do
}
 
    // ----------------------------------------------------------------
    // Get a character from the keyboard buffer (blocking)
    // ----------------------------------------------------------------

int getch() {
    accept_key_presses = 1;
    uint8_t scancode = 0;
    int code;

    while (1) {
        if (scan_dequeue(&scancode)) {
            code = capitalize_if_shift(scancode_to_ascii(scancode));
            if (code != 0) return code;
        }
    }
}

int getch_nb() {
    accept_key_presses = 1;
    uint8_t scancode = 0;
    int code;

    if (scan_dequeue(&scancode)) {
        code = capitalize_if_shift(scancode_to_ascii(scancode));
        if (code != 0) return code;
    }

    return -1;
}

 void irq_keyboard_handler_c(uint8_t scancode) {
    if (accept_key_presses) {
        scan_enqueue(scancode);
        handle_keypress(scancode);
    }
}
