/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * console.c
 */

#include "console.h"
#include "screen.h"


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

void print(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++)
    {
        printc(str[i]);  // Print each character
    }
}

void println(const char *str)
{
    print(str);
    newline();  // Move to a new line after printing
    update_cursor();
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


void int_to_str(int value, char* str, int min_digits) {
    char buffer[16];
    int i = 0;
    int is_negative = 0;

    if (value < 0) {
        is_negative = 1;
        value = -value;
    }

    do {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    } while (value);

    // add leading zeroes
    while (i < min_digits) {
        buffer[i++] = '0';
    }

    if (is_negative) {
        buffer[i++] = '-';
    }

    // reverse it into str
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }

    str[j] = '\0';
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

// global seed, cos why the fuck not
unsigned int rng_seed = 123456789;

// call this tae set the seed, if ye give a toss
void srand(unsigned int seed) {
    rng_seed = seed;
}

// random number between 0 and max-1
unsigned int rand(unsigned int max) {
    rng_seed = rng_seed * 1103515245 + 12345;
    return (rng_seed >> 16) % max;
}


