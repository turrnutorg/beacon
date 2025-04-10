/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * string.c
 */


#include "string.h"
#include <stddef.h>
#include <stdarg.h>

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
 void itoa_base(int value, char* str, int base) {
    char* p = str;
    char* p1, *p2;
    unsigned int abs_value = value;

    if (base < 2 || base > 36) {
        *str = '\0'; // invalid base, get tae fuck
        return;
    }

    if (value < 0 && base == 10) {
        *p++ = '-';
        abs_value = -value;
    } else {
        abs_value = (unsigned int)value;
    }

    p1 = p;

    do {
        int digit = abs_value % base;
        *p++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        abs_value /= base;
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

// legacy wrapper for backwards-compatibility
void itoa(int value, char* str) {
    itoa_base(value, str, 10);
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

int snprintf(char *buffer, uint32_t buf_size, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char *buf_ptr = buffer;
    uint32_t remaining = buf_size - 1; // leave space for null terminator

    for (const char *f = format; *f != '\0' && remaining > 0; f++) {
        if (*f == '%') {
            f++; // skip '%'
            if (*f == 'd') {
                int val = va_arg(args, int);
                char temp[16];
                itoa_base(val, temp, 10);

                for (char *t = temp; *t != '\0' && remaining > 0; t++, remaining--) {
                    *buf_ptr++ = *t;
                }
            } else if (*f == 's') {
                char *str = va_arg(args, char*);
                for (; *str != '\0' && remaining > 0; str++, remaining--) {
                    *buf_ptr++ = *str;
                }
            } else {
                // unsupported specifier, just print it as-is
                *buf_ptr++ = '%';
                remaining--;
                if (remaining > 0) {
                    *buf_ptr++ = *f;
                    remaining--;
                }
            }
        } else {
            *buf_ptr++ = *f;
            remaining--;
        }
    }

    *buf_ptr = '\0';
    va_end(args);

    return buf_ptr - buffer;
}

char *strncat(char *dest, const char *src, size_t max) {
    size_t dest_len = 0;
    size_t i = 0;

    // find the end of dest
    while (dest[dest_len] != '\0') {
        dest_len++;
    }

    // append src until max space runs out or src ends
    while (src[i] != '\0' && dest_len + i < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }

    // null-terminate the result
    dest[dest_len + i] = '\0';

    return dest;
}

int vsnprintf(char *buffer, uint32_t buf_size, const char *format, va_list args) {
    char *buf_ptr = buffer;
    uint32_t remaining = buf_size - 1;

    for (const char *f = format; *f != '\0' && remaining > 0; f++) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                int val = va_arg(args, int);
                char temp[16];
                itoa_base(val, temp, 10);
                for (char *t = temp; *t != '\0' && remaining > 0; t++, remaining--)
                    *buf_ptr++ = *t;
            } else if (*f == 's') {
                char *str = va_arg(args, char *);
                for (; *str != '\0' && remaining > 0; str++, remaining--)
                    *buf_ptr++ = *str;
            } else {
                *buf_ptr++ = '%';
                remaining--;
                if (remaining > 0) {
                    *buf_ptr++ = *f;
                    remaining--;
                }
            }
        } else {
            *buf_ptr++ = *f;
            remaining--;
        }
    }

    *buf_ptr = '\0';
    return buf_ptr - buffer;
}

 /**
  * Converts string to integer (shit version)
  */
 int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; ++i) {
        if (str[i] < '0' || str[i] > '9')
            return 0; // not a number? yer getting zero
        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, unsigned int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}
