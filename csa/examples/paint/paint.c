#include "syscall.h"
#include <stdint.h>

#define WIDTH 80
#define HEIGHT 25

syscall_table_t* sys;

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
    sys->gotoxy(cursor_x, cursor_y);
    sys->set_color(current_fg, current_bg);
    char block[2] = { 219, '\0' };
    sys->print(block);
}

void undraw_cursor() {
    sys->gotoxy(cursor_x, cursor_y);
    sys->set_color(current_fg, current_bg);
    sys->print(" ");
}

void draw_help() {
    sys->gotoxy(0, 0);
    sys->set_color(15, 0);
    sys->print("Arrow keys = move | C to clear | F to fill | +/- to change color | SPACE to exit");
}

void fill_screen() {
    for (int y = 1; y < HEIGHT; y++) {
        sys->gotoxy(0, y);
        for (int x = 0; x < WIDTH; x++) {
            sys->set_color(current_fg, current_bg);
            // char block[2] = { 219, '\0' };
            sys->print(" "); //(block);
        }
    }
}

void clear_screen_art() {
    for (int y = 1; y < HEIGHT; y++) {
        sys->gotoxy(0, y);
        for (int x = 0; x < WIDTH; x++) {
            sys->set_color(15, 15);
            sys->print(" ");
        }
    }
}

void main_function() {
    sys = *(syscall_table_t**)0x200000;
    sys->srand(sys->extra_rand());
    sys->clear_screen();
    sys->disable_cursor();
    draw_help();

    while (1) {
        draw_cursor();
        int key = sys->getch();

        undraw_cursor();

        switch (key) {
            case 'w': case -1: cursor_y = (cursor_y > 1) ? cursor_y - 1 : 1; break;
            case 's': case -2: cursor_y = (cursor_y < HEIGHT - 1) ? cursor_y + 1 : HEIGHT - 1; break;
            case 'a': case -3: cursor_x = (cursor_x > 0) ? cursor_x - 1 : 0; break;
            case 'd': case -4: cursor_x = (cursor_x < WIDTH - 1) ? cursor_x + 1 : WIDTH - 1; break;

            case '+': current_bg = (current_bg + 1) % 16; break;
            case '-': current_bg = (current_bg - 1) % 16; break;

            case 'f': current_mode = FILL; fill_screen(); break;
            case 'c': current_mode = CLEAR; clear_screen_art(); break;

            case ' ': sys->reset(); return;
        }

        // paint if in pen mode
        if (current_mode == PEN) {
            sys->gotoxy(cursor_x, cursor_y);
            sys->set_color(current_fg, current_bg);
            // char block[2] = { 219, '\0' };
            sys->print(" "); //(block);
        }
    }
}
