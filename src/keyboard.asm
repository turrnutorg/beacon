# Copyright (c) Turrnut Open Source Organization
# Under the GPL v3 License
# See COPYING for information on how you can use this file
#
# keyboard.asm
#

.section .text
.global keyboard_handler
.global get_key
.extern irq_keyboard_handler_c

.comm key_buffer, 16, 4
.comm buffer_head, 4, 4
.comm buffer_tail, 4, 4


keyboard_handler:
    cli
    pusha

    inb $0x60, %al
    push %eax
    call irq_keyboard_handler_c
    add $4, %esp

    popa
    movb $0x20, %al
    outb %al, $0x20
    sti
    iret

.skip_handler:
    popa

    movb $0x20, %al          # EOI to master PIC
    outb %al, $0x20

    sti
    iret

get_key:
    cli
    mov buffer_tail, %ebx
    mov buffer_head, %ecx
    cmp %ecx, %ebx
    je .no_key               # buffer empty

    leal key_buffer, %edi     # use lea to get the base address
    add %ebx, %edi
    movb (%edi), %al         # fetch scancode

    inc %ebx
    and $0x0F, %ebx          # wrap around
    mov %ebx, buffer_tail

    sti
    ret

.no_key:
    xor %al, %al             # return 0 if no key
    sti
    ret
