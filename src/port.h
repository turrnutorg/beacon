/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * port.h
 */

#ifndef port_header
#define port_header
#include <stdint.h>

// 8-bit Port
typedef struct {
    uint16_t port_number;

} Port8Bit;

void initPort8Bit(Port8Bit* port, uint16_t port_number);
void writePort8Bit(Port8Bit* port, uint8_t data);
uint8_t readPort8Bit(Port8Bit* port);

// 8-bit Port Slow
typedef struct {
    uint16_t port_number;

} Port8BitSlow;

void initPort8BitSlow(Port8BitSlow* port, uint16_t port_number);
void writePort8BitSlow(Port8BitSlow* port, uint8_t data);
// same as 8-bit Port's
uint8_t readPort8BitSlow(Port8BitSlow* port);

// 16-bit Port
typedef struct {
    uint16_t port_number;

} Port16Bit;

void initPort16Bit(Port16Bit* port, uint16_t port_number);
void writePort16Bit(Port16Bit* port, uint16_t data);
uint16_t readPort16Bit(Port16Bit* port);

// 32-bit Port
typedef struct {
    uint16_t port_number;

} Port32Bit;

void initPort32Bit(Port32Bit* port, uint16_t port_number);
void writePort32Bit(Port32Bit* port, uint32_t data);
uint32_t readPort32Bit(Port32Bit* port);

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
void outw(uint16_t port, uint16_t value);
uint16_t inw(uint16_t port);

#endif