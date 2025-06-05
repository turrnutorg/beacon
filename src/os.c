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
#include "time.h"
#include "serial.h"
#include "csa.h"
#include <stdint.h>
#include "ff.h"  // Include FatFs header
#include "stdlib.h"  // For memory management functions
#include "disks.h"
// Declare a FAT file system object and a file object
FATFS fs;     // File system object
FIL fil;      // File object
FRESULT res;  // Result code

extern uint8_t __heap_start;
extern uint8_t __heap_end;

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

int firstboot = 1;  // Flag to indicate if this is the first boot

void reboot() {
    asm volatile ("cli");
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    asm volatile ("hlt");
}

void start() {
    if (firstboot) {
        pit_init_for_polling();

        serial_init();
        serial_toggle();
        serial_write_string("serial up n runnin\r\n");

        // Mount the filesystem after initializing the screen and serial
        mount_filesystem();
        curs_row++;
        firstboot = 0;  // Set firstboot to false after the first run
    }
    
    set_serial_command(1);
    set_serial_waiting(0);

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
    input_len = 0;
    update_cursor();

    int loop_counter = 0;

    while (1) {
        uint8_t scancode;

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

        serial_poll();
        csa_tick();
    }
}

