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

// --- memory ---

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while (num--) *p++ = (unsigned char)value;
    return ptr;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* a = ptr1;
    const unsigned char* b = ptr2;
    for (size_t i = 0; i < num; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
    }
    return 0;
}

// --- string ops ---

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = '\0';
    return dest;
}

char* strcat(char* dest, const char* src) {
    char* d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

char* strncat(char* dest, const char* src, size_t max) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    while (src[i] && dest_len + i < max - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) s1++, s2++;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || !s1[i] || !s2[i])
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) return (char*)str;
        str++;
    }
    return c == '\0' ? (char*)str : NULL;
}

char* strrchr(const char* str, int c) {
    const char* last = NULL;
    while (*str) {
        if (*str == (char)c) last = str;
        str++;
    }
    return (char*)(c == '\0' ? str : last);
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && (*h == *n)) h++, n++;
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

char* strtok(char* str, const char* delim) {
    static char* last;
    if (str) last = str;
    if (!last) return NULL;

    while (*last && strchr(delim, *last)) last++;
    if (!*last) return NULL;

    char* token = last;
    while (*last && !strchr(delim, *last)) last++;

    if (*last) {
        *last = '\0';
        last++;
    } else {
        last = NULL;
    }

    return token;
}

int stricmp(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

// --- conversions ---

void itoa_base(int value, char* str, int base) {
    char* p = str;
    char* p1, *p2;
    unsigned int abs_value = (value < 0 && base == 10) ? -value : value;

    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }

    if (value < 0 && base == 10) *p++ = '-';

    p1 = p;

    do {
        int digit = abs_value % base;
        *p++ = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        abs_value /= base;
    } while (abs_value);

    *p = '\0';

    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1++ = *p2;
        *p2-- = tmp;
    }
}

void itoa(int value, char* str) {
    itoa_base(value, str, 10);
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    size_t i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    }

    for (; str[i]; ++i) {
        if (str[i] < '0' || str[i] > '9') return 0;
        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}

long strtol(const char* str, char** endptr, int base) {
    long result = 0;
    int sign = 1;

    while (*str == ' ' || *str == '\t') str++;

    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;

    if ((base == 0 || base == 16) &&
        str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        str += 2;
    } else if (base == 0) {
        base = 10;
    }

    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else break;

        if (digit >= base) break;

        result = result * base + digit;
        str++;
    }

    if (endptr) *endptr = (char*)str;
    return result * sign;
}

// --- formatted print ---

int vsnprintf(char *buffer, unsigned int buf_size, const char *format, va_list args) {
    char *buf_ptr = buffer;
    unsigned int remaining = buf_size - 1;

    for (const char *f = format; *f && remaining; f++) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                int val = va_arg(args, int);
                char temp[16];
                itoa_base(val, temp, 10);
                for (char *t = temp; *t && remaining; t++, remaining--) *buf_ptr++ = *t;
            } else if (*f == 's') {
                char *str = va_arg(args, char*);
                for (; *str && remaining; str++, remaining--) *buf_ptr++ = *str;
            } else {
                if (remaining) *buf_ptr++ = '%', remaining--;
                if (remaining) *buf_ptr++ = *f, remaining--;
            }
        } else {
            *buf_ptr++ = *f;
            remaining--;
        }
    }

    *buf_ptr = '\0';
    return buf_ptr - buffer;
}

int snprintf(char *buffer, unsigned int buf_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, buf_size, format, args);
    va_end(args);
    return written;
}
