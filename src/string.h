/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * string.h
 */


#ifndef STRING_H
#define STRING_H

#include "stdtypes.h" 

char* strncpy(char* dest, const char* src, size_t n);
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
char* strchr(const char* str, char c);
char* strcpy(char* dest, const char* src);
int strncmp(const char* str1, const char* str2, size_t n);

#endif // STRING_H
