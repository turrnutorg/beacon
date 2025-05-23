#include "syscall.h"
#include "snake.h"

extern char __bss_start;
extern char __bss_end;

void main(void) {
    for (char* p = &__bss_start; p < &__bss_end; ++p)
        *p = 0;
    extern syscall_table_t* sys;
    sys = *( (syscall_table_t**)0x200000 );
    snake_start();
}
