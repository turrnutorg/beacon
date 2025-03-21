/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * screen.h
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "stdtypes.h"

// VGA constants
#define VGA_ADDRESS 0xB8000
#define NUM_COLS 80
#define NUM_ROWS 25

// Character structure for VGA
struct Char {
    char character;
    uint8_t color;
};

// Screen-related variables
extern volatile struct Char* vga_buffer;
extern uint8_t default_color;

// Cursor position
extern size_t curs_row; // Ensure this is size_t
extern size_t curs_col; // Ensure this is size_t

extern int is_scrolling;

// Screen-related functions
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void disable_cursor();
void update_cursor();
void scroll_screen();
void putchar(char c);
void enable_bright_bg();

#endif // SCREEN_H
