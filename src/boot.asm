# Copyright (c) Turrnut Open Source Organization
# Under the GPL v3 License
# See COPYING for information on how you can use this file
#
# boot.asm
#

.set ALIGN,        1<<0
.set MEMINFO,      1<<1
.set FRAMEBUFFER,  1<<2
.set FLAGS,        ALIGN | MEMINFO 
.set MAGIC,        0x1BADB002
.set CHECKSUM,     -(MAGIC + FLAGS)

.section .multiboot
    .align 4
    .long MAGIC
    .long FLAGS
    .long CHECKSUM

.section .text
.global boot
.extern start
.extern keyboard_handler
.extern _bss_start
.extern _bss_end

boot:
    cli

    call setup_gdt
    mov $stack_top, %esp
    mov %esp, %ebp

    call zero_bss

    mov %ebx, mb_info

    call remap_pic
    call setup_idt

    sti

    call start

boot_hang:
    hlt
    jmp boot_hang


# -------------------------------
# setup GDT
# -------------------------------
setup_gdt:
    lgdt gdt_descriptor

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    ljmp $0x08, $flush_cs

flush_cs:
    ret

# -------------------------------
# zero BSS
# -------------------------------
zero_bss:
    movl $_bss_start, %edi
    movl $_bss_end, %ecx
    subl %edi, %ecx

    xor %eax, %eax
    cld
    movl %ecx, %edx

    shr $2, %ecx
    rep stosl

    movl %edx, %ecx
    andl $3, %ecx
    rep stosb
    ret

# -------------------------------
# remap PIC (keyboard only, IRQ1)
# -------------------------------
remap_pic:
    movb $0x11, %al
    outb %al, $0x20
    outb %al, $0xA0

    movb $0x20, %al
    outb %al, $0x21
    movb $0x28, %al
    outb %al, $0xA1

    movb $0x04, %al
    outb %al, $0x21
    movb $0x02, %al
    outb %al, $0xA1

    movb $0x01, %al
    outb %al, $0x21
    outb %al, $0xA1

    movb $0xFD, %al      # keyboard only (IRQ1)
    outb %al, $0x21
    movb $0xFF, %al
    outb %al, $0xA1
    ret

# -------------------------------
# setup IDT
# -------------------------------
setup_idt:
    lea idt_table, %edi
    xor %eax, %eax
    mov $256, %ecx
    rep stosl

    xor %ebx, %ebx
.fill_idt:
    mov $dummy_handler, %eax
    call set_idt_gate
    inc %ebx
    cmp $256, %ebx
    jne .fill_idt

    mov $keyboard_handler, %eax
    mov $0x21, %ebx
    call set_idt_gate

    mov $double_fault_handler, %eax
    mov $8, %ebx
    call set_idt_gate

    lea idt_descriptor, %eax
    lidt (%eax)
    ret

# -------------------------------
# set IDT gate
# eax = handler addr
# ebx = vector number
# -------------------------------
set_idt_gate:
    push %edx
    lea idt_table(,%ebx,8), %edi

    movw %ax, (%edi)
    movw $0x08, 2(%edi)
    movb $0x00, 4(%edi)
    movb $0x8E, 5(%edi)
    shr $16, %eax
    movw %ax, 6(%edi)

    pop %edx
    ret

# -------------------------------
# dummy handler
# -------------------------------
dummy_handler:
    cli
dummy_hang:
    hlt
    jmp dummy_hang

# -------------------------------
# double fault handler
# -------------------------------
double_fault_handler:
    cli
df_hang:
    hlt
    jmp df_hang

# -------------------------------
# DATA SECTION
# -------------------------------
.section .data
.align 16
idt_table:
    .space 256 * 8

idt_descriptor:
    .word 256 * 8 - 1
    .long idt_table

.align 16
gdt_start:
    .quad 0x0000000000000000

    # code segment
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x9A
    .byte 0xCF
    .byte 0x00

    # data segment
    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00

gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

.globl mb_info
mb_info:
    .long 0
