#include "syscall.h"
#include "your_code_here.h"
/*
 * The entry point for the payload.
 * This file is kept minimal; it zeroes out .bss, initializes the syscall pointer,
 * and calls the main program.
 */
extern char __bss_start;
extern char __bss_end;

void zero_bss(void);  // declared in dungeon.h and defined in dungeon.c

// main() is kept as clean as possible; all game logic is in dungeon.c
void main(void) {
    zero_bss();
    // initialize the syscall table pointer from the fixed location (set up by the loader)
    // note: this assignment sets the global variable "sys" in your_code_here.c
    extern syscall_table_t* sys;
    sys = *( (syscall_table_t**)0x200000 );
    // now launch the app!
    main_function();
}
