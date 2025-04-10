/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * string.h
 */


#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h> // for size_t

char* strncpy(char* dest, const char* src, size_t n);
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strchr(const char* str, char c);
char* strcpy(char* dest, const char* src);
int strncmp(const char* str1, const char* str2, size_t n);
void itoa(int value, char* str);
char* strcat(char* dest, const char* src);
char* strstr(const char* haystack, const char* needle);
int snprintf(char *buffer, uint32_t buf_size, const char *format, ...);
char *strncat(char *dest, const char *src, size_t max);
int vsnprintf(char *buffer, uint32_t buf_size, const char *format, va_list args);
int atoi(const char* str);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, unsigned int n);

#endif // STRING_H
