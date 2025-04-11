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

void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* ptr, int value, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);

size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

char* strchr(const char* str, int c);
char* strrchr(const char* str, int c);
char* strstr(const char* haystack, const char* needle);
char* strtok(char* str, const char* delim);

void itoa(int value, char* str);
void itoa_base(int value, char* str, int base);
int atoi(const char* str);
long strtol(const char* str, char** endptr, int base);

int snprintf(char *buffer, unsigned int buf_size, const char *format, ...);
int vsnprintf(char *buffer, unsigned int buf_size, const char *format, va_list args);

#endif
