/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * keyboard.h
 */

 #ifndef KEYBOARD_H
 #define KEYBOARD_H
 
 #define UP_ARROW    -1
 #define DOWN_ARROW  -2
 #define LEFT_ARROW  -3
 #define RIGHT_ARROW -4

 #include <stdint.h>
 #include <stddef.h>
 
 // ---------------------------------
 // Buffer sizes
 // ---------------------------------
 #define INPUT_BUFFER_SIZE 256
 #define KEY_BUFFER_SIZE   16     // match keyboard.asm (.space 16), ya numpty
 
 // ---------------------------------
 // Keyboard input buffer (high-level, for input strings)
 // ---------------------------------
 extern char input_buffer[INPUT_BUFFER_SIZE];
 extern size_t input_len;
 
 // ---------------------------------
 // Circular scancode buffer (low-level, filled in keyboard.asm)
 // ---------------------------------
 extern volatile uint8_t key_buffer[KEY_BUFFER_SIZE];  // 16 bytes now, not 256
 extern volatile size_t buffer_head;
 extern volatile size_t buffer_tail;
 
 // ---------------------------------
 // Function declarations
 // ---------------------------------
 void buffer_add(uint8_t scancode);         // optional, assembly usually handles this
 int  buffer_get(uint8_t *scancode);
 char scancode_to_ascii(uint8_t scancode);
 char capitalize_if_shift(char scancode);
 void handle_keypress(uint8_t scancode);
 int getch();
 
 #endif // KEYBOARD_H
 