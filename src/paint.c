/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * paint.c
 */

#include "os.h"
#include "screen.h"
#include "console.h"
#include "keyboard.h"
#include "stdlib.h"
#include <stdint.h>

#define WIDTH 80
#define HEIGHT 25

typedef enum {
    PEN,
    FILL,
    CLEAR
} mode_t;

int cursor_x = WIDTH / 2;
int cursor_y = HEIGHT / 2;
uint8_t current_fg = 15;
uint8_t current_bg = 0;
mode_t current_mode = PEN;

void draw_cursor() {
    gotoxy(cursor_x, cursor_y);
    set_color(current_fg, current_bg);
    char block[2] = { 219, '\0' };
    print(block);
}

void undraw_cursor() {
    gotoxy(cursor_x, cursor_y);
    set_color(current_fg, current_bg);
    print(" ");
}

void draw_help() {
    gotoxy(0, 0);
    set_color(15, 0);
    print("Arrow keys = move | C to clear | F to fill | +/- to change color | SPACE to exit");
}

void fill_screen() {
    for (int y = 1; y < HEIGHT; y++) {
        gotoxy(0, y);
        for (int x = 0; x < WIDTH; x++) {
            set_color(current_fg, current_bg);
            // char block[2] = { 219, '\0' };
            print(" "); //(block);
        }
    }
}

void clear_screen_art() {
    for (int y = 1; y < HEIGHT; y++) {
        gotoxy(0, y);
        for (int x = 0; x < WIDTH; x++) {
            set_color(15, 15);
            print(" ");
        }
    }
}

void paint_main() {
    srand(extra_rand());
    clear_screen();
    disable_cursor();
    draw_help();

    while (1) {
        draw_cursor();
        int key = getch();

        undraw_cursor();

        switch (key) {
            case 'w': case -1: cursor_y = (cursor_y > 1) ? cursor_y - 1 : 1; break;
            case 's': case -2: cursor_y = (cursor_y < HEIGHT - 1) ? cursor_y + 1 : HEIGHT - 1; break;
            case 'a': case -3: cursor_x = (cursor_x > 0) ? cursor_x - 1 : 0; break;
            case 'd': case -4: cursor_x = (cursor_x < WIDTH - 1) ? cursor_x + 1 : WIDTH - 1; break;

            case '+': case '=': current_bg = (current_bg + 1) % 16; break;
            case '-': current_bg = (current_bg - 1) % 16; break;

            case 'f': current_mode = FILL; fill_screen(); break;
            case 'c': current_mode = CLEAR; clear_screen_art(); break;

            case ' ': start(); return;
        }

        // paint if in pen mode
        if (current_mode == PEN) {
            gotoxy(cursor_x, cursor_y);
            set_color(current_fg, current_bg);
            // char block[2] = { 219, '\0' };
            print(" "); //(block);
        }
    }
}
