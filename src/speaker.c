/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * speaker.c
 */
#include <stdint.h>
#include "time.h"
#include "port.h"
#include "speaker.h"

// beep_custom lets ye set the frequency (in Hz) and duration (in ms) for yer ear-assault
// if ye set frequency to 0, don't even bother, ye brainless twat
void beep(uint32_t frequency, uint32_t ms) {
    if (frequency == 0) {
        return; // don't be a clueless bastard
    }

    uint32_t div = 1193180 / frequency;  // calculate PIT divider

    // configure PIT channel 2 for square wave
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div & 0xFF));        // low byte
    outb(0x42, (uint8_t)((div >> 8) & 0xFF)); // high byte

    // turn on the PC speaker (bits 0 and 1)
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 0x03);

    // ACTUALLY wait now using proper delay
    delay_ms(ms);

    // shut that shit off
    tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}