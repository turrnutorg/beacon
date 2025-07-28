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

int firstboot = 1;  // Flag to indicate if this is the first boot

// resets the os thru keyboard controller
void reboot() {
    asm volatile ("cli");
    while (inb(0x64) & 0x02); 
    outb(0x64, 0xFE);
    asm volatile ("hlt");
}

// write in your own custom logo, or use this one :)
static unsigned char logo[] = {
    17, 17, 17, 17, 17, 17, 17, 17, 14, 14, 14, 14,  4,  4,  4,  4, 14, 14, 14, 14, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 14, 14,  4,  4,  4,  4, 14, 14, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 14, 14, 14, 14, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17,  4,  4, 17, 17, 17, 17,  4,  4,  4,  4, 17, 17, 17, 17,  4,  4, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17,  4,  4,  4,  4, 17, 17, 17, 17,  4,  4,  4,  4, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17,  2,  2,  2,  2,  2,  2, 15, 15, 15, 15,  2,  2,  2,  2,  2,  2, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17,  2,  2,  2,  2,  2,  2, 15, 15, 15, 15,  2,  2,  2,  2,  2,  2, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17,  2,  2,  2,  2,  2,  2, 15, 15, 15, 15,  2,  2,  2,  2,  2,  2, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 15, 15, 10, 10, 15, 15, 10, 10, 15, 15, 10, 10, 17, 17, 17, 17, 17, 17, 17, 17,
    15, 15, 15, 15, 15, 15, 17, 17, 17, 17, 15, 15, 10, 10, 15, 15, 10, 10, 17, 17, 17, 17, 15, 15, 15, 15, 15, 15,
    17, 17, 17, 17, 15, 15, 15, 15, 17, 17, 15, 15, 15, 15, 15, 15, 15, 15, 17, 17, 15, 15, 15, 15, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 15, 15, 15, 15, 15, 15, 17, 17, 17, 17, 15, 15, 15, 15, 15, 15, 17, 17, 17, 17, 17, 17
};

void start() {
    pit_init_for_polling();

    clear_screen();
    set_color(15, 0);
    repaint_screen(15, 0);
    println("Booting Beacon...");
    println("This may take a moment.");
    println("");
    enable_bright_bg();
    drawTile(logo, 0, 8, 28, 16, 0);

    gotoxy(0, 2);

    set_color(15, 0);

    // Mount all file systems and initialize cwd for each drive
    println("Mounting file systems...");
    mount_all_filesystems();
    curs_row++;

    accept_key_presses = 0;

    // this is here to let the user know that the OS, is, in fact, working and not frozen
    clear_screen();
    set_color(GREEN_COLOR, WHITE_COLOR);
    repaint_screen(GREEN_COLOR, WHITE_COLOR);
    gotoxy(0, 0);
    print("*boot jingle*");
    gotoxy(0, 0);
    beep(440, 1000);
    beep(2, 400);
    beep(440, 1000);
    command_shell_loop();
}



