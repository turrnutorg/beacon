/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * console.h
 */

#pragma once
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

extern size_t col;  // Declare external variables
extern size_t row;

enum Color
{
	BLACK_COLOR,
	BLUE_COLOR,
	GREEN_COLOR,
	CYAN_COLOR,
	RED_COLOR,
	MAGENTA_COLOR,
	BROWN_COLOR,
	LIGHT_GRAY_COLOR,
	DARK_GRAY_COLOR,
	LIGHT_BLUE_COLOR,
	LIGHT_GREEN_COLOR,
	LIGHT_CYAN_COLOR,
	LIGHT_RED_COLOR,
	PINK_COLOR,
	YELLOW_COLOR,
	WHITE_COLOR
};

void clear_screen();
void newline();
void printc(char c);
void print(const char *str);
void println(const char *str);
void set_color(uint8_t fg, uint8_t bg);
void print_hex(uintptr_t val);

void move_cursor_left();

int extra_rand();

void gotoxy(size_t x, size_t y);
void repaint_screen(uint8_t fg_color, uint8_t bg_color);

#endif
