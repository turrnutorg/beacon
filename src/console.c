/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * console.c
 */

#include "console.h"
#include "screen.h"
#include "port.h"
#include "time.h"
#include "serial.h"
#include "string.h"

// Define col and row in this file
size_t col = 0;
size_t row = 0;

// Default color setting
uint8_t default_color = GREEN_COLOR | WHITE_COLOR << 4;
struct Char *buffer = (struct Char *)0xb8000;

void clear_row(size_t row)
{
    struct Char empty = (struct Char){
        .character = ' ',
        .color = default_color,
    };

    for (size_t col = 0; col < NUM_COLS; col++)
    {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void clear_screen()
{
    for (size_t i = 0; i < NUM_ROWS; i++)
    {
        clear_row(i);
    }
}

void newline()
{
    col = 0;  // Reset the column to the start

    if (row < NUM_ROWS - 1)
    {
        row++;  // Move to the next row
        return;
    }

    scroll_screen();  // Scroll the screen if we're at the last row
}

/* Print a character*/
void printc(char character)
{
    if (character == '\n')
    {
        newline();
        return;
    }

    if (col >= NUM_COLS)
    {
        newline();
        curs_row++;
        update_cursor();
    }

    buffer[col + NUM_COLS * row] = (struct Char){
        .character = (uint8_t)character,
        .color = default_color,
    };

    col++;  // Move to the next column
}

/* Print a string */
void print(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        printc(str[i]);  // Print each character
    }
}

/* Print a string and a new line */
void println(const char *str)
{
    print(str);
    newline();  // Move to a new line after printing
    update_cursor();
}

void print_hex(uintptr_t val) {
    char hex[] = "0123456789ABCDEF";
    char buf[11] = "0x00000000";
    for (int i = 9; i >= 2; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    println(buf);
}

void set_color(uint8_t foreground, uint8_t background)
{
    default_color = foreground + (background << 4);  // Update color
}

void move_cursor_left()
{
    if (col > 0)  // Don't go past the first column
    {
        col--;  // Decrease the column position
        buffer[col + NUM_COLS * row].character = ' ';  // Clear the character in that position
    }
}

void repaint_screen(uint8_t fg_color, uint8_t bg_color) {
    uint8_t color = (bg_color << 4) | fg_color;

    for (int i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        vga_buffer[i].color = color;
    }
}

int extra_rand() {
    uint8_t sec = cmos_read(0x00);
    uint8_t min = cmos_read(0x02);
    uint8_t hour = cmos_read(0x04);
    uint8_t day = cmos_read(0x07);
    uint8_t mon = cmos_read(0x08);
    uint8_t year = cmos_read(0x09);

    // mix it all up like a methhead makin potions
    uint32_t seed = sec + (min << 2) + (hour << 4) + (day << 6) + (mon << 8) + (year << 10);

    // linear congruential generator like yer nan used in '85
    seed = seed ^ (seed << 13);
    seed = seed ^ (seed >> 17);
    seed = seed ^ (seed << 5);

    return (int)(seed & 0x7FFFFFFF); // keep it positive like yer gran before she saw your code
}

 // implement gotoxy: set os print positions and update hardware cursor
 void gotoxy(size_t x, size_t y) {
    row = y;         // console module's current row
    col = x;         // console module's current col
    curs_row = y;    // vga cursor row
    curs_col = x;    // vga cursor col
    update_cursor();
}