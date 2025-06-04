ENTRY(boot)

SECTIONS {
    . = 1M;

    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text*)
        *(.rodata*)
    }

    .data ALIGN(4K) : {
        *(.data*)
    }

    .bss ALIGN(4K) (NOLOAD) : {
        _bss_start = .;
        *(COMMON)
        *(.bss*)
        _bss_end = .;
    }

    .stack ALIGN(4K) (NOLOAD) : {
        stack_bottom = .;
        . += 8192;
        stack_top = .;
    }

    .heap ALIGN(4K) (NOLOAD) : {
        __heap_start = .;
        . += 0x100000; /* 1 MB heap */
        __heap_end = .;
    }

    /DISCARD/ : {
        *(.fini_array*)
        *(.comment)
    }
}
