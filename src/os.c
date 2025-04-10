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
#include "command.h"
#include "string.h"
#include "speaker.h"
#include "velocity.h"
#include "time.h"
#include "serial.h"
#include "csa.h"
#include <stdint.h>

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

int retain_clock = 1;

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

void reboot() {
    asm volatile ("cli");
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    asm volatile ("hlt");
}

void start() {
    serial_init();
    serial_toggle();
    serial_write_string("serial command handler initialized\r\n");

    set_serial_command(1);
    set_serial_waiting(0);
    serial_write_string("serial command handler set\r\n");
    serial_write_string("Beacon booting...\r\n");

    clear_screen();
    set_color(GREEN_COLOR, WHITE_COLOR);
    repaint_screen(GREEN_COLOR, WHITE_COLOR);

    enable_bright_bg();
    enable_cursor(0, 15);

    col = 0;
    row = 0;
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
        serial_poll(); // always poll for serial input
        csa_tick(); // <- call the safe csa processor here
    }
}
