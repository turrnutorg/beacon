/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * speaker.c
 */
#include <stdint.h>
#include "speaker.h"

// output a byte to the given port, don't fuck it up, ya numpty
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// read a byte from the given port, try not to stick yer fingers in there, ya daft git
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// beep_custom lets ye set the frequency (in Hz) and duration (in ms) for yer ear-assault
// if ye set frequency to 0, don't even bother, ye brainless twat
void beep(uint32_t frequency, uint32_t ms) {
    if (frequency == 0) {
        return; // don't be a clueless bastard
    }
    
    uint32_t div = 1193180 / frequency;  // calculate the divider for the PIT
    
    // set up the PIT channel 2 in square wave mode
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div & 0xFF));        // low byte
    outb(0x42, (uint8_t)((div >> 8) & 0xFF));   // high byte
    
    // enable the PC speaker by setting bits 0 and 1 at port 0x61
    uint8_t tmp = inb(0x61);
    if (!(tmp & 3)) {
        outb(0x61, tmp | 3);
    }
    
    // a crude delay loop to keep the beep on for roughly 'ms' milliseconds
    // ye might need to adjust this multiplier dependin on yer hardware speed, ye lazy sod
    volatile uint32_t delay = ms * 100000;
    for (volatile uint32_t i = 0; i < delay; i++) {
         __asm__ volatile ("nop");
    }
    
    // disable the speaker and stop the damn noise
    tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

