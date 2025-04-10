#ifndef CSA_H
#define CSA_H

#include <stdint.h>

// public interface

void command_load(char* args);  // registers the CSA loader
void csa_feedthru(char byte);   // internal feedthru handler
void csa_tick(void);         // internal tick handler for csa loading
void execute_csa(void);     // executes the loaded CSA binary
void csa_clear(void);    // clears the CSA buffer and resets state

extern void* csa_entrypoint;    // holds entry address after load

// definition of the table struct passed into .csa executables
typedef struct syscall_table {
    void (*print)(const char*);
    void (*gotoxy)(size_t x, size_t y); // use size_t not int, ya peanut
    void (*clear_screen)(void);
    void (*repaint_screen)(uint8_t fg, uint8_t bg); // match yer defs
    void (*set_color)(uint8_t fg, uint8_t bg);
    void (*delay_ms)(int ms);
    void (*disable_cursor)(void);
    void (*update_cursor)(void);
    int  (*getch)(void);
    void (*srand)(uint32_t seed);
    uint32_t (*rand)(uint32_t max);
    int (*extra_rand)(void);
    void (*read_rtc_datetime)(uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*, uint8_t*);
    void (*reset)(void);
    int  (*snprintf)(char*, uint32_t, const char*, ...);
    int  (*vsnprintf)(char*, uint32_t, const char*, va_list);
    char* (*strcpy)(char* dest, const char* src);
    int  (*strcmp)(const char* s1, const char* s2);
    uint32_t (*strlen)(const char* str);
    void (*beep)(uint32_t freq, uint32_t duration); // was (int, int), now proper
    int  (*strncmp)(const char*, const char*, size_t);
} syscall_table_t;


#endif // CSA_H
