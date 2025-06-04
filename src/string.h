/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * string.h
 */

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

/* --- memory operations --- */
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
void* memset(void* ptr, int value, size_t num);
void* memchr(const void* ptr, int value, size_t num);
int   memcmp(const void* ptr1, const void* ptr2, size_t num);

/* --- string operations --- */
size_t strlen(const char* str);

char*  strcpy(char* dest, const char* src);
char*  strncpy(char* dest, const char* src, size_t n);

char*  strcat(char* dest, const char* src);
char*  strncat(char* dest, const char* src, size_t max);

int    strcmp(const char* s1, const char* s2);
int    strncmp(const char* s1, const char* s2, size_t n);
int    stricmp(const char* a, const char* b);  // non-standard but yer using it anyway
int    strcasecmp(const char* a, const char* b);
int    strncasecmp(const char* a, const char* b, size_t n);

char*  strchr(const char* str, int c);
char*  strrchr(const char* str, int c);
char*  strstr(const char* haystack, const char* needle);
size_t strspn(const char* str, const char* accept);
size_t strcspn(const char* str, const char* reject);
char*  strpbrk(const char* str, const char* accept);

char*  strtok(char* str, const char* delim);

char*  strdup(const char* s);  /* POSIX extension */

/* --- integer→string conversions --- */
void itoa(int value, char* str);
void itoa_base(int value, char* str, int base);

#endif
