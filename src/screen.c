/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * screen.c
 */

#include "screen.h"
#include "os.h"
#include "console.h" // Assumes you have a console module

volatile struct Char* vga_buffer = (volatile struct Char*)VGA_ADDRESS;
extern uint8_t default_color;

size_t curs_row = 0;
size_t curs_col = 0;

// Enable the VGA cursor
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A); // Cursor start register
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    outb(0x3D4, 0x0B); // Cursor end register
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

// Disable the VGA cursor (optional)
void disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

// Update VGA cursor position
void update_cursor() {
    uint16_t position = curs_row * NUM_COLS + curs_col; // Calculate position
    outb(0x3D4, 14);                                   // Select high byte
    outb(0x3D5, (position >> 8) & 0xFF);               // Send high byte
    outb(0x3D4, 15);                                   // Select low byte
    outb(0x3D5, position & 0xFF);                      // Send low byte
}

// Scroll the screen when the bottom is reached
void scroll_screen() {
    retain_clock = 0;
    for (size_t i = 1; i < NUM_ROWS; i++) {
        for (size_t j = 0; j < NUM_COLS; j++) {
            size_t from = i * NUM_COLS + j;
            size_t to = (i - 1) * NUM_COLS + j;
            vga_buffer[to] = vga_buffer[from];
        }
    }
    for (size_t j = 0; j < NUM_COLS; j++) {
        vga_buffer[(NUM_ROWS - 1) * NUM_COLS + j] = (struct Char){' ', default_color};
    }
}

void putchar(char c) {
    if (c == '\n') {
        curs_row++;
        curs_col = 0;
    } else if (c == '\r') {
        curs_col = 0;
    } else {
        const size_t index = curs_row * NUM_COLS + curs_col;
        vga_buffer[index].character = c;
        vga_buffer[index].color = default_color;
        curs_col++;
        if (curs_col >= NUM_COLS) {
            curs_col = 0;
            curs_row++;
        }
    }

    if (curs_row >= NUM_ROWS) {
        scroll_screen();
        curs_row = 0; 
    }

    update_cursor();
}