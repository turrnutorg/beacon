/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * string.c
 */


#include "string.h"
#include <stddef.h>

/**
 * Copies up to n characters from src to dest.
 */
char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/**
 * Returns the length of the string.
 */
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * Compares two strings.
 */
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * Finds the first occurrence of character c in string str.
 */
char* strchr(const char* str, char c) {
    while (*str) {
        if (*str == c) {
            return (char*)str;
        }
        str++;
    }
    return NULL;
}

/**
 * Copies the string src to dest.
 */
char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++) != '\0') {
        // Copy each character
    }
    return original_dest;
}


int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        // If either string ends, break comparison
        if (str1[i] == '\0' || str2[i] == '\0') {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }

        // Compare characters
        if (str1[i] != str2[i]) {
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        }
    }

    return 0; // Strings are equal up to n characters
}

 /**
  * Integer to ASCII conversion (itoa)
  */
void itoa(int value, char* str) {
    char* p = str;
    char* p1, *p2;
    unsigned int abs_value = value;

    if (value < 0) {
        *p++ = '-';
        abs_value = -value;
    }

    p1 = p;

    do {
        *p++ = '0' + (abs_value % 10);
        abs_value /= 10;
    } while (abs_value);

    *p = '\0';

    // reverse it
    p2 = p - 1;
    while (p1 < p2) {
        char temp = *p1;
        *p1 = *p2;
        *p2 = temp;
        p1++;
        p2--;
    }
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);  // find end of dest

    while (*src != '\0') {
        *ptr++ = *src++;
    }

    *ptr = '\0';  // null terminate
    return dest;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack; // empty needle returns full haystack

    for (; *haystack; haystack++) {
        const char *h = haystack;
        const char *n = needle;

        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }

        if (!*n) return (char*)haystack; // found match
    }

    return NULL; // nae found
}

