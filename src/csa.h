/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * csa.h
 */

#ifndef CSA_H
#define CSA_H

#include <stdint.h>
#include "math.h"

// public interface

void command_load(char* args);  // registers the CSA loader
void csa_feedthru(char byte);   // internal feedthru handler
void csa_tick(void);         // internal tick handler for csa loading
void execute_csa(void);     // executes the loaded CSA binary

extern void* csa_entrypoint;    // holds entry address after load

typedef void (*serial_feedthru_callback_t)(char);

// definition of the table struct passed into .csa executables
typedef struct syscall_table {
    // original console/sys stuff
    void (*print)(const char*);
    void (*gotoxy)(size_t x, size_t y);
    void (*clear_screen)(void);
    void (*repaint_screen)(uint8_t fg, uint8_t bg);
    void (*set_color)(uint8_t fg, uint8_t bg);
    void (*delay_ms)(uint32_t ms);
    void (*disable_cursor)(void);
    void (*update_cursor)(void);
    int  (*getch)(void);
    void (*srand)(uint32_t seed);
    uint32_t (*rand)(uint32_t max);
    int (*extra_rand)(void);
    void (*read_rtc_datetime)(uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);
    void (*reset)(void);
    void (*beep)(uint32_t freq, uint32_t duration);
    int  (*getch_nb)(void);

    // formatting
    int  (*snprintf)(char*, uint32_t, const char*, ...);
    int  (*vsnprintf)(char*, uint32_t, const char*, va_list);

    // string ops
    char* (*strcpy)(char* dest, const char* src);
    char* (*strncpy)(char* dest, const char* src, size_t n);
    char* (*strcat)(char* dest, const char* src);
    char* (*strncat)(char* dest, const char* src, size_t n);
    size_t (*strlen)(const char* str);
    int (*strcmp)(const char* s1, const char* s2);
    int (*strncmp)(const char* s1, const char* s2, size_t n);
    char* (*strchr)(const char* str, int c);
    char* (*strrchr)(const char* str, int c);
    char* (*strstr)(const char* haystack, const char* needle);
    char* (*strtok)(char* str, const char* delim);

    // memory
    void* (*memcpy)(void* dest, const void* src, size_t n);
    void* (*memset)(void* ptr, int value, size_t num);
    int (*memcmp)(const void* ptr1, const void* ptr2, size_t num);

    // conversions
    void (*itoa)(int value, char* str);
    void (*itoa_base)(int value, char* str, int base);
    int (*atoi)(const char* str);
    long (*strtol)(const char* str, char** endptr, int base);
    void (*display_datetime)(void);

    void (*set_rtc_date)(uint8_t, uint8_t, uint8_t);
    void (*set_rtc_time)(uint8_t, uint8_t, uint8_t);
    const char* (*get_month_name)(uint8_t);

    void (*serial_init)(void);
    int (*serial_received)(void);
    char (*serial_read)(void);
    int (*serial_is_transmit_empty)(void);
    void (*serial_write)(char);
    void (*serial_write_string)(const char*);
    void (*set_serial_command)(int);
    void (*set_serial_waiting)(int);
    void (*set_serial_feedthru_callback)(serial_feedthru_callback_t callback);
    void (*serial_poll)(void);
    void (*serial_toggle)(void);

    void (*putchar)(char);
    void (*printc)(char);
    void (*println)(const char*);
    void (*newline)(void);
    void (*move_cursor_left)(void);
    void (*enable_cursor)(uint8_t, uint8_t);
    void (*enable_bright_bg)(void);

    // ─── fixed-point math ───────────────
    fixed_t (*fabs)(fixed_t x);
    fixed_t (*sqrt)(fixed_t x);
    fixed_t (*pow)(fixed_t base, fixed_t exp);
    fixed_t (*exp)(fixed_t x);
    fixed_t (*log)(fixed_t x);
    fixed_t (*fmod)(fixed_t a, fixed_t b);
    fixed_t (*sin)(fixed_t x);
    fixed_t (*cos)(fixed_t x);
    fixed_t (*tan)(fixed_t x);
    fixed_t (*asin)(fixed_t x);
    fixed_t (*acos)(fixed_t x);
    fixed_t (*atan)(fixed_t x);
    fixed_t (*sinh)(fixed_t x);
    fixed_t (*cosh)(fixed_t x);
    fixed_t (*tanh)(fixed_t x);
    fixed_t (*floor)(fixed_t x);
    fixed_t (*ceil)(fixed_t x);
    fixed_t (*round)(fixed_t x);
    int     (*signbit)(fixed_t x);
    int     (*isnan)(fixed_t x);
    int     (*isinf)(fixed_t x);

    // ─── float math ───────────────
    float   (*fabsf)(float x);
    float   (*sqrtf)(float x);
    float   (*powf)(float base, float exp);
    float   (*expf)(float x);
    float   (*logf)(float x);
    float   (*fmodf)(float a, float b);
    float   (*sinf)(float x);
    float   (*cosf)(float x);
    float   (*tanf)(float x);
    float   (*asinf)(float x);
    float   (*acosf)(float x);
    float   (*atanf)(float x);
    float   (*sinhf)(float x);
    float   (*coshf)(float x);
    float   (*tanhf)(float x);
    float   (*floorf)(float x);
    float   (*ceilf)(float x);
    float   (*roundf)(float x);
    int     (*signbitf)(float x);
    int     (*isnanf)(float x);
    int     (*isinff)(float x);
} syscall_table_t;

#endif // CSA_H
