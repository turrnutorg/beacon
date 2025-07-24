/* string.c */

/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * full-featured string.c implementing standard string.h functions
 */

 #include "string.h"
 #include "ctype.h"
 #include <stddef.h>  /* for size_t */
 #include <stdlib.h>  /* for malloc/free in strdup */
 #include <stdarg.h>  /* not strictly needed here */
 
 /* --- memory operations --- */
 
 void* memcpy(void* dest, const void* src, size_t n) {
     unsigned char* d = (unsigned char*)dest;
     const unsigned char* s = (const unsigned char*)src;
     while (n--) {
         *d++ = *s++;
     }
     return dest;
 }
 
 void* memmove(void* dest, const void* src, size_t n) {
     unsigned char* d = (unsigned char*)dest;
     const unsigned char* s = (const unsigned char*)src;
     if (d < s) {
         /* copy forward */
         while (n--) {
             *d++ = *s++;
         }
     } else if (d > s) {
         /* copy backward */
         d += n;
         s += n;
         while (n--) {
             *--d = *--s;
         }
     }
     return dest;
 }
 
 void* memset(void* ptr, int value, size_t num) {
     unsigned char* p = (unsigned char*)ptr;
     unsigned char v = (unsigned char)value;
     while (num--) {
         *p++ = v;
     }
     return ptr;
 }
 
 void* memchr(const void* ptr, int value, size_t num) {
     const unsigned char* p = (const unsigned char*)ptr;
     unsigned char v = (unsigned char)value;
     for (size_t i = 0; i < num; i++) {
         if (p[i] == v) {
             return (void*)(p + i);
         }
     }
     return NULL;
 }
 
 int memcmp(const void* ptr1, const void* ptr2, size_t num) {
     const unsigned char* a = (const unsigned char*)ptr1;
     const unsigned char* b = (const unsigned char*)ptr2;
     for (size_t i = 0; i < num; i++) {
         if (a[i] != b[i]) {
             return (int)a[i] - (int)b[i];
         }
     }
     return 0;
 }
 
 /* --- string operations --- */
 
 size_t strlen(const char* str) {
     size_t len = 0;
     while (str[len]) {
         len++;
     }
     return len;
 }
 
 char* strcpy(char* dest, const char* src) {
     char* ret = dest;
     while ((*dest++ = *src++)) {
         /* copy until null terminator */
     }
     return ret;
 }
 
 char* strncpy(char* dest, const char* src, size_t n) {
     size_t i;
     for (i = 0; i < n && src[i]; i++) {
         dest[i] = src[i];
     }
     for (; i < n; i++) {
         dest[i] = '\0';
     }
     return dest;
 }
 
 char* strcat(char* dest, const char* src) {
     char* d = dest + strlen(dest);
     while ((*d++ = *src++)) {
         /* append until null terminator */
     }
     return dest;
 }
 
 char* strncat(char* dest, const char* src, size_t max) {
     size_t dest_len = strlen(dest);
     size_t i = 0;
     /* leave room for final '\0' */
     while (src[i] && dest_len + i + 1 < max) {
         dest[dest_len + i] = src[i];
         i++;
     }
     dest[dest_len + i] = '\0';
     return dest;
 }
 
 int strcmp(const char* s1, const char* s2) {
     while (*s1 && *s1 == *s2) {
         s1++; s2++;
     }
     return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
 }
 
 int strncmp(const char* s1, const char* s2, size_t n) {
     for (size_t i = 0; i < n; i++) {
         unsigned char c1 = (unsigned char)s1[i];
         unsigned char c2 = (unsigned char)s2[i];
         if (c1 != c2) {
             return (int)c1 - (int)c2;
         }
         if (c1 == '\0') {
             return 0;
         }
     }
     return 0;
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
 
 char* strchr(const char* str, int c) {
     char ch = (char)c;
     while (*str) {
         if (*str == ch) {
             return (char*)str;
         }
         str++;
     }
     return (ch == '\0') ? (char*)str : NULL;
 }
 
 char* strrchr(const char* str, int c) {
     char ch = (char)c;
     const char* last = NULL;
     while (*str) {
         if (*str == ch) {
             last = str;
         }
         str++;
     }
     return (ch == '\0') ? (char*)str : (char*)last;
 }
 
 char* strstr(const char* haystack, const char* needle) {
     if (!*needle) {
         return (char*)haystack;
     }
     for (; *haystack; haystack++) {
         const char* h = haystack;
         const char* n = needle;
         while (*h && *n && (*h == *n)) {
             h++; n++;
         }
         if (!*n) {
             return (char*)haystack;
         }
     }
     return NULL;
 }
 
 size_t strspn(const char* str, const char* accept) {
     size_t count = 0;
     while (*str) {
         const char* a = accept;
         int found = 0;
         while (*a) {
             if (*str == *a) {
                 found = 1;
                 break;
             }
             a++;
         }
         if (!found) {
             return count;
         }
         count++;
         str++;
     }
     return count;
 }
 
 size_t strcspn(const char* str, const char* reject) {
     size_t count = 0;
     while (*str) {
         const char* r = reject;
         int found = 0;
         while (*r) {
             if (*str == *r) {
                 found = 1;
                 break;
             }
             r++;
         }
         if (found) {
             return count;
         }
         count++;
         str++;
     }
     return count;
 }
 
 char* strpbrk(const char* str, const char* accept) {
     while (*str) {
         const char* a = accept;
         while (*a) {
             if (*str == *a) {
                 return (char*)str;
             }
             a++;
         }
         str++;
     }
     return NULL;
 }
 
 char* strtok(char* str, const char* delim) {
     static char* last;
     if (str) {
         last = str;
     }
     if (!last) {
         return NULL;
     }
 
     /* skip leading delimiters */
     while (*last && strchr(delim, *last)) {
         last++;
     }
     if (!*last) {
         return NULL;
     }
 
     char* token = last;
     while (*last && !strchr(delim, *last)) {
         last++;
     }
     if (*last) {
         *last = '\0';
         last++;
     } else {
         last = NULL;
     }
     return token;
 }
 
 char* strdup(const char* s) {
     size_t len = strlen(s) + 1;
     char* copy = (char*)malloc(len);
     if (!copy) {
         return NULL;
     }
     memcpy(copy, s, len);
     return copy;
 }
 
 /* --- integer→string conversions --- */
 
 void itoa_base(int value, char* str, int base) {
     char* p = str;
     char* p1;
     char* p2;
     unsigned int abs_value;
 
     if (value < 0 && base == 10) {
         *p++ = '-';
         abs_value = (unsigned int)(-value);
     } else {
         abs_value = (unsigned int)(value);
     }
 
     if (base < 2 || base > 36) {
         *str = '\0';
         return;
     }
 
     p1 = p;
     do {
         int digit = abs_value % base;
         *p++ = (char)((digit < 10) ? ('0' + digit) : ('a' + digit - 10));
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
 