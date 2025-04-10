/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * os.c
 */

#include "os.h"
#include "screen.h"
#include "keyboard.h"
#include "console.h"
#include "stdtypes.h"
#include "command.h"
#include "string.h"
#include "speaker.h"
#include "velocity.h"
#include "time.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_FREQUENCY 1193182

// External variables from screen.c
extern volatile struct Char* vga_buffer;
extern uint8_t default_color;

// External variables from keyboard.c
extern char input_buffer[INPUT_BUFFER_SIZE];
extern size_t input_len;

// Cursor position from screen.c
extern size_t curs_row;
extern size_t curs_col;

// Variables for setup and clock
int setup_mode = 0;
int retain_clock = 1;
int setup_ran = 0;

extern void* mb_info;
void* g_mb_info = 0;

void pit_wait(uint16_t ticks) {
    outb(PIT_COMMAND, 0x30); // channel 0, mode 0, binary
    outb(PIT_CHANNEL0, ticks & 0xFF); // low byte
    outb(PIT_CHANNEL0, (ticks >> 8) & 0xFF); // high byte

    // spinlock while bit 5 of port 0x61 is cleared (timer active)
    while (!(inb(0x61) & 0x20));
}

void delay_ms(int ms) {
    while (ms--) {
        pit_wait(PIT_FREQUENCY / 1000); // wait 1 ms
    }
}

void run_initial_setup() {
    println("Do you want to run the initial setup? [Just Date & Time] (Y/n):");
    curs_row++;
    update_cursor();

    char response = '\0';

    while (response == '\0') {
        uint8_t scancode;
        if (buffer_get(&scancode)) {
            handle_keypress(scancode);
            response = scancode_to_ascii(scancode);
        }
    }

    if (response == 'n' || response == 'N') {
        println("Skipping initial setup.");
        curs_row++;
        input_len = 0;
        retain_clock = 1;
        update_cursor();
        setup_ran = 1;
        setup_mode = 0;
        delay_ms(500);
        clear_screen();
        return;
    }

    input_len = 0;
    setup_ran = 0;
    clear_screen();
    row = 0;
    col = 0;

    setup_mode = 1;

    println("Enter current date (DD MM YY):");
    input_len = strlen("setdate ");
    strncpy(input_buffer, "setdate ", INPUT_BUFFER_SIZE);
    move_cursor_back();
    curs_col = 0;
    curs_row = 1;
    update_cursor();

    while (setup_mode == 1) {
        uint8_t scancode;
        if (buffer_get(&scancode)) {
            handle_keypress(scancode);

            char ascii = scancode_to_ascii(scancode);
            if (ascii == '\n') {
                input_buffer[input_len] = '\0';
                println("");
                process_command(input_buffer);

                setup_mode = 2;

                curs_row++;
                println("Enter current time (HH MM SS):");
                input_len = strlen("settime ");
                strncpy(input_buffer, "settime ", INPUT_BUFFER_SIZE);
                move_cursor_back();
                curs_col = 0;
                update_cursor();
            }
        }
    }

    while (setup_mode == 2) {
        uint8_t scancode;
        if (buffer_get(&scancode)) {
            handle_keypress(scancode);

            char ascii = scancode_to_ascii(scancode);
            if (ascii == '\n') {
                input_buffer[input_len] = '\0';
                println("");
                process_command(input_buffer);

                setup_mode = 0;
            }
        }
    }

    setup_ran = 1;
    setup_mode = 0;
    delay_ms(500);
    clear_screen();
}

void reboot() {
    asm volatile ("cli");
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    asm volatile ("hlt");
}

void move_cursor_back() {
    curs_col = 0;
    update_cursor();
}

void start() {
    clear_screen();
    set_color(GREEN_COLOR, WHITE_COLOR);
    repaint_screen(GREEN_COLOR, WHITE_COLOR);

    enable_bright_bg();
    enable_cursor(0, 15);

    if (setup_ran == 0) {
        run_initial_setup();
    }

    col = 0;
    row = 0;
    setup_mode = 0;
    retain_clock = 1;

    println("Copyright (c) 2025 Turrnut Open Source Organization.");
    println("");
    display_datetime();
    println("Type a command (type 'help' for a list):");

    curs_row = 3;
    curs_col = 0;
    g_mb_info = mb_info;
    update_cursor();

    int loop_counter = 0;

    while (1) {
        uint8_t scancode;

        // GET FROM BUFFER, NOT PORT
        if (buffer_get(&scancode)) {
            handle_keypress(scancode);
        }

        if (retain_clock == 1) {
            loop_counter++;
            if (loop_counter >= 50000) {
                display_datetime();
                loop_counter = 0;
            }
        }

        update_rainbow(); // whatever insane animation ye got going
    }
}
