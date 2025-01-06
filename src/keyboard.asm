# Copyright (c) Turrnut Open Source Organization
# Under the GPL v3 License
# See COPYING for information on how you can use this file
#
# keyboard.asm
#
.section .data
.global scancode_buffer
scancode_buffer:
    .byte 0                          # holds the last valid scancode

.section .text
.global keyboard_handler
.global get_key

keyboard_handler:
    pusha                        # Save all registers

    inb $0x60, %al               # Read scancode from keyboard port
    test $0x80, %al              # Check if the scancode is a key release
    jnz skip_handler             # Skip releases (we only care about keypresses)

    movzbl %al, %eax             # Zero-extend scancode to 32 bits
    cmp $58, %al                 # Check if scancode is within valid range (you could adjust this range)
    ja skip_handler              # Skip invalid scancodes

    # Add the scancode to buffer
    pushl %eax                   # Push scancode onto stack for C function
    call handle_keypress         # Call the C function
    add $4, %esp                 # Clean up stack

skip_handler:
    popa                         # Restore registers
    movb $0x20, %al              # Send end-of-interrupt to PIC
    outb %al, $0x20
    iret                         # Return from interrupt


get_key:
    inb $0x60, %al                   # read from keyboard port
    ret                              # return the scancode
