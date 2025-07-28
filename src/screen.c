/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * screen.c
 */

 #include "screen.h"
 #include "os.h"
 #include "console.h" 
 #include "stdlib.h"
 
 volatile struct Char* vga_buffer = (volatile struct Char*)VGA_ADDRESS;
 extern uint8_t default_color;
 
 size_t curs_row = 0;
 size_t curs_col = 0;
 
 int is_scrolling = 0;

 char block[2] = { 219, '\0' };
 
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
 
 void scroll_screen() {
     is_scrolling = 1;  // Set scrolling flag
     // If cursor is at or past the last row, move it to the second-to-last row
     if (curs_row >= NUM_ROWS - 1) {
         curs_row = NUM_ROWS - 2;
     }
 
     // Allow the new line to print in the last row
     update_cursor();  // Print to the last row (cursor is at second-to-last)
 
     // Shift the rows up by one to make space for the next line
     for (size_t i = 0; i < NUM_ROWS - 1; i++) {  // Don't shift the last row
         for (size_t j = 0; j < NUM_COLS; j++) {
             size_t from = (i + 1) * NUM_COLS + j;
             size_t to = i * NUM_COLS + j;
             vga_buffer[to] = vga_buffer[from];
         }
     }
 
     // Clear the last row after the shift
     for (size_t j = 0; j < NUM_COLS; j++) {
         vga_buffer[(NUM_ROWS - 1) * NUM_COLS + j] = (struct Char){' ', default_color};
     }
 
     // Move the cursor back to the second-to-last row after printing
     curs_row = NUM_ROWS - 3;
     update_cursor();  // Update cursor position after the scroll
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
 
 void enable_bright_bg() {
    inb(0x3DA);                 // reset attribute flip-flop, just read
    outb(0x3C0, 0x10);          // select Attribute Mode Control register
    uint8_t mode = inb(0x3C1);  // VGA doesnae allow readin' like this; ye need tae track yer own copy
    mode &= ~(1 << 3);          // clear blink bit (bit 3), enable bright BG
    outb(0x3C0, mode);          // write back corrected mode

    outb(0x3C0, 0x20);          // re-enable video output
}

void drawTile(unsigned char *tile, int x, int y, int width, int height, int bg_color) {
    int k = 0;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            gotoxy(x + j, y + i);

            if (tile[k] != 17) {
                set_color(tile[k], bg_color);
                print(block);
            }

            k++;
        }
    }
}

void set_palette_color(unsigned char index, unsigned char rgb_val) {
    inb(0x3DA);
    outb(0x3C0, (index & 0x1F) | 0x20);
    outb(0x3C0, rgb_val);
    inb(0x3DA);
}

unsigned char ega_color(unsigned char r, unsigned char g, unsigned char b) {
    return ((r & 0x03) << 4) | ((g & 0x03) << 2) | (b & 0x03);
}
 